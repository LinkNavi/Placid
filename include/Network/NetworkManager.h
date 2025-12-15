// NetworkManager.h - Complete rewrite with all fixes
// Clean P2P networking with host-authority model

#pragma once

#include "HERO.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <iostream>

namespace Network {

// Message type constants (2-char codes for MagicWords)
namespace MessageType {
    const std::string PLAYER_JOIN = "PJ";
    const std::string PLAYER_LEAVE = "PL";
    const std::string PLAYER_STATE = "PS";
    const std::string PLAYER_SPAWN = "SP";
    const std::string PLAYER_SHOOT = "SH";
    const std::string PLAYER_HIT = "HI";
    const std::string PLAYER_DEATH = "DT";
    const std::string GAME_START = "GS";
    const std::string GAME_END = "GE";
    const std::string MAP_INFO = "MI";
    const std::string CHAT_MESSAGE = "CH";
    const std::string PING_REQUEST = "PR";
    const std::string PING_RESPONSE = "PP";
}

// Client info structure
struct ClientInfo {
    uint32_t playerId;
    std::string name;
    std::string ipAddress;
    uint16_t port;
    bool isReady;
    std::chrono::steady_clock::time_point lastSeen;
};

// Callback types
using MessageCallback = std::function<void(const std::string& msgType, 
                                          const std::vector<std::string>& args,
                                          const std::string& fromIP, 
                                          uint16_t fromPort)>;
using PlayerJoinCallback = std::function<void(uint32_t playerId, const std::string& name)>;
using PlayerLeaveCallback = std::function<void(uint32_t playerId)>;

class NetworkManager {
private:
    // Network objects
    HERO::HeroServer* server;
    HERO::HeroClient* client;
    
    // Connection state
    bool isHost;
    bool isConnected;
    std::string hostIP;
    uint16_t port;
    uint32_t localPlayerId;
    uint32_t nextPlayerId;
    
    // Clients (host tracks all clients, clients track known players)
    std::unordered_map<uint32_t, ClientInfo> clients;
    
    // Callbacks
    MessageCallback messageCallback;
    PlayerJoinCallback playerJoinCallback;
    PlayerLeaveCallback playerLeaveCallback;
    
    // Stats
    uint32_t packetsSent;
    uint32_t packetsReceived;
    float lastPingTime;
    
    // Constants
    static constexpr float PING_INTERVAL = 2.0f;     // Ping every 2 seconds
    static constexpr float TIMEOUT_DURATION = 10.0f; // Disconnect after 10 seconds

public:
    NetworkManager() 
        : server(nullptr)
        , client(nullptr)
        , isHost(false)
        , isConnected(false)
        , port(0)
        , localPlayerId(0)
        , nextPlayerId(1)
        , packetsSent(0)
        , packetsReceived(0)
        , lastPingTime(0.0f)
    {}
    
    ~NetworkManager() {
        Disconnect();
    }
    
    // ====================================================================
    // CONNECTION MANAGEMENT
    // ====================================================================
    
    bool HostGame(uint16_t hostPort, const std::string& playerName) {
        if (isConnected) {
            std::cerr << "[NET] Already connected\n";
            return false;
        }
        
        std::cout << "[NET] Starting host on port " << hostPort << "...\n";
        
        // Create server only - host doesn't need client connection
        server = new HERO::HeroServer(hostPort);
        server->Start();
        
        // Brief wait for port to bind
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        isHost = true;
        isConnected = true;
        port = hostPort;
        localPlayerId = nextPlayerId++;
        client = nullptr;
        
        // Add ourselves to client list
        ClientInfo hostInfo;
        hostInfo.playerId = localPlayerId;
        hostInfo.name = playerName;
        hostInfo.ipAddress = "127.0.0.1";
        hostInfo.port = hostPort;
        hostInfo.isReady = true;
        hostInfo.lastSeen = std::chrono::steady_clock::now();
        clients[localPlayerId] = hostInfo;
        
        std::cout << "[NET] Host started. Player ID: " << localPlayerId << "\n";
        std::cout << "[NET] Listening on port " << hostPort << "\n";
        
        return true;
    }
    
