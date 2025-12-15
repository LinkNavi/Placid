// simple_hero_test.cpp - Direct test of HERO library
// This tests if HERO itself works before using NetworkManager

#include "HERO.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "=== HERO Direct Test ===\n\n";
    
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " host\n";
        std::cout << "  " << argv[0] << " client\n";
        return 0;
    }
    
    std::string mode = argv[1];
    uint16_t port = 7777;
    
    if (mode == "host") {
        std::cout << "Starting HERO server on port " << port << "...\n";
        
        HERO::HeroServer server(port);
        server.Start();
        
        std::cout << "Server started. Waiting for connections...\n";
        std::cout << "Press Ctrl+C to stop\n\n";
        
        int messageCount = 0;
        while (true) {
            bool gotMessage = server.Poll([&](const HERO::Packet& pkt, 
                                             const std::string& host, 
                                             uint16_t port) {
                std::string msg(pkt.payload.begin(), pkt.payload.end());
                std::cout << "Received from " << host << ":" << port << " - " << msg << "\n";
                messageCount++;
                
                // Echo back
                std::string response = "Echo: " + msg;
                server.SendTo(response, host, port);
            });
            
            if (gotMessage) {
                std::cout << "Total messages: " << messageCount << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    else if (mode == "client") {
        std::cout << "Connecting to server at 127.0.0.1:" << port << "...\n";
        
        HERO::HeroClient client;
        
        if (!client.Connect("127.0.0.1", port)) {
            std::cerr << "Failed to connect!\n";
            std::cerr << "Make sure the server is running first.\n";
            return 1;
        }
        
        std::cout << "Connected!\n";
        std::cout << "Sending test messages...\n\n";
        
        // Send some test messages
        for (int i = 0; i < 5; i++) {
            std::string msg = "Test message " + std::to_string(i);
            client.Send(msg);
            std::cout << "Sent: " << msg << "\n";
            
            // Try to receive response
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            HERO::Packet pkt;
            if (client.Receive(pkt, 500)) {
                std::string response(pkt.payload.begin(), pkt.payload.end());
                std::cout << "Got response: " << response << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        std::cout << "\nTest complete. Disconnecting...\n";
        client.Disconnect();
    }
    else {
        std::cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }
    
    return 0;
}
