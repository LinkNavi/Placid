// NetworkManager.h - Complete ENet implementation

#pragma once

#include <enet/enet.h>
#include "PCD/PCD.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <functional>
#include <cstring>
#include <fstream>

namespace Network {

enum class MessageType : uint8_t {
    PLAYER_JOIN = 0,
    PLAYER_LEAVE = 1,
    PLAYER_STATE = 2,
    CHAT_MESSAGE = 3,
    MAP_REQUEST = 4,
    MAP_CHUNK = 5,
    MAP_COMPLETE = 6,
    GAME_START = 7
};

struct ClientInfo {
    uint32_t playerId;
    std::string name;
    ENetPeer* peer;
    bool hasMap;
    bool isReady;
};

using MessageCallback = std::function<void(const std::string&, const std::vector<std::string>&)>;
using PlayerJoinCallback = std::function<void(uint32_t, const std::string&)>;
using PlayerLeaveCallback = std::function<void(uint32_t)>;
using MapLoadedCallback = std::function<void(const PCD::Map&)>;

class NetworkManager {
private:
    ENetHost* host;
    ENetPeer* serverPeer;
    bool isHost;
    bool isConnected;
    uint32_t localPlayerId;
    uint32_t nextPlayerId;
    std::unordered_map<uint32_t, ClientInfo> clients;
    
    // Callbacks
    MessageCallback messageCallback;
    PlayerJoinCallback playerJoinCallback;
    PlayerLeaveCallback playerLeaveCallback;
    MapLoadedCallback mapLoadedCallback;
    
    // Map transfer
    PCD::Map currentMap;
    std::string currentMapPath;
    std::vector<uint8_t> receivedMapData;
    size_t expectedMapSize;
    size_t chunksReceived;
    
    // Stats
    uint32_t packetsSent;
    uint32_t packetsReceived;

public:
    NetworkManager() 
        : host(nullptr), serverPeer(nullptr), isHost(false)
        , isConnected(false), localPlayerId(0), nextPlayerId(1)
        , expectedMapSize(0), chunksReceived(0)
        , packetsSent(0), packetsReceived(0)
    {
        if (enet_initialize() != 0) {
            std::cerr << "[NET] Failed to initialize ENet\n";
        }
    }
    
    ~NetworkManager() {
        Disconnect();
        enet_deinitialize();
    }
    
    bool HostGame(uint16_t port, const std::string& playerName) {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        
        host = enet_host_create(&address, 32, 2, 0, 0);
        if (!host) {
            std::cerr << "[NET] Failed to create host\n";
            return false;
        }
        
        isHost = true;
        isConnected = true;
        localPlayerId = nextPlayerId++;
        
        ClientInfo info;
        info.playerId = localPlayerId;
        info.name = playerName;
        info.peer = nullptr;
        info.hasMap = true;
        info.isReady = true;
        clients[localPlayerId] = info;
        
        std::cout << "[NET] Host started on port " << port << "\n";
        std::cout << "[NET] Player ID: " << localPlayerId << "\n";
        return true;
    }
    
    bool JoinGame(const std::string& serverIP, uint16_t port, const std::string& playerName) {
        host = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!host) {
            std::cerr << "[NET] Failed to create client\n";
            return false;
        }
        
        ENetAddress address;
        enet_address_set_host(&address, serverIP.c_str());
        address.port = port;
        
        serverPeer = enet_host_connect(host, &address, 2, 0);
        if (!serverPeer) {
            std::cerr << "[NET] Failed to connect\n";
            enet_host_destroy(host);
            return false;
        }
        
        std::cout << "[NET] Connecting to " << serverIP << ":" << port << "...\n";
        
