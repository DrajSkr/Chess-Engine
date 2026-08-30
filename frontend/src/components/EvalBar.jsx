import { useMemo } from 'react';

/**
 * Evaluation Bar — vertical bar showing engine evaluation.
 * White fills from bottom, black from top.
 * Score is in centipawns, mapped to a visual percentage via sigmoid.
 */
export default function EvalBar({ score = 0, flipped = false }) {
  // Map centipawns to a visual percentage using a sigmoid-like curve
  // Clamp between 5% and 95% so the bar is never fully empty/full
  const whitePercent = useMemo(() => {
    // Sigmoid mapping: 50 + (score / max) * 50
    // Using 500cp as the "fully winning" threshold
    const mapped = 50 + (score / 500) * 50;
    return Math.max(5, Math.min(95, mapped));
  }, [score]);

  const displayScore = useMemo(() => {
    const abs = Math.abs(score);
    if (abs >= 49000) return score > 0 ? 'M' : '-M'; // Mate
    if (abs >= 100) return (score / 100).toFixed(1);
    return (score / 100).toFixed(2);
  }, [score]);

  return (
    <div className="eval-bar-container" style={{ transform: flipped ? 'rotate(180deg)' : 'none' }}>
      <div className="eval-bar">
        {/* Black section (top) */}
        <div
          className="eval-bar-black"
          style={{ height: `${100 - whitePercent}%` }}
        />
        {/* White section (bottom) */}
        <div
          className="eval-bar-white"
          style={{ height: `${whitePercent}%` }}
        />
      </div>
      <div className="eval-bar-score" style={{ transform: flipped ? 'rotate(180deg)' : 'none' }}>
        {displayScore}
      </div>
    </div>
  );
}
