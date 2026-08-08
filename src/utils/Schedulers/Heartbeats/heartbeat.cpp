#include "../src/utils/Requests/requests.hpp"
#include "main.hpp"

void Heartbeat() {
    while (true) {
        nlohmann::json data;
        data["type"] = "Heartbeat";

        logger.debug("Heartbeat sent to server");

        CreateRequest("POST", "/sendData", data);

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}