        ENetEvent event;
        if (enet_host_service(host, &event, 5000) > 0 && 
            event.type == ENET_EVENT_TYPE_CONNECT) {
            
            std::cout << "[NET] Connected!\n";
            
            // Send join request
            std::vector<uint8_t> data;
            data.push_back((uint8_t)MessageType::PLAYER_JOIN);
            data.insert(data.end(), playerName.begin(), playerName.end());
            
            ENetPacket* packet = enet_packet_create(data.data(), data.size(), 
                                                    ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(serverPeer, 0, packet);
            enet_host_flush(host);
            
            // Wait for player ID
            int attempts = 0;
            while (attempts++ < 20) {
                if (enet_host_service(host, &event, 100) > 0) {
                    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                        HandlePacket(event.packet->data, event.packet->dataLength, event.peer);
                        enet_packet_destroy(event.packet);
                        
                        if (localPlayerId != 0) {
                            isHost = false;
                            isConnected = true;
                            return true;
                        }
                    }
                }
            }
            
            std::cerr << "[NET] Failed to get player ID\n";
            enet_peer_disconnect(serverPeer, 0);
            enet_host_destroy(host);
            return false;
        }
        
        std::cerr << "[NET] Connection timeout\n";
        enet_peer_reset(serverPeer);
        enet_host_destroy(host);
        return false;
    }
    
    void Update(float deltaTime) {
        if (!isConnected || !host) return;
        
        ENetEvent event;
        while (enet_host_service(host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    std::cout << "[NET] Peer connected\n";
                    break;
                    
                case ENET_EVENT_TYPE_RECEIVE:
                    packetsReceived++;
                    HandlePacket(event.packet->data, event.packet->dataLength, event.peer);
                    enet_packet_destroy(event.packet);
                    break;
                    
                case ENET_EVENT_TYPE_DISCONNECT:
                    HandleDisconnect(event.peer);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    bool LoadMap(const std::string& mapPath) {
        if (!isHost) return false;
        
        std::cout << "[NET] Loading map: " << mapPath << "\n";
        
        if (!PCD::PCDReader::Load(currentMap, mapPath)) {
            std::cerr << "[NET] Failed to load map\n";
            return false;
        }
        
        currentMapPath = mapPath;
        std::cout << "[NET] Map loaded successfully\n";
        std::cout << "[NET]   Brushes: " << currentMap.brushes.size() << "\n";
        std::cout << "[NET]   Entities: " << currentMap.entities.size() << "\n";
        
        return true;
    }
    
    const PCD::Map& GetMap() const { return currentMap; }
    
    void RequestMap() {
        if (isHost || !serverPeer) return;
        
        std::cout << "[NET] Requesting map...\n";
        
        std::vector<uint8_t> data;
        data.push_back((uint8_t)MessageType::MAP_REQUEST);
        
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), 
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(serverPeer, 0, packet);
        enet_host_flush(host);
        packetsSent++;
    }
    
    void SendPlayerState(float x, float y, float z, float yaw, float pitch, int health, int weapon) {
        if (!isConnected) return;
        
        struct __attribute__((packed)) StatePacket {
            uint8_t type;
            uint32_t playerId;
            float x, y, z, yaw, pitch;
            int32_t health, weapon;
        } packet;
        
        packet.type = (uint8_t)MessageType::PLAYER_STATE;
        packet.playerId = localPlayerId;
        packet.x = x;
        packet.y = y;
        packet.z = z;
        packet.yaw = yaw;
        packet.pitch = pitch;
        packet.health = health;
        packet.weapon = weapon;
        
        ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(packet), 
                                                     ENET_PACKET_FLAG_UNSEQUENCED);
        
        if (isHost) {
            enet_host_broadcast(host, 1, enetPacket);
        } else {
            enet_peer_send(serverPeer, 1, enetPacket);
        }
        packetsSent++;
    }
    
    void SendChatMessage(const std::string& message) {
        if (!isConnected) return;
        
        std::vector<uint8_t> data;
        data.push_back((uint8_t)MessageType::CHAT_MESSAGE);
        
        uint32_t id = localPlayerId;
        data.push_back((id >> 24) & 0xFF);
        data.push_back((id >> 16) & 0xFF);
        data.push_back((id >> 8) & 0xFF);
        data.push_back(id & 0xFF);
        
        data.insert(data.end(), message.begin(), message.end());
        
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), 
                                                ENET_PACKET_FLAG_RELIABLE);
        