    bool JoinGame(const std::string& serverIP, uint16_t serverPort, const std::string& playerName) {
        if (isConnected) {
            std::cerr << "[NET] Already connected\n";
            return false;
        }
        
        std::cout << "[NET] Connecting to " << serverIP << ":" << serverPort << "...\n";
        
        client = new HERO::HeroClient();
        
        if (!client->Connect(serverIP, serverPort)) {
            std::cerr << "[NET] Failed to connect to host\n";
            delete client;
            client = nullptr;
            return false;
        }
        
        isHost = false;
        isConnected = true;
        hostIP = serverIP;
        port = serverPort;
        server = nullptr;
        
        std::cout << "[NET] Connected to host\n";
        
        // Send join request
        auto joinData = HERO::MagicWords::Encode(MessageType::PLAYER_JOIN, playerName);
        client->Send(joinData);
        
        // Wait briefly for ID assignment
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Process incoming messages to get our ID
        HERO::Packet pkt;
        int attempts = 0;
        while (attempts++ < 10 && localPlayerId == 0) {
            if (client->Receive(pkt, 100)) {
                auto [msgType, args] = HERO::MagicWords::Decode(pkt.payload);
                
                if (msgType == MessageType::PLAYER_JOIN && args.size() >= 2) {
                    localPlayerId = std::stoul(args[0]);
                    std::string name = args[1];
                    
                    // Add to our client list
                    ClientInfo info;
                    info.playerId = localPlayerId;
                    info.name = name;
                    info.ipAddress = serverIP;
                    info.port = serverPort;
                    info.lastSeen = std::chrono::steady_clock::now();
                    clients[localPlayerId] = info;
                    
                    std::cout << "[NET] Assigned Player ID: " << localPlayerId << "\n";
                }
            }
        }
        
        if (localPlayerId == 0) {
            std::cerr << "[NET] Failed to receive player ID from host\n";
            Disconnect();
            return false;
        }
        
        return true;
    }
    
    void Disconnect() {
        if (!isConnected) return;
        
        std::cout << "[NET] Disconnecting...\n";
        
        // Notify others we're leaving
        if (client && !isHost) {
            auto data = HERO::MagicWords::Encode(MessageType::PLAYER_LEAVE, localPlayerId);
            client->Send(data);
        }
        
        // Clean up client
        if (client) {
            client->Disconnect();
            delete client;
            client = nullptr;
        }
        
        // Clean up server
        if (server) {
            server->Stop();
            delete server;
            server = nullptr;
        }
        
        isConnected = false;
        isHost = false;
        clients.clear();
        localPlayerId = 0;
        nextPlayerId = 1;
        
        std::cout << "[NET] Disconnected\n";
    }
    
    // ====================================================================
    // UPDATE LOOP
    // ====================================================================
    
    void Update(float deltaTime) {
        if (!isConnected) return;
        
        // Update ping timer
        lastPingTime += deltaTime;
        if (lastPingTime >= PING_INTERVAL) {
            SendPing();
            lastPingTime = 0.0f;
        }
        
        // Process messages as host
        if (isHost && server) {
            server->Poll([this](const HERO::Packet& pkt, const std::string& fromIP, uint16_t fromPort) {
                packetsReceived++;
                HandlePacket(pkt, fromIP, fromPort);
            });
        }
        
        // Process messages as client
        if (!isHost && client) {
            HERO::Packet pkt;
            while (client->Receive(pkt, 1)) {
                packetsReceived++;
                HandlePacket(pkt, hostIP, port);
            }
        }
        
        // Clean up stale clients (host only)
        if (isHost) {
            CleanupStaleClients();
        }
    }
    
    // ====================================================================
    // MESSAGE SENDING
    // ====================================================================
    
