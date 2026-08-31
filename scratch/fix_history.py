import re

with open("frontend/src/App.jsx", "r", encoding="utf-8") as f:
    content = f.read()

# 1. Fix P2P onP2PMove
p2p_old = """      setMoveHistory((prev) => {
        const idx = currentMoveIndexRef.current;
        const newHist = prev.slice(0, idx + 1);
        newHist.push({ san: msg.move, fen: msg.fen, from: '', to: '' }); 
        setCurrentMoveIndex(newHist.length - 1);
        return newHist;
      });"""
p2p_new = """      setMoveHistory((prev) => {
        const newHist = [...prev, { san: msg.move, fen: msg.fen, from: '', to: '' }]; 
        setCurrentMoveIndex(newHist.length - 1);
        return newHist;
      });"""
content = content.replace(p2p_old, p2p_new)


# 2. Fix Engine Move handleMessage
engine_old = """            if (data.bestmove) {
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
engine_new = """            if (data.bestmove) {
              const from = data.bestmove.substring(0, 2);
              const to = data.bestmove.substring(2, 4);
              const prom = data.bestmove[4];

              setMoveHistory(prev => {
                const lastFen = prev.length > 0 ? prev[prev.length - 1].fen : initialFen;
                const tempGame = new Chess(lastFen);
                let san = data.bestmove;
                try {
                  const m = tempGame.move({ from, to, promotion: prom || 'q' });
                  if (m) san = m.san;
                } catch(e) {}

                const newHist = [...prev, { san, fen: data.fen, from, to }];
                setCurrentMoveIndex(newHist.length - 1);
                return newHist;
              });
            }"""
content = content.replace(engine_old, engine_new)

# 3. Ensure setGame is NOT called if we were viewing history during an engine/P2P move?
# Actually, if we ALWAYS want to snap the user back to present when the engine moves, we SHOULD call setGame(newGame).
# And my changes above just ensure currentMoveIndex updates to the present and history doesn't truncate.
# Wait! In handleMessage, we have:
# const newGame = new Chess(data.fen);
# setGame(newGame);
# This is correct for snapping the user back to present!
# The problem was ONLY the history truncation!

with open("frontend/src/App.jsx", "w", encoding="utf-8") as f:
    f.write(content)
