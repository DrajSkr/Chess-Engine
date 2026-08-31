/**
 * Game Mode Selector — 3-card mode selection UI.
 * Modes: LOCAL_PVP, VS_BOT, P2P (stub).
 */
export default function GameModeSelector({ currentMode, onSelectMode, isConnected }) {
  const modes = [
    {
      id: 'LOCAL_PVP',
      title: 'Local PvP',
      icon: '👥',
      description: 'Play against a friend on the same device',
      available: true,
    },
    {
      id: 'VS_BOT',
      title: 'Vs Engine',
      icon: '🤖',
      description: 'Challenge the Domino chess engine',
      available: isConnected,
      badge: !isConnected ? 'Server Offline' : null,
    },
    {
      id: 'P2P',
      title: 'Online P2P',
      icon: '🌐',
      description: 'Play against others online',
      available: isConnected,
      badge: !isConnected ? 'Server Offline' : null,
    },
  ];

  return (
    <div className="mode-selector">
      {modes.map((mode) => (
        <div key={mode.id} className="mode-card-wrapper">
          <button
            className={`mode-card ${currentMode === mode.id ? 'active' : ''} ${!mode.available ? 'disabled' : ''} ${mode.id === 'VS_BOT' ? 'no-hover' : ''}`}
            onClick={(e) => {
              if (mode.id !== 'VS_BOT' && mode.available) {
                onSelectMode(mode.id);
              }
            }}
            disabled={!mode.available}
            style={{ cursor: mode.id === 'VS_BOT' ? 'default' : 'pointer', paddingBottom: mode.id === 'VS_BOT' ? '10px' : '' }}
          >
            <span className="mode-icon">{mode.icon}</span>
            <span className="mode-title">{mode.title}</span>
            <span className="mode-desc">{mode.description}</span>
            {mode.badge && <span className="mode-badge">{mode.badge}</span>}
            
            {/* Color Selector specifically for VS_BOT mode always visible */}
            {mode.id === 'VS_BOT' && mode.available && (
              <div className="color-selector" style={{display: 'flex', gap: '8px', marginTop: '15px', justifyContent: 'center', width: '100%'}}>
                 <button 
                    onClick={(e) => { e.stopPropagation(); mode.available && onSelectMode(mode.id, 'w'); }} 
                    style={{flex: 1, padding: '8px 5px', borderRadius: '4px', background: '#f0d9b5', color: 'black', border: '1px solid rgba(255,255,255,0.1)', cursor: 'pointer', fontWeight: 'bold', fontSize: '0.9em'}}
                    title="Play as White"
                 >
                    White
                 </button>
                 <button 
                    onClick={(e) => { 
                      e.stopPropagation(); 
                      const randomColor = Math.random() < 0.5 ? 'w' : 'b';
                      mode.available && onSelectMode(mode.id, randomColor); 
                    }} 
                    style={{flex: 1, padding: '8px 5px', borderRadius: '4px', background: 'linear-gradient(135deg, #f0d9b5 50%, #b58863 50%)', color: 'white', textShadow: '1px 1px 2px black', border: '1px solid rgba(255,255,255,0.1)', cursor: 'pointer', fontWeight: 'bold', fontSize: '0.9em'}}
                    title="Random Color"
                 >
                    Random
                 </button>
                 <button 
                    onClick={(e) => { e.stopPropagation(); mode.available && onSelectMode(mode.id, 'b'); }} 
                    style={{flex: 1, padding: '8px 5px', borderRadius: '4px', background: '#b58863', color: 'white', border: '1px solid rgba(255,255,255,0.1)', cursor: 'pointer', fontWeight: 'bold', fontSize: '0.9em'}}
                    title="Play as Black"
                 >
                    Black
                 </button>
              </div>
            )}
          </button>
        </div>
      ))}
    </div>
  );
}
