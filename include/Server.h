
#pragma once

#include "HERO.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

class Server {
public:
    explicit Server(uint16_t port)
        : server(port), port(port), running(false) {}

    ~Server() {
        Stop();
    }

    void Start() {
        if (running.load())
            return;

        server.Start();
        running.store(true);

        worker = std::thread([this]() { Run(); });

        std::cout << "Server running on port " << port << "\n";
    }

    void Stop() {
        if (!running.load())
            return;

        running.store(false);

        if (worker.joinable())
            worker.join();
    }

private:
    void Run() {
        while (running.load()) {
            server.Poll([this](const HERO::Packet& pkt,
                              const std::string& host,
                              uint16_t port)
            {
                std::string msg(pkt.payload.begin(), pkt.payload.end());
                std::cout << "Received: " << msg
                          << " from " << host << ":" << port << "\n";

                server.SendTo("Echo: " + msg, host, port);
            });

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

private:
    HERO::HeroServer server;
    uint16_t port;

    std::thread worker;
    std::atomic<bool> running;
};
