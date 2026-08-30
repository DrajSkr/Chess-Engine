import { useState, useCallback, useEffect, useMemo, useRef } from 'react';
import { Chessboard } from 'react-chessboard';
import { Chess } from 'chess.js';
import EvalBar from './components/EvalBar';
import GameModeSelector from './components/GameModeSelector';
import { useEngineWebSocket } from './hooks/useEngineWebSocket';
import { useP2P } from './hooks/useP2P';
import './App.css';

// Game modes
const MODES = {
  LOCAL_PVP: 'LOCAL_PVP',
  VS_BOT: 'VS_BOT',
  P2P: 'P2P',
};

function App() {
  // ── Core State ──────────────────────────────────────────
  const [game, setGame] = useState(new Chess());
  const gameRef = useRef(game);
  
  // Keep ref in sync
  useEffect(() => {
    gameRef.current = game;
  }, [game]);

  const [gameMode, setGameMode] = useState(MODES.LOCAL_PVP);
  const [boardOrientation, setBoardOrientation] = useState('white');
  const [evalScore, setEvalScore] = useState(0);
  const [selectedSquare, setSelectedSquare] = useState(null);
  const [moveHistory, setMoveHistory] = useState([]);
  const [gameStatus, setGameStatus] = useState('');
  const [showModeSelector, setShowModeSelector] = useState(true);
  const [joinCodeInput, setJoinCodeInput] = useState('');

  // Promotion state
  const [moveFrom, setMoveFrom] = useState(null);
  const [moveTo, setMoveTo] = useState(null);
  const [showPromotionDialog, setShowPromotionDialog] = useState(false);

  // ── WebSocket ─────────────────────────
  const { isConnected, isThinking, lastMessage, sendMove, sendNewGame } = useEngineWebSocket();

  // ── P2P WebRTC ────────────────────────
  const onP2PMove = useCallback((msg) => {
    if (msg.fen) {
      const newGame = new Chess();
      newGame.load(msg.fen);
      setGame(newGame);
      
      // Update history using the move string
      setMoveHistory((prev) => [...prev, msg.move]);

      if (newGame.isCheckmate()) setGameStatus('Checkmate! You lose.');
      else if (newGame.isDraw() || newGame.isStalemate()) setGameStatus('Draw!');
      else if (newGame.isCheck()) setGameStatus('Check!');
      else setGameStatus('');
    }
  }, []);

  const onOpponentLeft = useCallback(() => {
    setGameStatus('Opponent disconnected.');
  }, []);

  const {
    isP2PConnected,
    isInRoom,
    roomCode,
    playerColor,
    createRoom,
    joinRoom,
    leaveRoom,
    sendP2PMove
  } = useP2P({ onP2PMove, onOpponentLeft });

  // Sync P2P color with board orientation
  useEffect(() => {
    if (gameMode === MODES.P2P && playerColor) {
      setBoardOrientation(playerColor);
      setGameStatus(`Match started! You are ${playerColor}`);
      const newGame = new Chess();
      setGame(newGame);
      setMoveHistory([]);
    }
  }, [gameMode, playerColor]);

  // ── Process WebSocket messages from engine ──────────────
  useEffect(() => {
    if (!lastMessage) return;

    // VS_BOT Logic
    if (gameMode === MODES.VS_BOT) {
      if (lastMessage.type === 'game_state') {
        const newGame = new Chess(lastMessage.fen);
        setGame(newGame);
        setEvalScore(lastMessage.score || 0);
        setMoveHistory([]);
        setGameStatus('');
      } else if (lastMessage.type === 'move_result') {
        if (lastMessage.valid && lastMessage.fen) {
          const newGame = new Chess(lastMessage.fen);
          setGame(newGame);
          setEvalScore(lastMessage.score || 0);

          if (lastMessage.bestmove) {
            setMoveHistory((prev) => [...prev, lastMessage.bestmove]);
          }

          if (newGame.isCheckmate()) {
            setGameStatus('Checkmate! Engine wins.');
          } else if (newGame.isDraw()) {
            setGameStatus('Draw!');
          } else if (newGame.isStalemate()) {
            setGameStatus('Stalemate!');
          } else if (newGame.isCheck()) {
            setGameStatus('Check!');
          } else {
            setGameStatus('');
          }
        }
      } else if (lastMessage.type === 'game_over') {
        if (lastMessage.fen) {
          const newGame = new Chess(lastMessage.fen);
          setGame(newGame);
        }
        const reason = lastMessage.reason === 'checkmate' ? 'Checkmate!' : 'Stalemate!';
        setGameStatus(`Game Over — ${reason}`);
      }
    }

    if (lastMessage.type === 'error') {
      console.error('[Engine Error]', lastMessage.message);
    }
  }, [lastMessage, gameMode]);

  // ── Legal move highlights for selected piece ────────────
  const legalMoveSquares = useMemo(() => {
    if (!selectedSquare) return {};
    const moves = game.moves({ square: selectedSquare, verbose: true });
    const styles = {};
    // Highlight selected square
    styles[selectedSquare] = {
      backgroundColor: 'rgba(255, 255, 0, 0.4)',
    };
    moves.forEach((move) => {
      styles[move.to] = {
        background:
          game.get(move.to) // is there a piece on target?
            ? 'radial-gradient(circle, rgba(0,0,0,.1) 85%, transparent 85%)'
            : 'radial-gradient(circle, rgba(0,0,0,.2) 25%, transparent 25%)',
        borderRadius: '50%',
      };
    });
    return styles;
  }, [selectedSquare, game]);

  // ── Compute final styles including check warning ──────────
  const finalSquareStyles = useMemo(() => {
    const styles = { ...legalMoveSquares };

    if (game.isCheck() || game.isCheckmate()) {
      const turn = game.turn();
      const board = game.board();
      for (let r = 0; r < 8; r++) {
        for (let c = 0; c < 8; c++) {
          const piece = board[r][c];
          if (piece && piece.type === 'k' && piece.color === turn) {
            const files = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'];
            const square = files[c] + (8 - r);
            styles[square] = {
              ...styles[square],
              background: 'radial-gradient(circle, rgba(255,0,0,0.8) 0%, rgba(255,0,0,0.6) 50%, rgba(255,0,0,0) 100%)',
              borderRadius: '50%',
              boxShadow: 'inset 0 0 10px rgba(255,0,0,0.8)'
            };
          }
        }
      }
    }
    return styles;
  }, [legalMoveSquares, game]);

  // ── Convert react-chessboard square coords to engine move string ──
  const toMoveString = useCallback((from, to, promotion) => {
    let moveStr = from + to;
    if (promotion) moveStr += promotion;
    return moveStr;
  }, []);

  const executeMove = useCallback((sourceSquare, targetSquare, promotionPiece) => {
      // Validate with chess.js (using the ref to avoid stale state)
      const gameCopy = new Chess(gameRef.current.fen());
      let move;
      try {
        move = gameCopy.move({
          from: sourceSquare,
          to: targetSquare,
          promotion: promotionPiece,
        });
      } catch {
        return false; // illegal move
      }

      if (!move) return false;

      // Add to move history
      setMoveHistory((prev) => [...prev, move.san]);

      if (gameMode === MODES.VS_BOT) {
        // Update local board immediately with player's move
        setGame(gameCopy);

        // Check if game is over after player's move
        if (gameCopy.isGameOver()) {
          if (gameCopy.isCheckmate()) {
            setGameStatus('Checkmate! You win!');
          } else if (gameCopy.isDraw()) {
            setGameStatus('Draw!');
          } else if (gameCopy.isStalemate()) {
            setGameStatus('Stalemate!');
          }
          return true;
        }

        // Send move to engine
        const moveStr = toMoveString(sourceSquare, targetSquare, promotionPiece);
        sendMove(moveStr, gameCopy.fen());
        return true;
      }

      if (gameMode === MODES.LOCAL_PVP) {
        setGame(gameCopy);

        // Check game status
        if (gameCopy.isCheckmate()) {
          setGameStatus(`Checkmate! ${gameCopy.turn() === 'w' ? 'Black' : 'White'} wins!`);
        } else if (gameCopy.isDraw()) {
          setGameStatus('Draw!');
        } else if (gameCopy.isStalemate()) {
          setGameStatus('Stalemate!');
        } else if (gameCopy.isCheck()) {
          setGameStatus('Check!');
        } else {
          setGameStatus('');
        }
        return true;
      }

      if (gameMode === MODES.P2P) {
        setGame(gameCopy);
        const moveStr = toMoveString(sourceSquare, targetSquare, promotionPiece);
        sendP2PMove(moveStr, gameCopy.fen());

        if (gameCopy.isCheckmate()) {
          setGameStatus('Checkmate! You win!');
        } else if (gameCopy.isDraw() || gameCopy.isStalemate()) {
          setGameStatus('Draw!');
        } else if (gameCopy.isCheck()) {
          setGameStatus('Check!');
        } else {
          setGameStatus('');
        }
        return true;
      }

      return false;
  }, [gameMode, sendMove, sendP2PMove, toMoveString]);

  // ── Handle piece drop ───────────────────────────────────
  const onDrop = useCallback(
    (sourceSquare, targetSquare, piece) => {
      setSelectedSquare(null);

      // Determine promotion piece if applicable
      const isPromotion =
        piece[1] === 'P' &&
        ((piece[0] === 'w' && targetSquare[1] === '8') ||
          (piece[0] === 'b' && targetSquare[1] === '1'));

      if (isPromotion) {
        // Only allow promotion dialog if it's a legal move
        const gameCopy = new Chess(gameRef.current.fen());
        try {
          // test if the move is legal (assume Queen promotion for test)
          const move = gameCopy.move({ from: sourceSquare, to: targetSquare, promotion: 'q' });
          if (move) {
            setMoveFrom(sourceSquare);
            setMoveTo(targetSquare);
            setShowPromotionDialog(true);
          }
        } catch {}
        return false;
      }

      return executeMove(sourceSquare, targetSquare);
    },
    [executeMove]
  );

  const onPromotionPieceSelect = useCallback(
    (piece) => {
      if (piece) {
        const promotionPiece = piece[1].toLowerCase();
        executeMove(moveFrom, moveTo, promotionPiece);
      }
      setMoveFrom(null);
      setMoveTo(null);
      setShowPromotionDialog(false);
      return true;
    },
    [moveFrom, moveTo, executeMove]
  );

  // ── Handle square click (for click-to-move) ────────────
  const onSquareClick = useCallback(
    (square) => {
      if (isThinking) return;
      if (game.isGameOver()) return;

      // If a piece is already selected, try to make a move
      if (selectedSquare) {
        const piece = game.get(selectedSquare);
        if (piece) {
          const isPromotion =
            piece.type === 'p' &&
            ((piece.color === 'w' && square[1] === '8') ||
              (piece.color === 'b' && square[1] === '1'));

          const result = onDrop(selectedSquare, square, piece.type === 'p' ? (piece.color + 'P') : (piece.color + piece.type.toUpperCase()));
          if (result) {
            setSelectedSquare(null);
            return;
          }
        }
      }

      // Select a piece (only if it belongs to the current player)
      const piece = game.get(square);
      if (piece && piece.color === game.turn()) {
        if (gameMode === MODES.VS_BOT && piece.color !== 'w') return;
        if (gameMode === MODES.P2P && (!isP2PConnected || piece.color !== boardOrientation[0])) return;
        
        setSelectedSquare(square);
      } else {
        setSelectedSquare(null);
      }
    },
    [selectedSquare, game, onDrop, isThinking, gameMode, isP2PConnected, boardOrientation]
  );

  // ── New Game handler ────────────────────────────────────
  const handleNewGame = useCallback(() => {
    const newGame = new Chess();
    setGame(newGame);
    setEvalScore(0);
    setSelectedSquare(null);
    setMoveHistory([]);
    setGameStatus('');

    if (gameMode === MODES.VS_BOT) {
      sendNewGame();
    }
  }, [gameMode, sendNewGame]);

  // ── Mode selection handler ──────────────────────────────
  const handleModeSelect = useCallback(
    (mode) => {
      setGameMode(mode);
      setShowModeSelector(false);
      // Reset game state for the new mode
      const newGame = new Chess();
      setGame(newGame);
      setEvalScore(0);
      setSelectedSquare(null);
      setMoveHistory([]);
      setGameStatus('');

      if (mode === MODES.VS_BOT) {
        sendNewGame();
      } else if (mode === MODES.P2P) {
        // Just show lobby, user will click create/join
      } else {
        if (isInRoom) leaveRoom();
      }
    },
    [sendNewGame, isInRoom, leaveRoom]
  );

  // ── Flip Board handler ──────────────────────────────────
  const handleFlipBoard = useCallback(() => {
    setBoardOrientation((prev) => (prev === 'white' ? 'black' : 'white'));
  }, []);

  // ── Determine if pieces are draggable ───────────────────
  const isDraggablePiece = useCallback(
    ({ piece }) => {
      if (isThinking) return false; // Don't allow moves while engine is thinking
      if (game.isGameOver()) return false;

      if (gameMode === MODES.VS_BOT) {
        // Only allow dragging white pieces (player is white)
        return piece[0] === 'w' && game.turn() === 'w';
      }

      if (gameMode === MODES.P2P) {
        if (!isP2PConnected) return false;
        // Only allow dragging if it's your assigned color's turn
        return piece[0] === boardOrientation[0] && game.turn() === boardOrientation[0];
      }

      // LOCAL_PVP: allow dragging current turn's pieces
      return piece[0] === game.turn().charAt(0);
    },
    [game, gameMode, isThinking, boardOrientation, isP2PConnected]
  );

  // ── Render ──────────────────────────────────────────────
  if (showModeSelector) {
    return (
      <div className="app-container">
        <header className="app-header">
          <h1 className="app-title">
            <span className="title-icon">♔</span> Domino Chess Engine
          </h1>
          <p className="app-subtitle">Choose your game mode</p>
        </header>
        <GameModeSelector
          currentMode={gameMode}
          onSelectMode={handleModeSelect}
          isConnected={isConnected}
        />
        <div className="connection-status">
          <span className={`status-dot ${isConnected ? 'online' : 'offline'}`} />
          <span>{isConnected ? 'Engine Connected' : 'Engine Offline'}</span>
        </div>
        
        {/* Footer */}
        <div className="app-footer">
          Made by DrajSkr
        </div>
      </div>
    );
  }

  return (
    <div className="app-container">
      {/* Header */}
      <header className="app-header compact">
        <h1 className="app-title small" onClick={() => setShowModeSelector(true)}>
          <span className="title-icon">♔</span> Domino
        </h1>
        <div className="header-controls">
          <span className={`mode-indicator ${gameMode.toLowerCase()}`}>
            {gameMode === MODES.LOCAL_PVP ? '👥 PvP' : (gameMode === MODES.P2P ? '🌐 Online P2P' : '🤖 vs Engine')}
          </span>
          {gameMode !== MODES.LOCAL_PVP && (
            <span className={`status-dot ${isConnected ? 'online' : 'offline'}`} />
          )}
        </div>
      </header>

      {/* Main game area */}
      <div className="game-area">
        {/* Eval bar (VS_BOT mode only) */}
        {gameMode === MODES.VS_BOT && (
          <EvalBar score={evalScore} flipped={boardOrientation === 'black'} />
        )}

        {/* Chess board */}
        <div className="board-wrapper">
          {gameMode === MODES.P2P && !isInRoom ? (
            <div className="p2p-lobby-overlay">
              <div className="p2p-lobby-content">
                <h2>Online Matchmaking</h2>
                <div className="lobby-actions">
                  <button className="btn btn-primary" onClick={createRoom}>
                    Create Room
                  </button>
                  <div className="join-section">
                    <input 
                      type="text" 
                      placeholder="Room Code" 
                      value={joinCodeInput} 
                      onChange={(e) => setJoinCodeInput(e.target.value.toUpperCase())}
                      maxLength={6}
                    />
                    <button className="btn btn-secondary" onClick={() => joinRoom(joinCodeInput)}>
                      Join Room
                    </button>
                  </div>
                </div>
                {roomCode && !isP2PConnected && (
                  <div className="room-code-display">
                    Your Room Code: <strong>{roomCode}</strong>
                    <p>Waiting for opponent...</p>
                  </div>
                )}
              </div>
            </div>
          ) : (
            <Chessboard
              id="main-board"
              position={game.fen()}
              onPieceDrop={onDrop}
              onSquareClick={onSquareClick}
              boardOrientation={boardOrientation}
              isDraggablePiece={isDraggablePiece}
              customSquareStyles={finalSquareStyles}
              promotionToSquare={moveTo}
              showPromotionDialog={showPromotionDialog}
              onPromotionPieceSelect={onPromotionPieceSelect}
              customBoardStyle={{
                borderRadius: '8px',
                boxShadow: '0 8px 32px rgba(0, 0, 0, 0.4)',
              }}
              customDarkSquareStyle={{ backgroundColor: '#779952' }}
              customLightSquareStyle={{ backgroundColor: '#edeed1' }}
              animationDuration={200}
            />
          )}
        </div>
      </div>

      {/* Controls + Info panel */}
      <div className="info-panel">
        {/* Game status */}
        {gameStatus && <div className="game-status">{gameStatus}</div>}

        {/* Thinking indicator */}
        {isThinking && (
          <div className="thinking-indicator">
            <div className="thinking-spinner" />
            <span>Engine is thinking...</span>
          </div>
        )}

        {/* P2P Status Indicator */}
        {gameMode === MODES.P2P && isInRoom && (
          <div className="thinking-indicator" style={{ background: isP2PConnected ? 'var(--success-glass)' : 'var(--warning-glass)' }}>
            <span className={`status-dot ${isP2PConnected ? 'online' : 'offline'}`} />
            <span>{isP2PConnected ? 'Peer Connected' : 'Connecting to peer (WebRTC)...'}</span>
            <span style={{marginLeft: 'auto'}}>Room: <strong>{roomCode}</strong></span>
          </div>
        )}

        {/* Control buttons */}
        <div className="controls">
          {gameMode !== MODES.P2P && (
            <button className="btn btn-primary" onClick={handleNewGame}>
              🔄 New Game
            </button>
          )}
          <button className="btn btn-secondary" onClick={handleFlipBoard}>
            🔃 Flip Board
          </button>
          <button className="btn btn-ghost" onClick={() => {
            if (isInRoom) leaveRoom();
            setShowModeSelector(true);
          }}>
            ← Change Mode
          </button>
        </div>

        {/* Move history */}
        {moveHistory.length > 0 && (
          <div className="move-history">
            <h3>Move History</h3>
            <div className="moves-list">
              {moveHistory.map((move, i) => (
                <span key={i} className={`move-item ${i % 2 === 0 ? 'white-move' : 'black-move'}`}>
                  {i % 2 === 0 && <span className="move-number">{Math.floor(i / 2) + 1}.</span>}
                  {move}
                </span>
              ))}
            </div>
          </div>
        )}

        {/* Turn indicator */}
        {(gameMode !== MODES.P2P || (gameMode === MODES.P2P && isP2PConnected)) && (
          <div className="turn-indicator">
            <div className={`turn-dot ${game.turn() === 'w' ? 'white' : 'black'}`} />
            <span>{game.turn() === 'w' ? 'White' : 'Black'} to move</span>
          </div>
        )}
      </div>
      
      {/* Footer */}
      <div className="app-footer">
        Made by DrajSkr
      </div>
    </div>
  );
}

export default App;
