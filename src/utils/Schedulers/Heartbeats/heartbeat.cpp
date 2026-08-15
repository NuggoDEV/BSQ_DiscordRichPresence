#include <thread>
#include "../src/utils/Requests/requests.hpp"
#include "main.hpp"

void HeartbeatLoop() {
    while (true) {
        nlohmann::json data;
        data["type"] = "HeartbeatReceiver";

        CreateRequest("POST", "/sendData", data);

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void Heartbeat() {
    std::thread(HeartbeatLoop).detach();
}