    void SendPlayerState(float x, float y, float z, float yaw, float pitch, int health, int weapon) {
        auto data = HERO::MagicWords::Encode(MessageType::PLAYER_STATE,
            localPlayerId, x, y, z, yaw, pitch, health, weapon);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    void SendPlayerSpawn(uint32_t playerId, float x, float y, float z) {
        auto data = HERO::MagicWords::Encode(MessageType::PLAYER_SPAWN, playerId, x, y, z);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    void SendPlayerShoot(float originX, float originY, float originZ,
                        float dirX, float dirY, float dirZ, int weaponType) {
        auto data = HERO::MagicWords::Encode(MessageType::PLAYER_SHOOT,
            localPlayerId, originX, originY, originZ, dirX, dirY, dirZ, weaponType);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    void SendPlayerHit(uint32_t victimId, int damage) {
        auto data = HERO::MagicWords::Encode(MessageType::PLAYER_HIT,
            localPlayerId, victimId, damage);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    void SendPlayerDeath(uint32_t killerId) {
        auto data = HERO::MagicWords::Encode(MessageType::PLAYER_DEATH,
            localPlayerId, killerId);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    void SendGameStart(const std::string& mapName) {
        if (!isHost) {
            std::cerr << "[NET] Only host can start game\n";
            return;
        }
        
        auto data = HERO::MagicWords::Encode(MessageType::GAME_START, mapName);
        BroadcastToClients(data);
        packetsSent++;
    }
    
    void SendGameEnd(const std::string& winner) {
        if (!isHost) return;
        
        auto data = HERO::MagicWords::Encode(MessageType::GAME_END, winner);
        BroadcastToClients(data);
        packetsSent++;
    }
    
    void SendMapInfo(const std::string& mapName, const std::string& mapHash) {
        auto data = HERO::MagicWords::Encode(MessageType::MAP_INFO, mapName, mapHash);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    void SendChatMessage(const std::string& message) {
        auto data = HERO::MagicWords::Encode(MessageType::CHAT_MESSAGE, localPlayerId, message);
        
        if (isHost) {
            BroadcastToClients(data);
        } else {
            SendToHost(data);
        }
        
        packetsSent++;
    }
    
    // ====================================================================
    // CALLBACKS
    // ====================================================================
    
    void SetMessageCallback(MessageCallback callback) {
        messageCallback = callback;
    }
    
    void SetPlayerJoinCallback(PlayerJoinCallback callback) {
        playerJoinCallback = callback;
    }
    
    void SetPlayerLeaveCallback(PlayerLeaveCallback callback) {
        playerLeaveCallback = callback;
    }
    
    // ====================================================================
    // QUERIES
    // ====================================================================
    
    bool IsHost() const { return isHost; }
    bool IsConnected() const { return isConnected; }
    uint32_t GetLocalPlayerId() const { return localPlayerId; }
    int GetPlayerCount() const { return clients.size(); }
    uint32_t GetPacketsSent() const { return packetsSent; }
    uint32_t GetPacketsReceived() const { return packetsReceived; }
    const std::unordered_map<uint32_t, ClientInfo>& GetClients() const { return clients; }

private:
    // ====================================================================
    // INTERNAL METHODS
    // ====================================================================
    
    void HandlePacket(const HERO::Packet& pkt, const std::string& fromIP, uint16_t fromPort) {
        auto [msgType, args] = HERO::MagicWords::Decode(pkt.payload);
        
        // ---- PLAYER JOIN ----
        if (msgType == MessageType::PLAYER_JOIN) {
            if (isHost && args.size() >= 1) {
                // HOST: New player joining
                std::string playerName = args[0];
                uint32_t newPlayerId = nextPlayerId++;
                
                ClientInfo newClient;
                newClient.playerId = newPlayerId;
                newClient.name = playerName;
                newClient.ipAddress = fromIP;
                newClient.port = fromPort;
                newClient.isReady = false;
                newClient.lastSeen = std::chrono::steady_clock::now();
                clients[newPlayerId] = newClient;
                
                std::cout << "[NET] Player joined: " << playerName << " (ID: " << newPlayerId << ")\n";
                
                // Send new player their assigned ID
                auto response = HERO::MagicWords::Encode(MessageType::PLAYER_JOIN, newPlayerId, playerName);
                server->SendTo(response, fromIP, fromPort);
                
                // Send new player info about all existing players
                for (const auto& [id, client] : clients) {
                    if (id != newPlayerId) {
                        auto playerInfo = HERO::MagicWords::Encode(MessageType::PLAYER_JOIN, id, client.name);
                        server->SendTo(playerInfo, fromIP, fromPort);
                    }
                }
                
                // Notify all other clients about the new player
                for (const auto& [id, client] : clients) {
                    if (id != localPlayerId && id != newPlayerId) {
                        server->SendTo(response, client.ipAddress, client.port);
                    }
                }
                
                if (playerJoinCallback) {
                    playerJoinCallback(newPlayerId, playerName);
                }
            }
            else if (!isHost && args.size() >= 2) {
                // CLIENT: Receiving player info from host
                uint32_t playerId = std::stoul(args[0]);
                std::string playerName = args[1];
                
                // Skip if this is us (already added during connect)
                if (playerId == localPlayerId) {
                    return;
                }
                
                // Add/update in our client list
                ClientInfo info;
                info.playerId = playerId;
                info.name = playerName;
                info.ipAddress = hostIP;
                info.port = port;
                info.isReady = false;
                info.lastSeen = std::chrono::steady_clock::now();
                clients[playerId] = info;
                
                std::cout << "[NET] Player in lobby: " << playerName << " (ID: " << playerId << ")\n";
                
                if (playerJoinCallback) {
                    playerJoinCallback(playerId, playerName);
                }
            }
        }
        
        // ---- PLAYER LEAVE ----
        else if (msgType == MessageType::PLAYER_LEAVE && args.size() >= 1) {
            uint32_t playerId = std::stoul(args[0]);
            
            auto it = clients.find(playerId);
            if (it != clients.end()) {
                std::string playerName = it->second.name;
                clients.erase(it);
                
                std::cout << "[NET] Player left: " << playerName << " (ID: " << playerId << ")\n";
                
                // Host broadcasts to other clients
                if (isHost) {
                    auto data = HERO::MagicWords::Encode(MessageType::PLAYER_LEAVE, playerId);
                    BroadcastToClients(data);
                }
                
                if (playerLeaveCallback) {
                    playerLeaveCallback(playerId);
                }
            }
        }
        
        // ---- PING REQUEST ----
        else if (msgType == MessageType::PING_REQUEST && args.size() >= 1) {
            uint32_t playerId = std::stoul(args[0]);
            
            if (isHost) {
                // Host received ping from client - update last seen
                auto it = clients.find(playerId);
                if (it != clients.end()) {
                    it->second.lastSeen = std::chrono::steady_clock::now();
                }
            } else {
                // Client received ping from host - respond immediately
                auto response = HERO::MagicWords::Encode(MessageType::PING_RESPONSE, localPlayerId);
                SendToHost(response);
            }
        }
        
        // ---- PING RESPONSE ----
        else if (msgType == MessageType::PING_RESPONSE && args.size() >= 1) {
            if (isHost) {
                uint32_t playerId = std::stoul(args[0]);
                auto it = clients.find(playerId);
                if (it != clients.end()) {
                    it->second.lastSeen = std::chrono::steady_clock::now();
                }
            }
        }
        
        // ---- PLAYER STATE ----
        else if (msgType == MessageType::PLAYER_STATE && args.size() >= 8) {
            uint32_t playerId = std::stoul(args[0]);
            
            // Update last seen for this player
            auto it = clients.find(playerId);
            if (it != clients.end()) {
                it->second.lastSeen = std::chrono::steady_clock::now();
            }
            
            // Host broadcasts to other clients
            if (isHost && playerId != localPlayerId) {
                BroadcastToOthers(pkt.payload, playerId);
            }
            
            if (messageCallback) {
                messageCallback(msgType, args, fromIP, fromPort);
            }
        }
        
        // ---- PLAYER SHOOT ----
        else if (msgType == MessageType::PLAYER_SHOOT && args.size() >= 8) {
            // Host validates and broadcasts to all clients
            if (isHost) {
                uint32_t shooterId = std::stoul(args[0]);
                BroadcastToOthers(pkt.payload, shooterId);
            }
            
            if (messageCallback) {
                messageCallback(msgType, args, fromIP, fromPort);
            }
        }
        
        // ---- CHAT MESSAGE ----
        else if (msgType == MessageType::CHAT_MESSAGE && args.size() >= 2) {
            uint32_t senderId = std::stoul(args[0]);
            std::string message = args[1];
            
            // Find sender name
            std::string senderName = "Unknown";
            auto it = clients.find(senderId);
            if (it != clients.end()) {
                senderName = it->second.name;
            }
            
            std::cout << "[CHAT] " << senderName << ": " << message << "\n";
            
            // Host broadcasts to all other clients
            if (isHost && senderId != localPlayerId) {
                BroadcastToOthers(pkt.payload, senderId);
            }
            
            if (messageCallback) {
                messageCallback(msgType, args, fromIP, fromPort);
            }
        }
        
        // ---- OTHER MESSAGES ----
        else if (messageCallback) {
            messageCallback(msgType, args, fromIP, fromPort);
        }
    }
    
    void SendPing() {
        if (isHost) {
            // Host sends ping to all clients
            for (const auto& [id, client] : clients) {
                if (id != localPlayerId) {
                    auto data = HERO::MagicWords::Encode(MessageType::PING_REQUEST, id);
                    server->SendTo(data, client.ipAddress, client.port);
                }
            }
        } else {
            // Client sends ping to host
            auto data = HERO::MagicWords::Encode(MessageType::PING_REQUEST, localPlayerId);
            SendToHost(data);
        }
    }
    
    void CleanupStaleClients() {
        if (!isHost) return;
        
        auto now = std::chrono::steady_clock::now();
        std::vector<uint32_t> toRemove;
        
        for (const auto& [id, client] : clients) {
            if (id == localPlayerId) continue; // Don't check ourselves
            
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - client.lastSeen).count();
            if (elapsed > TIMEOUT_DURATION) {
                std::cout << "[NET] Client timeout: " << id << "\n";
                toRemove.push_back(id);
            }
        }
        
        for (uint32_t id : toRemove) {
            auto it = clients.find(id);
            if (it != clients.end()) {
                std::string playerName = it->second.name;
                clients.erase(it);
                
                std::cout << "[NET] Player left: " << playerName << " (ID: " << id << ")\n";
                
                // Notify other clients
                auto data = HERO::MagicWords::Encode(MessageType::PLAYER_LEAVE, id);
                BroadcastToClients(data);
                
                if (playerLeaveCallback) {
                    playerLeaveCallback(id);
                }
            }
        }
    }
    
    void SendToHost(const std::vector<uint8_t>& data) {
        if (client && !isHost) {
            client->Send(data);
        }
    }
    
    void BroadcastToClients(const std::vector<uint8_t>& data) {
        if (!isHost || !server) return;
        
        for (const auto& [id, client] : clients) {
            if (id != localPlayerId) {
                server->SendTo(data, client.ipAddress, client.port);
            }
        }
    }
    
    void BroadcastToOthers(const std::vector<uint8_t>& data, uint32_t excludeId) {
        if (!isHost || !server) return;
        
        for (const auto& [id, client] : clients) {
            if (id != localPlayerId && id != excludeId) {
                server->SendTo(data, client.ipAddress, client.port);
            }
        }
    }
};

} // namespace Network
