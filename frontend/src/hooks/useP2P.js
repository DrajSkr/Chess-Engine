import { useState, useEffect, useRef, useCallback } from 'react';
import { useEngineWebSocket } from './useEngineWebSocket';

const rtcConfig = {
  iceServers: [
    { urls: 'stun:stun.l.google.com:19302' },
    { urls: 'stun:stun1.l.google.com:19302' },
  ],
};

export function useP2P({ onP2PMove, onOpponentLeft }) {
  const { isConnected, lastMessage, sendCreateRoom, sendJoinRoom, sendSignal } = useEngineWebSocket();
  
  const [roomCode, setRoomCode] = useState(null);
  const [isInRoom, setIsInRoom] = useState(false);
  const [isP2PConnected, setIsP2PConnected] = useState(false);
  const [playerColor, setPlayerColor] = useState(null);

  const pcRef = useRef(null);
  const dcRef = useRef(null);

  const cleanup = useCallback(() => {
    if (dcRef.current) {
      dcRef.current.close();
      dcRef.current = null;
    }
    if (pcRef.current) {
      pcRef.current.close();
      pcRef.current = null;
    }
    setIsP2PConnected(false);
    setIsInRoom(false);
    setRoomCode(null);
    setPlayerColor(null);
  }, []);

  const initPeerConnection = useCallback((roomId) => {
    const pc = new RTCPeerConnection(rtcConfig);
    pcRef.current = pc;

    pc.onicecandidate = (event) => {
      if (event.candidate) {
        sendSignal(roomId, { signalType: 'ice', candidate: event.candidate });
      }
    };

    pc.onconnectionstatechange = () => {
      console.log('[WebRTC] Connection state:', pc.connectionState);
      if (pc.connectionState === 'connected') {
        setIsP2PConnected(true);
      } else if (pc.connectionState === 'disconnected' || pc.connectionState === 'failed' || pc.connectionState === 'closed') {
        setIsP2PConnected(false);
        onOpponentLeft && onOpponentLeft();
        cleanup();
      }
    };

    pc.ondatachannel = (event) => {
      const receiveChannel = event.channel;
      dcRef.current = receiveChannel;
      setupDataChannel(receiveChannel);
    };

    return pc;
  }, [sendSignal, onOpponentLeft, cleanup]);

  const setupDataChannel = (channel) => {
    channel.onopen = () => console.log('[WebRTC] Data channel opened');
    channel.onclose = () => console.log('[WebRTC] Data channel closed');
    channel.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        if (msg.type === 'MOVE') {
          onP2PMove && onP2PMove(msg);
        }
      } catch (err) {
        console.error('Failed to parse P2P message', err);
      }
    };
  };

  useEffect(() => {
    if (!lastMessage) return;

    const handleSignaling = async () => {
      try {
        if (lastMessage.type === 'room_created') {
          setRoomCode(lastMessage.roomId);
        } else if (lastMessage.type === 'p2p_start') {
          setIsInRoom(true);
          setPlayerColor(lastMessage.color === 'w' ? 'white' : 'black');
          const roomId = lastMessage.roomId || roomCode;
          setRoomCode(roomId);

          const pc = initPeerConnection(roomId);
          
          if (lastMessage.color === 'w') {
            const dc = pc.createDataChannel('chess');
            dcRef.current = dc;
            setupDataChannel(dc);

            const offer = await pc.createOffer();
            await pc.setLocalDescription(offer);
            sendSignal(roomId, { signalType: 'offer', sdp: pc.localDescription });
          }
        } else if (lastMessage.type === 'signal') {
          const pc = pcRef.current;
          if (!pc) return;

          if (lastMessage.signalType === 'offer') {
            await pc.setRemoteDescription(new RTCSessionDescription(lastMessage.sdp));
            const answer = await pc.createAnswer();
            await pc.setLocalDescription(answer);
            sendSignal(lastMessage.roomId, { signalType: 'answer', sdp: pc.localDescription });
          } else if (lastMessage.signalType === 'answer') {
            await pc.setRemoteDescription(new RTCSessionDescription(lastMessage.sdp));
          } else if (lastMessage.signalType === 'ice') {
            await pc.addIceCandidate(new RTCIceCandidate(lastMessage.candidate));
          }
        } else if (lastMessage.type === 'opponent_left') {
          onOpponentLeft && onOpponentLeft();
          cleanup();
        }
      } catch (err) {
        console.error('[WebRTC] Signaling error:', err);
      }
    };

    handleSignaling();
  }, [lastMessage, roomCode, initPeerConnection, sendSignal, onOpponentLeft, cleanup]);

  const sendP2PMove = useCallback((moveStr, fenStr) => {
    if (dcRef.current && dcRef.current.readyState === 'open') {
      dcRef.current.send(JSON.stringify({ type: 'MOVE', move: moveStr, fen: fenStr }));
      return true;
    }
    return false;
  }, []);

  const createRoom = useCallback(() => {
    sendCreateRoom();
  }, [sendCreateRoom]);

  const joinRoom = useCallback((code) => {
    setRoomCode(code);
    sendJoinRoom(code);
  }, [sendJoinRoom]);

  const leaveRoom = useCallback(() => {
    cleanup();
  }, [cleanup]);

  return {
    isServerConnected: isConnected,
    isP2PConnected,
    isInRoom,
    roomCode,
    playerColor,
    createRoom,
    joinRoom,
    leaveRoom,
    sendP2PMove,
  };
}
