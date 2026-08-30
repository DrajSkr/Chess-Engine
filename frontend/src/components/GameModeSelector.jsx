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
        <button
          key={mode.id}
          className={`mode-card ${currentMode === mode.id ? 'active' : ''} ${!mode.available ? 'disabled' : ''}`}
          onClick={() => mode.available && onSelectMode(mode.id)}
          disabled={!mode.available}
        >
          <span className="mode-icon">{mode.icon}</span>
          <span className="mode-title">{mode.title}</span>
          <span className="mode-desc">{mode.description}</span>
          {mode.badge && <span className="mode-badge">{mode.badge}</span>}
        </button>
      ))}
    </div>
  );
}