        if (isHost) {
            enet_host_broadcast(host, 0, packet);
        } else {
            enet_peer_send(serverPeer, 0, packet);
        }
        packetsSent++;
    }
    
    void SendGameStart(const std::string& mapName) {
        if (!isHost) return;
        
        std::vector<uint8_t> data;
        data.push_back((uint8_t)MessageType::GAME_START);
        data.insert(data.end(), mapName.begin(), mapName.end());
        
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), 
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(host, 0, packet);
        packetsSent++;
    }
    
    void Disconnect() {
        if (!isConnected) return;
        
        if (!isHost && serverPeer) {
            enet_peer_disconnect(serverPeer, 0);
            
            ENetEvent event;
            while (enet_host_service(host, &event, 3000) > 0) {
                if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
                if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                    enet_packet_destroy(event.packet);
                }
            }
            
            enet_peer_reset(serverPeer);
            serverPeer = nullptr;
        }
        
        if (host) {
            enet_host_destroy(host);
            host = nullptr;
        }
        
        isConnected = false;
        clients.clear();
        localPlayerId = 0;
        
        std::cout << "[NET] Disconnected\n";
    }
    
    void SetMessageCallback(MessageCallback callback) { messageCallback = callback; }
    void SetPlayerJoinCallback(PlayerJoinCallback callback) { playerJoinCallback = callback; }
    void SetPlayerLeaveCallback(PlayerLeaveCallback callback) { playerLeaveCallback = callback; }
    void SetMapLoadedCallback(MapLoadedCallback callback) { mapLoadedCallback = callback; }
    
    bool IsConnected() const { return isConnected; }
    bool IsHost() const { return isHost; }
    uint32_t GetLocalPlayerId() const { return localPlayerId; }
    int GetPlayerCount() const { return clients.size(); }
    uint32_t GetPacketsSent() const { return packetsSent; }
    uint32_t GetPacketsReceived() const { return packetsReceived; }
    const std::unordered_map<uint32_t, ClientInfo>& GetClients() const { return clients; }

