#include "nlohmann/json.hpp"
#include "../src/config.hpp"
#include "config.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "main.hpp"
#include "web-utils/shared/WebUtils.hpp"

#include <stdexcept>

WebUtils::StringResponse CreateRequest(std::string method, std::string URLPath, nlohmann::json jsonData) {
    std::thread([method, URLPath, jsonData]() -> WebUtils::StringResponse {
        const std::string getIp = getConfig().PCIPSetting.GetValue();
        const std::string getPort = getConfig().PortSetting.GetValue();

        const std::string URL = "http://" + getIp + ":" + getPort + URLPath;

        std::string jsonStr = jsonData.dump();

        WebUtils::URLOptions path{ URL };
        path.noEscape = true;
        
        std::span<const uint8_t> body(
            reinterpret_cast<const uint8_t*>(jsonStr.data()),
            jsonStr.size()
        );

        std::future<WebUtils::StringResponse> response;

        if (method == "GET") {
            response = WebUtils::GetAsync<WebUtils::StringResponse>(path);
        } else if (method == "POST") {
            response = WebUtils::PostAsync<WebUtils::StringResponse>(path, body);
        } else {
            throw std::runtime_error("Invalid method for request");
        }

        response.wait();

        auto responseValue = response.get();

        logger.info(
            "Attempted to send post request to {} with result of status code {}, and curl status being {}",
            path.fullURl(), 
            std::to_string(responseValue.get_HttpCode()), 
            std::to_string(responseValue.get_CurlStatus())
        );

        bool success = responseValue.IsSuccessful();
        if (!success) {
            logger.debug("Failed to get response");
        }

        return responseValue;
    }).detach();
}