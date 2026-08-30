/*##########################

WebSocket Server for Domino Chess Engine
Thin adapter layer using raw Winsock2 WebSocket.
No external library dependencies (no Boost, no websocketpp).

Protocol (JSON over WebSocket):
  Client -> Server:
    {"type":"move","move":"e2e4"}
    {"type":"new_game"}
  Server -> Client:
    {"type":"game_state","fen":"...","score":0}
    {"type":"move_result","fen":"...","bestmove":"e7e5","score":15,"valid":true}
    {"type":"error","message":"Illegal move"}
    {"type":"game_over","fen":"...","bestmove":"","score":0,"valid":true}

#############################*/

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include <iostream>
#include <string>
#include "SearchCapture.hpp"
#include "FenExport.hpp"
#include "MagicGenerator.hpp"
#include "Zobrist.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <ctime>

// Engine includes (same order as main.cpp — DO NOT modify these files)
#include "config.hpp"
#include "Board.hpp"
#include "MoveGenerator.hpp"
#include "UCI.hpp"

// Our new utilities (read-only, no engine modifications)
#include "FenExport.hpp"
#include "SearchCapture.hpp"

#ifdef _WIN32
// Global critical section to protect engine state during search
CRITICAL_SECTION engine_cs;
// P2P Room state
CRITICAL_SECTION rooms_cs;
#else
#include <mutex>
#include <thread>
std::mutex engine_cs;
std::mutex rooms_cs;
#endif

struct Room {
    std::string roomId;
    SOCKET player1;   // first to join
    SOCKET player2;   // second to join
    Room() : player1(INVALID_SOCKET), player2(INVALID_SOCKET) {}
};
std::map<std::string, Room> rooms;

