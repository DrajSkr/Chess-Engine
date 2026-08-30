import { useState, useEffect, useCallback } from 'react';

const WS_URL = import.meta.env.VITE_WS_URL || `ws://${window.location.hostname}:8080`;

let wsInstance = null;
let reconnectTimer = null;
const listeners = new Set();
let isThinkingGlobal = false;
let isConnectedGlobal = false;
const connectionListeners = new Set();

function notifyConnectionChange(status) {
  isConnectedGlobal = status;
  connectionListeners.forEach(l => l(status));
}

function connect() {
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }
  if (wsInstance) {
    wsInstance.close();
  }

  try {
    const ws = new WebSocket(WS_URL);
    wsInstance = ws;

    ws.onopen = () => {
      console.log('[WS] Connected to engine server');
      notifyConnectionChange(true);
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        console.log('[WS] Received:', data);
        
        if (data.type === 'move_result' || data.type === 'error' || data.type === 'game_over') {
          isThinkingGlobal = false;
        }

        // Notify all registered listeners
        listeners.forEach(listener => listener(data));
      } catch (err) {
        console.error('[WS] Failed to parse message:', event.data, err);
      }
    };

    ws.onclose = () => {
      console.log('[WS] Disconnected from engine server');
      notifyConnectionChange(false);
      wsInstance = null;
      reconnectTimer = setTimeout(connect, 2000);
    };

    ws.onerror = (err) => {
      console.error('[WS] Error:', err);
    };
  } catch (err) {
    console.error('[WS] Connection failed:', err);
    reconnectTimer = setTimeout(connect, 2000);
  }
}

// Initial connect
connect();

export function useEngineWebSocket() {
  const [isConnected, setIsConnected] = useState(isConnectedGlobal);
  const [isThinking, setIsThinking] = useState(isThinkingGlobal);

  useEffect(() => {
    const handler = (status) => setIsConnected(status);
    connectionListeners.add(handler);
    return () => connectionListeners.delete(handler);
  }, []);

  const addMessageListener = useCallback((listener) => {
    listeners.add(listener);
    return () => listeners.delete(listener);
  }, []);

  const sendMove = useCallback((moveStr, fenStr) => {
    if (wsInstance && wsInstance.readyState === WebSocket.OPEN) {
      const msg = JSON.stringify({ type: 'move', move: moveStr, fen: fenStr });
      console.log('[WS] Sending:', msg);
      isThinkingGlobal = true;
      setIsThinking(true);
      wsInstance.send(msg);
      return true;
    }
    console.warn('[WS] Cannot send move - not connected');
    return false;
  }, []);

  const sendNewGame = useCallback(() => {
    if (wsInstance && wsInstance.readyState === WebSocket.OPEN) {
      const msg = JSON.stringify({ type: 'new_game' });
      console.log('[WS] Sending:', msg);
      isThinkingGlobal = false;
      setIsThinking(false);
      wsInstance.send(msg);
      return true;
    }
    return false;
  }, []);

  const sendCreateRoom = useCallback(() => {
    if (wsInstance && wsInstance.readyState === WebSocket.OPEN) {
      wsInstance.send(JSON.stringify({ type: 'create_room' }));
      return true;
    }
    return false;
  }, []);

  const sendJoinRoom = useCallback((roomId) => {
    if (wsInstance && wsInstance.readyState === WebSocket.OPEN) {
      wsInstance.send(JSON.stringify({ type: 'join_room', roomId }));
      return true;
    }
    return false;
  }, []);

  const sendSignal = useCallback((roomId, payload) => {
    if (wsInstance && wsInstance.readyState === WebSocket.OPEN) {
      wsInstance.send(JSON.stringify({ type: 'signal', roomId, ...payload }));
      return true;
    }
    return false;
  }, []);

  return {
    isConnected,
    isThinking,
    addMessageListener,
    sendMove,
    sendNewGame,
    sendCreateRoom,
    sendJoinRoom,
    sendSignal,
    reconnect: connect,
  };
}
