import re

with open("frontend/src/App.jsx", "r", encoding="utf-8") as f:
    content = f.read()

# 1. Add Sound Logic
sound_logic = """
// Sound objects
const sounds = {
  move: new Audio('/sounds/move.ogg'),
  capture: new Audio('/sounds/capture.ogg'),
  check: new Audio('/sounds/check.ogg'),
  gameEnd: new Audio('/sounds/genericnotify.ogg'),
};

const playSoundForMove = (san, isGameOver) => {
  try {
    if (isGameOver) {
      sounds.gameEnd.currentTime = 0;
      sounds.gameEnd.play().catch(() => {});
    } else if (san.includes('+') || san.includes('#')) {
      sounds.check.currentTime = 0;
      sounds.check.play().catch(() => {});
    } else if (san.includes('x')) {
      sounds.capture.currentTime = 0;
      sounds.capture.play().catch(() => {});
    } else {
      sounds.move.currentTime = 0;
      sounds.move.play().catch(() => {});
    }
  } catch (e) {}
};
"""

content = re.sub(
    r"(const MODES = \{)",
    sound_logic + r"\n\1",
    content
)

# 2. Add playSoundForMove in P2P onP2PMove
p2p_move_hook = """      if (newGame.isCheckmate()) setGameStatus('Checkmate! You lose.');
      else if (newGame.isDraw() || newGame.isStalemate()) setGameStatus('Draw!');
      else if (newGame.isCheck()) setGameStatus('Check!');
      else setGameStatus('');
      
      playSoundForMove(msg.move, newGame.isGameOver());
"""
content = re.sub(
    r"(if \(newGame\.isCheckmate\(\)\) setGameStatus\('Checkmate! You lose\.'\);\s*else if \(newGame\.isDraw\(\) \|\| newGame\.isStalemate\(\)\) setGameStatus\('Draw!'\);\s*else if \(newGame\.isCheck\(\)\) setGameStatus\('Check!'\);\s*else setGameStatus\(''\);)",
    p2p_move_hook,
    content
)

# 3. Add playSoundForMove in engine handleMessage move_result
engine_move_hook = """                const newHist = [...prev, { san, fen: data.fen, from, to }];
                setCurrentMoveIndex(newHist.length - 1);
                
                // Play sound
                setTimeout(() => playSoundForMove(san, newGame.isGameOver()), 0);
                
                return newHist;"""
content = re.sub(
    r"(const newHist = \[\.\.\.prev, \{ san, fen: data\.fen, from, to \}\];\s*setCurrentMoveIndex\(newHist\.length - 1\);\s*return newHist;)",
    engine_move_hook,
    content
)

# 4. Add playSoundForMove in user executeMove
execute_move_hook = """      setMoveHistory(prev => {
        const idx = currentMoveIndexRef.current;
        const newHist = prev.slice(0, idx + 1);
        newHist.push(moveItem);
        setCurrentMoveIndex(newHist.length - 1);
        
        playSoundForMove(move.san, gameCopy.isGameOver());
        
        return newHist;
      });"""
content = re.sub(
    r"(setMoveHistory\(prev => \{\s*const idx = currentMoveIndexRef\.current;\s*const newHist = prev\.slice\(0, idx \+ 1\);\s*newHist\.push\(moveItem\);\s*setCurrentMoveIndex\(newHist\.length - 1\);\s*return newHist;\s*\}\);)",
    execute_move_hook,
    content
)

with open("frontend/src/App.jsx", "w", encoding="utf-8") as f:
    f.write(content)
