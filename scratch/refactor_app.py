import re

with open("frontend/src/App.jsx", "r", encoding="utf-8") as f:
    content = f.read()

# 1. State additions
state_add = """  const [initialFen, setInitialFen] = useState(new Chess().fen());
  const [currentMoveIndex, setCurrentMoveIndex] = useState(-1);
  const currentMoveIndexRef = useRef(currentMoveIndex);
  
  useEffect(() => {
    currentMoveIndexRef.current = currentMoveIndex;
  }, [currentMoveIndex]);
"""
content = re.sub(
    r"const \[moveHistory, setMoveHistory\] = useState\(\[\]\);",
    r"const [moveHistory, setMoveHistory] = useState([]);\n" + state_add,
    content
)

# 2. jumpToMove function
jump_to_move = """
  const jumpToMove = useCallback((index) => {
    setCurrentMoveIndex(index);
    if (index === -1) {
      setGame(new Chess(initialFen));
    } else {
      setGame(new Chess(moveHistory[index].fen));
    }
    setSelectedSquare(null);
  }, [moveHistory, initialFen]);
"""
content = re.sub(
    r"(const handleNewGame = useCallback\(\(\) => \{)",
    jump_to_move + r"\n  \1",
    content
)

# 3. handleNewGame updates
content = re.sub(
    r"setGame\(newGame\);\n\s*setEvalScore\(0\);\n\s*setSelectedSquare\(null\);\n\s*setMoveHistory\(\[\]\);",
    r"setGame(newGame);\n    setInitialFen(newGame.fen());\n    setEvalScore(0);\n    setSelectedSquare(null);\n    setMoveHistory([]);\n    setCurrentMoveIndex(-1);",
    content
)

# 4. P2P onP2PMove
content = re.sub(
    r"setMoveHistory\(\(prev\) => \[\.\.\.prev, msg\.move\]\);",
    r"""setMoveHistory((prev) => {
        const idx = currentMoveIndexRef.current;
        const newHist = prev.slice(0, idx + 1);
        newHist.push({ san: msg.move, fen: msg.fen, from: '', to: '' }); 
        setCurrentMoveIndex(newHist.length - 1);
        return newHist;
      });""",
    content
)

# 5. P2P Sync
content = re.sub(
    r"setGameStatus\(`Match started! You are \$\{playerColor\}`\);\n\s*const newGame = new Chess\(\);\n\s*setGame\(newGame\);\n\s*setMoveHistory\(\[\]\);",
    r"setGameStatus(`Match started! You are ${playerColor}`);\n      const newGame = new Chess();\n      setGame(newGame);\n      setInitialFen(newGame.fen());\n      setMoveHistory([]);\n      setCurrentMoveIndex(-1);",
    content
)

# 6. handleMessage game_state
content = re.sub(
    r"const newGame = new Chess\(data\.fen\);\n\s*setGame\(newGame\);\n\s*setEvalScore\(data\.score \|\| 0\);\n\s*setMoveHistory\(\[\]\);\n\s*setGameStatus\(''\);",
    r"const newGame = new Chess(data.fen);\n          setGame(newGame);\n          setInitialFen(data.fen);\n          setEvalScore(data.score || 0);\n          setMoveHistory([]);\n          setCurrentMoveIndex(-1);\n          setGameStatus('');",
    content
)

# 7. handleMessage move_result
bestmove_block = """            if (data.bestmove) {
              const from = data.bestmove.substring(0, 2);
              const to = data.bestmove.substring(2, 4);
              const prom = data.bestmove[4];
              const tempGame = new Chess(gameRef.current.fen());
              let san = data.bestmove;
              try {
                const m = tempGame.move({ from, to, promotion: prom || 'q' });
                if (m) san = m.san;
              } catch(e) {}

              setMoveHistory(prev => {
                const idx = currentMoveIndexRef.current;
                const newHist = prev.slice(0, idx + 1);
                newHist.push({ san, fen: data.fen, from, to });
                setCurrentMoveIndex(newHist.length - 1);
                return newHist;
              });
            }"""
content = re.sub(
    r"if \(data\.bestmove\) \{\n\s*setMoveHistory\(\(prev\) => \[\.\.\.prev, data\.bestmove\]\);\n\s*\}",
    bestmove_block,
    content
)