// Generate a random 6-character alphanumeric room code
static std::string generate_room_code()
{
    static const char alphanum[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::string code;
    for (int i = 0; i < 6; i++)
    {
        code += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return code;
}

// Find room containing a specific socket
static std::string find_room_for_socket(SOCKET s)
{
    for (auto& kv : rooms)
    {
        if (kv.second.player1 == s || kv.second.player2 == s)
            return kv.first;
    }
    return "";
}

#ifdef _WIN32
// RAII wrapper for CRITICAL_SECTION
struct CSLock {
    CRITICAL_SECTION& cs;
    CSLock(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
    ~CSLock() { LeaveCriticalSection(&cs); }
};
#else
// RAII wrapper for mutex
struct CSLock {
    std::lock_guard<std::mutex> lock;
    CSLock(std::mutex& m) : lock(m) {}
};
#endif

// Search depth for bot
static const int SEARCH_DEPTH = 64;

/*##########################
  Base64 encoding for WebSocket handshake
#############################*/

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const unsigned char* data, size_t len)
{
    std::string result;
    result.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        unsigned int n = ((unsigned int)data[i]) << 16;
        if (i + 1 < len) n |= ((unsigned int)data[i + 1]) << 8;
        if (i + 2 < len) n |= (unsigned int)data[i + 2];
        result += b64_table[(n >> 18) & 0x3F];
        result += b64_table[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? b64_table[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? b64_table[n & 0x3F] : '=';
    }
    return result;
}

/*##########################
  SHA-1 (minimal, for WebSocket handshake only)
#############################*/

static void sha1(const unsigned char* data, size_t len, unsigned char hash[20])
{
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    size_t msg_len = len;
    size_t total_len = ((msg_len + 8) / 64 + 1) * 64;
    std::vector<unsigned char> msg(total_len, 0);
    memcpy(msg.data(), data, len);
    msg[len] = 0x80;
    uint64_t bit_len = (uint64_t)msg_len * 8;
    for (int i = 0; i < 8; i++)
        msg[total_len - 1 - i] = (unsigned char)(bit_len >> (i * 8));

    for (size_t chunk = 0; chunk < total_len; chunk += 64)
    {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = ((uint32_t)msg[chunk + i * 4] << 24) |
                   ((uint32_t)msg[chunk + i * 4 + 1] << 16) |
                   ((uint32_t)msg[chunk + i * 4 + 2] << 8) |
                   ((uint32_t)msg[chunk + i * 4 + 3]);
        for (int i = 16; i < 80; i++)
        {
            uint32_t t = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (t << 1) | (t >> 31);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; i++)
        {
            uint32_t f, k_val;
            if (i < 20) { f = (b & c) | ((~b) & d); k_val = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k_val = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k_val = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k_val = 0xCA62C1D6; }
            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k_val + w[i];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }

    uint32_t hh[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; i++)
    {
        hash[i * 4] = (unsigned char)(hh[i] >> 24);
        hash[i * 4 + 1] = (unsigned char)(hh[i] >> 16);
        hash[i * 4 + 2] = (unsigned char)(hh[i] >> 8);
        hash[i * 4 + 3] = (unsigned char)(hh[i]);
    }
}

/*##########################
  Minimal JSON helpers
#############################*/

static std::string json_get_string(const std::string& json, const std::string& key)
{
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return "";
    size_t colon_pos = json.find(':', key_pos + search_key.size());
    if (colon_pos == std::string::npos) return "";
    size_t val_start = json.find('"', colon_pos + 1);
    if (val_start == std::string::npos) return "";
    val_start++;
    size_t val_end = json.find('"', val_start);
    if (val_end == std::string::npos) return "";
    return json.substr(val_start, val_end - val_start);
}

static std::string json_game_state(const std::string& fen, int score)
{
    std::ostringstream ss;
    ss << "{\"type\":\"game_state\",\"fen\":\"" << fen << "\",\"score\":" << score << "}";
    return ss.str();
}

static std::string json_move_result(const std::string& fen, const std::string& bestmove, int score, bool valid)
{
    std::ostringstream ss;
    ss << "{\"type\":\"move_result\",\"fen\":\"" << fen
       << "\",\"bestmove\":\"" << bestmove
       << "\",\"score\":" << score
       << ",\"valid\":" << (valid ? "true" : "false") << "}";
    return ss.str();
}

static std::string json_error(const std::string& message)
{
    std::ostringstream ss;
    ss << "{\"type\":\"error\",\"message\":\"" << message << "\"}";
    return ss.str();
}

/*##########################
  WebSocket frame helpers
#############################*/

// Read a complete WebSocket frame from socket, returns the payload text.
// Returns empty string on connection close or error.
static std::string ws_read_frame(SOCKET sock)
{
    unsigned char header[2];
    int r = recv(sock, (char*)header, 2, 0);
    if (r <= 0) return "";

    // int fin = (header[0] >> 7) & 1;   // Unused but kept for protocol completeness
    int opcode = header[0] & 0x0F;
    if (opcode == 0x8) return ""; // close frame
    
    int masked = (header[1] >> 7) & 1;
    uint64_t payload_len = header[1] & 0x7F;

    if (payload_len == 126)
    {
        unsigned char ext[2];
        recv(sock, (char*)ext, 2, 0);
        payload_len = ((uint64_t)ext[0] << 8) | ext[1];
    }
    else if (payload_len == 127)
    {
        unsigned char ext[8];
        recv(sock, (char*)ext, 8, 0);
        payload_len = 0;
        for (int i = 0; i < 8; i++)
            payload_len = (payload_len << 8) | ext[i];
    }

    unsigned char mask_key[4] = {0};
    if (masked)
        recv(sock, (char*)mask_key, 4, 0);

    std::string payload(payload_len, '\0');
    size_t received = 0;
    while (received < payload_len)
    {
        r = recv(sock, &payload[received], (int)(payload_len - received), 0);
        if (r <= 0) return "";
        received += r;
    }

    if (masked)
    {
        for (size_t i = 0; i < payload_len; i++)
            payload[i] ^= mask_key[i % 4];
    }

    return payload;
}

// Send a WebSocket text frame
static bool ws_send_frame(SOCKET sock, const std::string& data)
{
    std::vector<unsigned char> frame;
    frame.push_back(0x81); // FIN + text opcode

    if (data.size() <= 125)
    {
        frame.push_back((unsigned char)data.size());
    }
    else if (data.size() <= 65535)
    {
        frame.push_back(126);
        frame.push_back((unsigned char)(data.size() >> 8));
        frame.push_back((unsigned char)(data.size() & 0xFF));
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--)
            frame.push_back((unsigned char)(data.size() >> (i * 8)));
    }

    frame.insert(frame.end(), data.begin(), data.end());
    int total = (int)frame.size();
    int sent = 0;
    while (sent < total)
    {
        int r = send(sock, (const char*)&frame[sent], total - sent, 0);
        if (r <= 0) return false;
        sent += r;
    }
    return true;
}

/*##########################
  WebSocket HTTP handshake
#############################*/

static bool ws_handshake(SOCKET client_sock)
{
    // Read HTTP upgrade request
    char buf[4096];
    int r = recv(client_sock, buf, sizeof(buf) - 1, 0);
    if (r <= 0) return false;
    buf[r] = '\0';
    std::string request(buf);

    // Only log if it's a real request
    std::cout << "[WS] Client connected. Performing handshake..." << std::endl;

    // Extract Sec-WebSocket-Key
    std::string key_header = "Sec-WebSocket-Key: ";
    size_t key_pos = request.find(key_header);
    
    // If no WebSocket key is found, treat it as a health check or regular HTTP request
    if (key_pos == std::string::npos) 
    {
        if (request.find("GET /healthz") != std::string::npos || request.find("GET / ") != std::string::npos)
        {
            std::cout << "[WS] Health check OK." << std::endl;
            std::string ok = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
            send(client_sock, ok.c_str(), (int)ok.size(), 0);
        }
        return false; // Close connection
    }

    size_t key_start = key_pos + key_header.size();
    size_t key_end = request.find("\r\n", key_start);
    std::string ws_key = request.substr(key_start, key_end - key_start);

    // Compute accept key: SHA1(key + magic_guid) → base64
    std::string accept_input = ws_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char sha1_hash[20];
    sha1((const unsigned char*)accept_input.c_str(), accept_input.size(), sha1_hash);
    std::string accept_key = base64_encode(sha1_hash, 20);

    // Send HTTP 101 response
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << accept_key << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "\r\n";
    std::string resp_str = response.str();
    send(client_sock, resp_str.c_str(), (int)resp_str.size(), 0);
    return true;
}

/*##########################
  Engine initialization
  (mirrors main.cpp init_everything — engine core untouched)
#############################*/

void init_engine()
{
    init_leapers_attacks();
    init_sliding_attacks();
    init_magic_numbers();
    
    //init char_pieces
    for (int i=0;i<128;i++)
        char_pieces[i] = -1;
        
    char_pieces['P'] = P;
    char_pieces['N'] = N;
    char_pieces['B'] = B;
    char_pieces['R'] = R;
    char_pieces['Q'] = Q;
    char_pieces['K'] = K;
    char_pieces['p'] = p;
    char_pieces['n'] = n;
    char_pieces['b'] = b;
    char_pieces['r'] = r;
    char_pieces['q'] = q;
    char_pieces['k'] = k;
    promoted_pieces[Q] = 'q';
    promoted_pieces[R] = 'r';
    promoted_pieces[B] = 'b';
    promoted_pieces[N] = 'n';
    promoted_pieces[q] = 'q';
    promoted_pieces[r] = 'r';
    promoted_pieces[b] = 'b';
    promoted_pieces[n] = 'n';
    
    // Init TT
    init_zobrist();
    init_tt(16); // 16 MB TT default
}

/*##########################
  Handle a single client connection
#############################*/

void handle_client(SOCKET client_sock)
{

    if (!ws_handshake(client_sock))
    {
        std::cout << "[WS] Handshake failed." << std::endl;
        closesocket(client_sock);
        return;
    }

    std::cout << "[WS] Handshake successful." << std::endl;

    // Send initial game state
    {
        CSLock lock(engine_cs);
        std::string fen = export_fen();
        std::string response = json_game_state(fen, 0);
        std::cout << "[WS] Sending initial state: " << response << std::endl;
        ws_send_frame(client_sock, response);
    }

    // Message loop
    while (true)
    {
        std::string payload = ws_read_frame(client_sock);
        if (payload.empty())
        {
            std::cout << "[WS] Client disconnected." << std::endl;
            break;
        }

        std::cout << "[WS] Received: " << payload << std::endl;

        std::string msg_type = json_get_string(payload, "type");

        if (msg_type == "new_game")
        {
            CSLock lock(engine_cs);
            parse_FEN_string(start_position);
            std::string fen = export_fen();
            std::string response = json_game_state(fen, 0);
            std::cout << "[WS] New game. Sending: " << response << std::endl;
            ws_send_frame(client_sock, response);
        }
        else if (msg_type == "create_room")
        {
#ifdef _WIN32
            EnterCriticalSection(&rooms_cs);
#else
            rooms_cs.lock();
#endif
            // Generate a unique room code
            std::string code;
            do {
                code = generate_room_code();
            } while (rooms.find(code) != rooms.end());

            Room room;
            room.roomId = code;
            room.player1 = client_sock;
            room.player2 = INVALID_SOCKET;
            rooms[code] = room;
#ifdef _WIN32
            LeaveCriticalSection(&rooms_cs);
#else
            rooms_cs.unlock();
#endif

            std::ostringstream response;
            response << "{\"type\":\"room_created\",\"roomId\":\"" << code << "\"}";
            ws_send_frame(client_sock, response.str());
            std::cout << "[WS] Room created: " << code << std::endl;
        }
        else if (msg_type == "join_room")
        {
            std::string code = json_get_string(payload, "roomId");
#ifdef _WIN32
            EnterCriticalSection(&rooms_cs);
#else
            rooms_cs.lock();
#endif
            auto it = rooms.find(code);
            if (it == rooms.end())
            {
    #ifdef _WIN32
            LeaveCriticalSection(&rooms_cs);
#else
            rooms_cs.unlock();
#endif
                ws_send_frame(client_sock, json_error("Room not found: " + code));
                continue;
            }
            if (it->second.player2 != INVALID_SOCKET)
            {
    #ifdef _WIN32
            LeaveCriticalSection(&rooms_cs);
#else
            rooms_cs.unlock();
#endif
                ws_send_frame(client_sock, json_error("Room is full: " + code));
                continue;
            }
            it->second.player2 = client_sock;
            SOCKET s1 = it->second.player1;
            SOCKET s2 = it->second.player2;

            // Randomly assign colors
            bool hostIsWhite = (rand() % 2 == 0);
            std::string c1 = hostIsWhite ? "w" : "b";
            std::string c2 = hostIsWhite ? "b" : "w";
#ifdef _WIN32
            LeaveCriticalSection(&rooms_cs);
#else
            rooms_cs.unlock();
#endif

            std::ostringstream r1, r2;
            r1 << "{\"type\":\"p2p_start\",\"color\":\"" << c1 << "\",\"roomId\":\"" << code << "\"}";
            r2 << "{\"type\":\"p2p_start\",\"color\":\"" << c2 << "\",\"roomId\":\"" << code << "\"}";
            ws_send_frame(s1, r1.str());
            ws_send_frame(s2, r2.str());
            std::cout << "[WS] Room " << code << " match started! Player1=" << c1 << " Player2=" << c2 << std::endl;
        }
        else if (msg_type == "signal")
        {
            // Relay WebRTC signaling to the peer in the same room
            std::string roomId = json_get_string(payload, "roomId");
#ifdef _WIN32
            EnterCriticalSection(&rooms_cs);
#else
            rooms_cs.lock();
#endif
            auto it = rooms.find(roomId);
            if (it != rooms.end())
            {
                SOCKET target = INVALID_SOCKET;
                if (it->second.player1 == client_sock)
                    target = it->second.player2;
                else if (it->second.player2 == client_sock)
                    target = it->second.player1;

                if (target != INVALID_SOCKET)
                {
                    ws_send_frame(target, payload);
                }
            }
#ifdef _WIN32
            LeaveCriticalSection(&rooms_cs);
#else
            rooms_cs.unlock();
#endif
        }
        else if (msg_type == "move")
        {
            std::string move_str = json_get_string(payload, "move");

            if (move_str.empty())
            {
                ws_send_frame(client_sock, json_error("Missing move field"));
                continue;
            }

            // Lock engine state for the duration of move + search
            CSLock lock(engine_cs);

            // 1. Parse and validate the player's move
            int move = parse_move_string(move_str);
            if (move == 0)
            {
                std::string err = json_error("Illegal move: " + move_str);
                std::cout << "[WS] " << err << std::endl;
                ws_send_frame(client_sock, err);
                continue;
            }

            // 2. Make the player's move on the board
            if (!make_move(move, all_moves))
            {
                std::string err = json_error("Illegal move (leaves king in check): " + move_str);
                std::cout << "[WS] " << err << std::endl;
                ws_send_frame(client_sock, err);
                continue;
            }

            std::cout << "[WS] Player move applied: " << move_str << std::endl;

            // 3. Check if game is over after player's move
            MoveList check_list;
            generate_moves(check_list);
            bool has_legal = false;
            for (int i = 0; i < check_list.index; i++)
            {
                // Save state to test legality
                U64 bb_copy[12], occ_copy[3];
                memcpy(bb_copy, bitboards, 96);
                memcpy(occ_copy, occupancies, 24);
                int side_c = side, ep_c = enpassant, castle_c = castle, fifty_c = fifty;

                if (make_move(check_list.moves[i], all_moves))
                {
                    has_legal = true;
                    // Restore state
                    memcpy(bitboards, bb_copy, 96);
                    memcpy(occupancies, occ_copy, 24);
                    side = side_c; enpassant = ep_c; castle = castle_c; fifty = fifty_c;
                    break;
                }
                // make_move already restores on illegal, but let's be safe
                memcpy(bitboards, bb_copy, 96);
                memcpy(occupancies, occ_copy, 24);
                side = side_c; enpassant = ep_c; castle = castle_c; fifty = fifty_c;
            }

            if (!has_legal)
            {
                std::string fen = export_fen();
                // Determine if checkmate or stalemate
                int king_sq = (side == white) ? get_fsb(bitboards[K]) : get_fsb(bitboards[k]);
                int in_check = is_square_attacked(king_sq, 1 - side);
                std::ostringstream go_ss;
                go_ss << "{\"type\":\"game_over\",\"fen\":\"" << fen
                      << "\",\"reason\":\"" << (in_check ? "checkmate" : "stalemate")
                      << "\",\"bestmove\":\"\",\"score\":0,\"valid\":true}";
                std::cout << "[WS] Game over: " << go_ss.str() << std::endl;
                ws_send_frame(client_sock, go_ss.str());
                continue;
            }

            // 4. Run the engine search
            std::cout << "[WS] Starting engine search at depth " << SEARCH_DEPTH << "..." << std::endl;
            SearchResult sr = capture_search(SEARCH_DEPTH);
            std::cout << "[WS] Engine found bestmove: " << sr.bestmove << " score: " << sr.score << std::endl;

            // 5. Make the engine's move on the board
            if (!sr.bestmove.empty())
            {
                int engine_move = parse_move_string(sr.bestmove);
                if (engine_move != 0)
                {
                    make_move(engine_move, all_moves);
                }
            }

            // 6. Export the final FEN and send response
            std::string fen = export_fen();
            std::string response = json_move_result(fen, sr.bestmove, sr.score, true);
            std::cout << "[WS] Sending: " << response << std::endl;
            ws_send_frame(client_sock, response);
        }
        else
        {
            ws_send_frame(client_sock, json_error("Unknown message type: " + msg_type));
        }
    }

    // Remove from rooms and notify opponent if needed
#ifdef _WIN32
    EnterCriticalSection(&rooms_cs);
#else
    rooms_cs.lock();
#endif
    std::string room_key = find_room_for_socket(client_sock);
    if (!room_key.empty())
    {
        Room& room = rooms[room_key];
        SOCKET opponent = INVALID_SOCKET;
        if (room.player1 == client_sock)
            opponent = room.player2;
        else
            opponent = room.player1;

        if (opponent != INVALID_SOCKET)
        {
            ws_send_frame(opponent, "{\"type\":\"opponent_left\"}");
        }
        rooms.erase(room_key);
        std::cout << "[WS] Room " << room_key << " destroyed (player disconnected)." << std::endl;
    }
#ifdef _WIN32
    LeaveCriticalSection(&rooms_cs);
#else
    rooms_cs.unlock();
#endif

    closesocket(client_sock);
}

#ifdef _WIN32
// Thread entry point for client handling (needs WINAPI calling convention on 32-bit)
static DWORD WINAPI client_thread_func(LPVOID arg)
{
    SOCKET s = (SOCKET)(uintptr_t)arg;
    handle_client(s);
    return 0;
}
#else
// Thread entry point for client handling
static void client_thread_func(SOCKET s)
{
    handle_client(s);
}
#endif

/*##########################
  Main — start listening
#############################*/

int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        // Initialize Winsock
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            std::cerr << "[WS] WSAStartup failed." << std::endl;
            return 1;
        }
    #endif

    std::cout << "####################################" << std::endl;
    std::cout << "Domino Chess Engine - WebSocket Mode" << std::endl;
    std::cout << "####################################" << std::endl;

    // Initialize engine (same as main.cpp)
    init_engine();
    parse_FEN_string(start_position);
#ifdef _WIN32
    InitializeCriticalSection(&engine_cs);
    InitializeCriticalSection(&rooms_cs);
#endif
    srand((unsigned int)time(NULL));

    std::cout << "[WS] Engine initialized." << std::endl;

    // Create server socket
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET)
    {
        std::cerr << "[WS] Failed to create socket." << std::endl;
        return 1;
    }

    // Allow port reuse
    int opt_val = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt_val, sizeof(opt_val));

    // Bind to port
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (::bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "[WS] Failed to bind to port " << port << "." << std::endl;
        closesocket(server_sock);
        return 1;
    }

    if (listen(server_sock, 5) == SOCKET_ERROR)
    {
        std::cerr << "[WS] Failed to listen." << std::endl;
        closesocket(server_sock);
        return 1;
    }

    std::cout << "[WS] Server listening on port " << port << std::endl;
    std::cout << "[WS] Waiting for connections..." << std::endl;

    // Accept loop — handle one client at a time (chess is a 1v1 game)
    while (true)
    {
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET)
        {
            std::cerr << "[WS] Accept failed." << std::endl;
            continue;
        }

#ifdef _WIN32
        // Handle client in a new thread
        HANDLE hThread = CreateThread(NULL, 0, client_thread_func, (LPVOID)(uintptr_t)client_sock, 0, NULL);
        if (hThread) CloseHandle(hThread);
#else
        std::thread(client_thread_func, client_sock).detach();
#endif
    }

    closesocket(server_sock);
    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