private:
    void HandlePacket(const uint8_t* data, size_t length, ENetPeer* peer) {
        if (length < 1) return;
        
        MessageType type = (MessageType)data[0];
        
        switch (type) {
            case MessageType::PLAYER_JOIN:
                HandlePlayerJoin(data, length, peer);
                break;
                
            case MessageType::PLAYER_STATE:
                HandlePlayerState(data, length);
                break;
                
            case MessageType::CHAT_MESSAGE:
                HandleChatMessage(data, length);
                break;
                
            case MessageType::MAP_REQUEST:
                HandleMapRequest(peer);
                break;
                
            case MessageType::MAP_CHUNK:
                HandleMapChunk(data, length);
                break;
                
            case MessageType::MAP_COMPLETE:
                HandleMapComplete(peer);
                break;
                
            case MessageType::GAME_START:
                HandleGameStart(data, length);
                break;
                
            default:
                break;
        }
    }
    
    void HandlePlayerJoin(const uint8_t* data, size_t length, ENetPeer* peer) {
        if (isHost) {
            // Host receives join request
            std::string name((char*)data + 1, length - 1);
            uint32_t newId = nextPlayerId++;
            
            ClientInfo info;
            info.playerId = newId;
            info.name = name;
            info.peer = peer;
            info.hasMap = false;
            info.isReady = false;
            clients[newId] = info;
            
            peer->data = (void*)(uintptr_t)newId;
            
            std::cout << "[NET] Player joined: " << name << " (ID: " << newId << ")\n";
            
            // Send player ID
            std::vector<uint8_t> response;
            response.push_back((uint8_t)MessageType::PLAYER_JOIN);
            response.push_back((newId >> 24) & 0xFF);
            response.push_back((newId >> 16) & 0xFF);
            response.push_back((newId >> 8) & 0xFF);
            response.push_back(newId & 0xFF);
            response.insert(response.end(), name.begin(), name.end());
            
            ENetPacket* packet = enet_packet_create(response.data(), response.size(),
                                                   ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);
            
            // Send info about all other players to new player
            for (const auto& [id, client] : clients) {
                if (id != newId && id != localPlayerId) {
                    std::vector<uint8_t> info;
                    info.push_back((uint8_t)MessageType::PLAYER_JOIN);
                    info.push_back((id >> 24) & 0xFF);
                    info.push_back((id >> 16) & 0xFF);
                    info.push_back((id >> 8) & 0xFF);
                    info.push_back(id & 0xFF);
                    info.insert(info.end(), client.name.begin(), client.name.end());
                    
                    ENetPacket* p = enet_packet_create(info.data(), info.size(),
                                                       ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(peer, 0, p);
                }
            }
            
            // Notify other clients
            for (const auto& [id, client] : clients) {
                if (id != newId && id != localPlayerId && client.peer) {
                    ENetPacket* p = enet_packet_create(response.data(), response.size(),
                                                       ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(client.peer, 0, p);
                }
            }
            
            enet_host_flush(host);
            
            if (playerJoinCallback) {
                playerJoinCallback(newId, name);
            }
        } else {
            // Client receives player info
            if (length >= 5) {
                uint32_t playerId = (data[1] << 24) | (data[2] << 16) | 
                                   (data[3] << 8) | data[4];
                std::string name((char*)data + 5, length - 5);
                
                if (localPlayerId == 0) {
                    localPlayerId = playerId;
                    std::cout << "[NET] Assigned player ID: " << localPlayerId << "\n";
                    
                    ClientInfo info;
                    info.playerId = localPlayerId;
                    info.name = name;
                    info.peer = serverPeer;
                    info.hasMap = false;
                    info.isReady = false;
                    clients[localPlayerId] = info;
                } else {
                    ClientInfo info;
                    info.playerId = playerId;
                    info.name = name;
                    info.peer = serverPeer;
                    info.hasMap = false;
                    info.isReady = false;
                    clients[playerId] = info;
                    
                    std::cout << "[NET] Player in lobby: " << name << " (ID: " << playerId << ")\n";
                    
                    if (playerJoinCallback) {
                        playerJoinCallback(playerId, name);
                    }
                }
            }
        }
    }
    
    void HandlePlayerState(const uint8_t* data, size_t length) {
        if (length < sizeof(uint8_t) + sizeof(uint32_t) + 5 * sizeof(float) + 2 * sizeof(int32_t)) return;
        
        const uint8_t* ptr = data + 1;
        uint32_t playerId = *(uint32_t*)ptr; ptr += sizeof(uint32_t);
        float x = *(float*)ptr; ptr += sizeof(float);
        float y = *(float*)ptr; ptr += sizeof(float);
        float z = *(float*)ptr; ptr += sizeof(float);
        float yaw = *(float*)ptr; ptr += sizeof(float);
        float pitch = *(float*)ptr; ptr += sizeof(float);
        int32_t health = *(int32_t*)ptr; ptr += sizeof(int32_t);
        int32_t weapon = *(int32_t*)ptr;
        
        if (messageCallback) {
            std::vector<std::string> args;
            args.push_back(std::to_string(playerId));
            args.push_back(std::to_string(x));
            args.push_back(std::to_string(y));
            args.push_back(std::to_string(z));
            args.push_back(std::to_string(yaw));
            args.push_back(std::to_string(pitch));
            args.push_back(std::to_string(health));
            args.push_back(std::to_string(weapon));
            messageCallback("PLAYER_STATE", args);
        }
    }
    
    void HandleChatMessage(const uint8_t* data, size_t length) {
        if (length < 6) return;
        
        uint32_t senderId = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        std::string message((char*)data + 5, length - 5);
        
        std::string senderName = "Unknown";
        auto it = clients.find(senderId);
        if (it != clients.end()) {
            senderName = it->second.name;
        }
        
        std::cout << "[CHAT] " << senderName << ": " << message << "\n";
        
        if (messageCallback) {
            std::vector<std::string> args;
            args.push_back(std::to_string(senderId));
            args.push_back(message);
            messageCallback("CHAT_MESSAGE", args);
        }
    }
    
    void HandleMapRequest(ENetPeer* peer) {
        if (!isHost || currentMapPath.empty()) return;
        
        std::cout << "[NET] Sending map to client...\n";
        
        std::ifstream f(currentMapPath, std::ios::binary | std::ios::ate);
        if (!f.is_open()) {
            std::cerr << "[NET] Cannot open map file\n";
            return;
        }
        
        size_t fileSize = f.tellg();
        f.seekg(0);
        
        std::vector<uint8_t> fileData(fileSize);
        f.read((char*)fileData.data(), fileSize);
        f.close();
        
        // Send in 8KB chunks
        const size_t chunkSize = 8192;
        size_t totalChunks = (fileSize + chunkSize - 1) / chunkSize;
        
        for (size_t i = 0; i < totalChunks; i++) {
            size_t offset = i * chunkSize;
            size_t size = std::min(chunkSize, fileSize - offset);
            
            std::vector<uint8_t> chunkData;
            chunkData.push_back((uint8_t)MessageType::MAP_CHUNK);
            chunkData.push_back((i >> 8) & 0xFF);
            chunkData.push_back(i & 0xFF);
            chunkData.push_back((totalChunks >> 8) & 0xFF);
            chunkData.push_back(totalChunks & 0xFF);
            chunkData.push_back((fileSize >> 24) & 0xFF);
            chunkData.push_back((fileSize >> 16) & 0xFF);
            chunkData.push_back((fileSize >> 8) & 0xFF);
            chunkData.push_back(fileSize & 0xFF);
            chunkData.insert(chunkData.end(), &fileData[offset], &fileData[offset + size]);
            
            ENetPacket* packet = enet_packet_create(chunkData.data(), chunkData.size(),
                                                    ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);
        }
        
        enet_host_flush(host);
        std::cout << "[NET] Map sent (" << totalChunks << " chunks)\n";
    }
    
    void HandleMapChunk(const uint8_t* data, size_t length) {
        if (length < 9) return;
        
        size_t chunkIdx = (data[1] << 8) | data[2];
        size_t totalChunks = (data[3] << 8) | data[4];
        size_t fileSize = (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
        
        if (chunkIdx == 0) {
            receivedMapData.clear();
            receivedMapData.reserve(fileSize);
            expectedMapSize = fileSize;
            chunksReceived = 0;
            std::cout << "[NET] Receiving map: " << fileSize << " bytes in " << totalChunks << " chunks\n";
        }
        
        receivedMapData.insert(receivedMapData.end(), data + 9, data + length);
        chunksReceived++;
        
        float progress = (float)chunksReceived / totalChunks * 100.0f;
        std::cout << "[NET] Progress: " << (int)progress << "%\r" << std::flush;
        
        if (chunksReceived >= totalChunks) {
            std::cout << "\n[NET] Map transfer complete!\n";
            
            std::string tempPath = "/tmp/received_map.pcd";
            std::ofstream f(tempPath, std::ios::binary);
            f.write((char*)receivedMapData.data(), receivedMapData.size());
            f.close();
            
            if (PCD::PCDReader::Load(currentMap, tempPath)) {
                std::cout << "[NET] Map loaded successfully\n";
                clients[localPlayerId].hasMap = true;
                
                // Send acknowledgment
                std::vector<uint8_t> ack;
                ack.push_back((uint8_t)MessageType::MAP_COMPLETE);
                
                ENetPacket* packet = enet_packet_create(ack.data(), ack.size(),
                                                       ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(serverPeer, 0, packet);
                enet_host_flush(host);
                
                if (mapLoadedCallback) {
                    mapLoadedCallback(currentMap);
                }
            } else {
                std::cerr << "[NET] Failed to load received map\n";
            }
            
            receivedMapData.clear();
        }
    }
    
    void HandleMapComplete(ENetPeer* peer) {
        if (!peer->data) return;
        
        uint32_t playerId = (uint32_t)(uintptr_t)peer->data;
        auto it = clients.find(playerId);
        if (it != clients.end()) {
            it->second.hasMap = true;
            std::cout << "[NET] Client " << playerId << " has map\n";
        }
    }
    
    void HandleGameStart(const uint8_t* data, size_t length) {
        std::string mapName((char*)data + 1, length - 1);
        std::cout << "[NET] GAME STARTING - Map: " << mapName << "\n";
        
        if (messageCallback) {
            std::vector<std::string> args;
            args.push_back(mapName);
            messageCallback("GAME_START", args);
        }
    }
    
    void HandleDisconnect(ENetPeer* peer) {
        if (!peer->data) return;
        
        uint32_t playerId = (uint32_t)(uintptr_t)peer->data;
        auto it = clients.find(playerId);
        if (it != clients.end()) {
            std::cout << "[NET] Player left: " << it->second.name << "\n";
            
            if (playerLeaveCallback) {
                playerLeaveCallback(playerId);
            }
            
            clients.erase(it);
        }
        
        peer->data = nullptr;
    }
};

} // namespace Network
