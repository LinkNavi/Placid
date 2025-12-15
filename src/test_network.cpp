// test_network.cpp - Clean test program for NetworkManager

#include "Network/NetworkManager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>

void PrintHelp() {
    std::cout << "\nCommands:\n";
    std::cout << "  help          - Show this help\n";
    std::cout << "  state         - Send player state (test)\n";
    std::cout << "  shoot         - Send shoot event (test)\n";
    std::cout << "  chat <msg>    - Send chat message\n";
    std::cout << "  start <map>   - Start game (host only)\n";
    std::cout << "  list          - List connected players\n";
    std::cout << "  stats         - Show network stats\n";
    std::cout << "  quit          - Exit\n\n";
}

void RunHost(uint16_t port) {
    Network::NetworkManager net;
    
    std::cout << "\n=== HOST MODE ===\n";
    if (!net.HostGame(port, "Host")) {
        std::cerr << "Failed to start host\n";
        return;
    }
    
    std::cout << "Waiting for players...\n";
    PrintHelp();
    
    // Set up callbacks
    net.SetPlayerJoinCallback([](uint32_t playerId, const std::string& name) {
        std::cout << "\n>>> " << name << " joined (ID: " << playerId << ")\n> ";
        std::cout.flush();
    });
    
    net.SetPlayerLeaveCallback([&net](uint32_t playerId) {
        // Find player name
        std::string name = "Player " + std::to_string(playerId);
        const auto& clients = net.GetClients();
        auto it = clients.find(playerId);
        if (it != clients.end()) {
            name = it->second.name;
        }
        std::cout << "\n>>> " << name << " left\n> ";
        std::cout.flush();
    });
    
    net.SetMessageCallback([&net](const std::string& msgType, const std::vector<std::string>& args,
                              const std::string& fromIP, uint16_t fromPort) {
        // Filter out system messages
        if (msgType == Network::MessageType::PING_REQUEST || 
            msgType == Network::MessageType::PING_RESPONSE ||
            msgType == Network::MessageType::PLAYER_STATE ||
            msgType.empty()) {
            return;
        }
        
        if (msgType == Network::MessageType::PLAYER_SHOOT && args.size() >= 8) {
            uint32_t shooterId = std::stoul(args[0]);
            int weaponType = std::stoi(args[7]);
            
            std::string playerName = "Player " + std::to_string(shooterId);
            const auto& clients = net.GetClients();
            auto it = clients.find(shooterId);
            if (it != clients.end()) {
                playerName = it->second.name;
            }
            
            std::cout << "\n>>> " << playerName << " fired weapon " << weaponType << "\n> ";
            std::cout.flush();
        }
    });
    
    // Main loop
    std::string line;
    bool running = true;
    
    // Update thread
    std::thread updateThread([&]() {
        while (running) {
            net.Update(0.016f);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    
    // Command loop
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            running = false;
        }
        else if (command == "help") {
            PrintHelp();
        }
        else if (command == "state") {
            net.SendPlayerState(1.0f, 2.0f, 3.0f, 0.5f, 0.3f, 100, 1);
            std::cout << "Sent player state\n";
        }
        else if (command == "shoot") {
            net.SendPlayerShoot(1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0);
            std::cout << "Sent shoot event\n";
        }
        else if (command == "chat") {
            // Get rest of line as message
            std::string msg;
            std::getline(iss, msg);
            // Trim leading space
            if (!msg.empty() && msg[0] == ' ') {
                msg = msg.substr(1);
            }
            
            if (!msg.empty()) {
                net.SendChatMessage(msg);
            } else {
                std::cout << "Usage: chat <message>\n";
            }
        }
        else if (command == "start") {
            std::string mapName;
            iss >> mapName;
            if (mapName.empty()) {
                mapName = "test_map.pcd";
            }
            net.SendGameStart(mapName);
            std::cout << "Game started with map: " << mapName << "\n";
        }
        else if (command == "list") {
            std::cout << "\nConnected players (" << net.GetPlayerCount() << "):\n";
            for (const auto& [id, client] : net.GetClients()) {
                std::cout << "  [" << id << "] " << client.name;
                if (id == net.GetLocalPlayerId()) {
                    std::cout << " (you)";
                }
                std::cout << "\n";
            }
        }
        else if (command == "stats") {
            std::cout << "\nNetwork Stats:\n";
            std::cout << "  Players: " << net.GetPlayerCount() << "\n";
            std::cout << "  Sent: " << net.GetPacketsSent() << " packets\n";
            std::cout << "  Received: " << net.GetPacketsReceived() << " packets\n";
        }
        else if (!command.empty()) {
            std::cout << "Unknown command. Type 'help' for commands.\n";
        }
    }
    
    running = false;
    updateThread.join();
    net.Disconnect();
}

void RunClient(const std::string& hostIP, uint16_t port) {
    Network::NetworkManager net;
    
    std::cout << "\n=== CLIENT MODE ===\n";
    std::cout << "Enter your name: ";
    std::string playerName;
    std::getline(std::cin, playerName);
    if (playerName.empty()) playerName = "Player";
    
    if (!net.JoinGame(hostIP, port, playerName)) {
        std::cerr << "Failed to connect\n";
        return;
    }
    
    std::cout << "Connected!\n";
    PrintHelp();
    
    // Set up callbacks
    net.SetPlayerJoinCallback([](uint32_t playerId, const std::string& name) {
        std::cout << "\n>>> " << name << " joined (ID: " << playerId << ")\n> ";
        std::cout.flush();
    });
    
    net.SetPlayerLeaveCallback([&net](uint32_t playerId) {
        std::string name = "Player " + std::to_string(playerId);
        const auto& clients = net.GetClients();
        auto it = clients.find(playerId);
        if (it != clients.end()) {
            name = it->second.name;
        }
        std::cout << "\n>>> " << name << " left\n> ";
        std::cout.flush();
    });
    
    net.SetMessageCallback([&net](const std::string& msgType, const std::vector<std::string>& args,
                              const std::string& fromIP, uint16_t fromPort) {
        // Filter system messages
        if (msgType == Network::MessageType::PING_REQUEST || 
            msgType == Network::MessageType::PING_RESPONSE ||
            msgType == Network::MessageType::PLAYER_STATE ||
            msgType.empty()) {
            return;
        }
        
        if (msgType == Network::MessageType::PLAYER_SHOOT && args.size() >= 8) {
            uint32_t shooterId = std::stoul(args[0]);
            int weaponType = std::stoi(args[7]);
            
            std::string playerName = "Player " + std::to_string(shooterId);
            const auto& clients = net.GetClients();
            auto it = clients.find(shooterId);
            if (it != clients.end()) {
                playerName = it->second.name;
            }
            
            std::cout << "\n>>> " << playerName << " fired weapon " << weaponType << "\n> ";
            std::cout.flush();
        }
        else if (msgType == Network::MessageType::GAME_START && !args.empty()) {
            std::cout << "\n>>> GAME STARTING - Map: " << args[0] << "\n> ";
            std::cout.flush();
        }
    });
    
    // Main loop
    std::string line;
    bool running = true;
    
    // Update thread
    std::thread updateThread([&]() {
        while (running) {
            net.Update(0.016f);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    
    // Command loop
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit" || command == "exit") {
            running = false;
        }
        else if (command == "help") {
            PrintHelp();
        }
        else if (command == "state") {
            net.SendPlayerState(5.0f, 1.0f, 7.0f, 1.2f, 0.1f, 80, 2);
            std::cout << "Sent player state\n";
        }
        else if (command == "shoot") {
            net.SendPlayerShoot(5.0f, 1.0f, 7.0f, 1.0f, 0.0f, 0.0f, 1);
            std::cout << "Sent shoot event\n";
        }
        else if (command == "chat") {
            std::string msg;
            std::getline(iss, msg);
            if (!msg.empty() && msg[0] == ' ') {
                msg = msg.substr(1);
            }
            
            if (!msg.empty()) {
                net.SendChatMessage(msg);
            } else {
                std::cout << "Usage: chat <message>\n";
            }
        }
        else if (command == "start") {
            std::cout << "Only host can start the game\n";
        }
        else if (command == "list") {
            std::cout << "\nConnected players (" << net.GetPlayerCount() << "):\n";
            for (const auto& [id, client] : net.GetClients()) {
                std::cout << "  [" << id << "] " << client.name;
                if (id == net.GetLocalPlayerId()) {
                    std::cout << " (you)";
                }
                std::cout << "\n";
            }
        }
        else if (command == "stats") {
            std::cout << "\nNetwork Stats:\n";
            std::cout << "  Players: " << net.GetPlayerCount() << "\n";
            std::cout << "  Sent: " << net.GetPacketsSent() << " packets\n";
            std::cout << "  Received: " << net.GetPacketsReceived() << " packets\n";
        }
        else if (!command.empty()) {
            std::cout << "Unknown command. Type 'help' for commands.\n";
        }
    }
    
    running = false;
    updateThread.join();
    net.Disconnect();
}

int main(int argc, char* argv[]) {
    std::cout << "=== Placid Network Test ===\n";
    
    if (argc < 2) {
        std::cout << "\nUsage:\n";
        std::cout << "  Host:   " << argv[0] << " host [port]\n";
        std::cout << "  Client: " << argv[0] << " <host-ip> [port]\n";
        std::cout << "\nExamples:\n";
        std::cout << "  " << argv[0] << " host 7777\n";
        std::cout << "  " << argv[0] << " 127.0.0.1 7777\n";
        return 0;
    }
    
    std::string arg1 = argv[1];
    uint16_t port = 7777;
    
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }
    
    if (arg1 == "host") {
        RunHost(port);
    } else {
        RunClient(arg1, port);
    }
    
    return 0;
}
