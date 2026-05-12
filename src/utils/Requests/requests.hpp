#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "web-utils/shared/WebUtils.hpp"

WebUtils::JsonResponse CreateRequest(std::string method, std::string URLPath, nlohmann::json jsonData);