# 8. executeMove
execute_move_block = """      // Add to move history
      const moveItem = { san: move.san, fen: gameCopy.fen(), from: move.from, to: move.to };
      setMoveHistory(prev => {
        const idx = currentMoveIndexRef.current;
        const newHist = prev.slice(0, idx + 1);
        newHist.push(moveItem);
        setCurrentMoveIndex(newHist.length - 1);
        return newHist;
      });"""
content = re.sub(
    r"// Add to move history\n\s*setMoveHistory\(\(prev\) => \[\.\.\.prev, move\.san\]\);",
    execute_move_block,
    content
)

# 9. finalSquareStyles
final_styles_add = """
    // Highlight last move
    if (currentMoveIndex >= 0 && moveHistory[currentMoveIndex]) {
      const lastMove = moveHistory[currentMoveIndex];
      if (lastMove.from && lastMove.to) {
        styles[lastMove.from] = { ...styles[lastMove.from], backgroundColor: 'rgba(255, 255, 0, 0.4)' };
        styles[lastMove.to] = { ...styles[lastMove.to], backgroundColor: 'rgba(255, 255, 0, 0.4)' };
      }
    }
"""
content = re.sub(
    r"(const finalSquareStyles = useMemo\(\(\) => \{\n\s*const styles = \{ \.\.\.legalMoveSquares \};)",
    r"\1" + final_styles_add,
    content
)
content = re.sub(
    r"return styles;\n\s*\}, \[legalMoveSquares, game\]\);",
    r"return styles;\n  }, [legalMoveSquares, game, currentMoveIndex, moveHistory]);",
    content
)

# 10. Replay controls UI
replay_controls = """
        {/* Replay Controls */}
        {moveHistory.length > 0 && (
          <div className="replay-controls" style={{ display: 'flex', gap: '8px', marginBottom: '16px', justifyContent: 'center' }}>
            <button 
              className="btn btn-secondary" 
              style={{ padding: '4px 8px', fontSize: '12px' }}
              disabled={currentMoveIndex === -1}
              onClick={() => jumpToMove(-1)}
              title="Go to start"
            >
              &lt;&lt;
            </button>
            <button 
              className="btn btn-secondary" 
              style={{ padding: '4px 12px', fontSize: '12px' }}
              disabled={currentMoveIndex === -1}
              onClick={() => jumpToMove(currentMoveIndex - 1)}
              title="Previous move"
            >
              &lt;
            </button>
            <button 
              className="btn btn-secondary" 
              style={{ padding: '4px 12px', fontSize: '12px' }}
              disabled={currentMoveIndex === moveHistory.length - 1}
              onClick={() => jumpToMove(currentMoveIndex + 1)}
              title="Next move"
            >
              &gt;
            </button>
            <button 
              className="btn btn-secondary" 
              style={{ padding: '4px 8px', fontSize: '12px' }}
              disabled={currentMoveIndex === moveHistory.length - 1}
              onClick={() => jumpToMove(moveHistory.length - 1)}
              title="Go to end"
            >
              &gt;&gt;
            </button>
          </div>
        )}
"""
content = re.sub(
    r'(<div className="controls">)',
    replay_controls + r'\n        \1',
    content
)

# 11. Move history rendering UI
history_render = """              {moveHistory.map((move, i) => (
                <span 
                  key={i} 
                  className={`move-item ${i % 2 === 0 ? 'white-move' : 'black-move'}`}
                  onClick={() => jumpToMove(i)}
                  style={{ 
                    cursor: 'pointer', 
                    backgroundColor: i === currentMoveIndex ? 'rgba(255,255,255,0.2)' : 'transparent', 
                    borderRadius: '4px', 
                    padding: '2px 4px' 
                  }}
                >
                  {i % 2 === 0 && <span className="move-number">{Math.floor(i / 2) + 1}.</span>}
                  {move.san}
                </span>
              ))}"""
content = re.sub(
    r"\{moveHistory\.map\(\(move, i\) => \([\s\S]*?\}\)\)",
    history_render,
    content
)

# 12. Handle isDraggablePiece to disable when looking at history
content = re.sub(
    r"if \(isThinking\) return false;",
    r"if (isThinking) return false;\n      if (currentMoveIndex !== moveHistory.length - 1) return false;",
    content
)
content = re.sub(
    r"\[game, gameMode, isThinking, boardOrientation, isP2PConnected\]",
    r"[game, gameMode, isThinking, boardOrientation, isP2PConnected, currentMoveIndex, moveHistory]",
    content
)

with open("frontend/src/App.jsx", "w", encoding="utf-8") as f:
    f.write(content)
