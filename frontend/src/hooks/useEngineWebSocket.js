import { useState, useEffect, useCallback, useRef } from 'react';

const WS_URL = import.meta.env.VITE_WS_URL || `ws://${window.location.hostname}:8080`;

/**
 * Custom hook for managing WebSocket connection to the C++ engine backend.
 * Handles connection lifecycle, auto-reconnect, and message parsing.
 */
export function useEngineWebSocket() {
  const [isConnected, setIsConnected] = useState(false);
  const [lastMessage, setLastMessage] = useState(null);
  const [isThinking, setIsThinking] = useState(false);
  const wsRef = useRef(null);
  const reconnectTimerRef = useRef(null);

  const connect = useCallback(() => {
    // Clear any existing reconnect timer
    if (reconnectTimerRef.current) {
      clearTimeout(reconnectTimerRef.current);
      reconnectTimerRef.current = null;
    }

    // Close existing connection
    if (wsRef.current) {
      wsRef.current.close();
    }

    try {
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;

      ws.onopen = () => {
        console.log('[WS] Connected to engine server');
        setIsConnected(true);
      };

      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          console.log('[WS] Received:', data);
          setLastMessage(data);
          // Clear thinking state when we get a response
          if (data.type === 'move_result' || data.type === 'error' || data.type === 'game_over') {
            setIsThinking(false);
          }
        } catch (err) {
          console.error('[WS] Failed to parse message:', event.data, err);
        }
      };

      ws.onclose = () => {
        console.log('[WS] Disconnected from engine server');
        setIsConnected(false);
        wsRef.current = null;
        // Auto-reconnect after 2 seconds
        reconnectTimerRef.current = setTimeout(connect, 2000);
      };

      ws.onerror = (err) => {
        console.error('[WS] Error:', err);
      };
    } catch (err) {
      console.error('[WS] Connection failed:', err);
      reconnectTimerRef.current = setTimeout(connect, 2000);
    }
  }, []);

  // Connect on mount, cleanup on unmount
  useEffect(() => {
    connect();
    return () => {
      if (reconnectTimerRef.current) {
        clearTimeout(reconnectTimerRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connect]);

  const sendMove = useCallback((moveStr) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      const msg = JSON.stringify({ type: 'move', move: moveStr });
      console.log('[WS] Sending:', msg);
      setIsThinking(true);
      wsRef.current.send(msg);
      return true;
    }
    console.warn('[WS] Cannot send move - not connected');
    return false;
  }, []);

  const sendNewGame = useCallback(() => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      const msg = JSON.stringify({ type: 'new_game' });
      console.log('[WS] Sending:', msg);
      setIsThinking(false);
      wsRef.current.send(msg);
      return true;
    }
    return false;
  }, []);

  const sendCreateRoom = useCallback(() => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: 'create_room' }));
      return true;
    }
    return false;
  }, []);

  const sendJoinRoom = useCallback((roomId) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: 'join_room', roomId }));
      return true;
    }
    return false;
  }, []);

  const sendSignal = useCallback((roomId, payload) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: 'signal', roomId, ...payload }));
      return true;
    }
    return false;
  }, []);

  return {
    isConnected,
    isThinking,
    lastMessage,
    sendMove,
    sendNewGame,
    sendCreateRoom,
    sendJoinRoom,
    sendSignal,
    reconnect: connect,
  };
}
