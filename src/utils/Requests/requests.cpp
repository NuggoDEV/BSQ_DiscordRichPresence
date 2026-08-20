#include "nlohmann/json.hpp"
#include "../src/config.hpp"
#include "config.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "main.hpp"
#include "web-utils/shared/WebUtils.hpp"

#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <vector>
#include <memory>

std::future<WebUtils::JsonResponse> CreateRequest(
    std::string method,
    std::string URLPath,
    nlohmann::json jsonData
) {
    // Single-sender queue: enqueue the request and return a future
    struct Task {
        std::string method;
        std::string urlPath;
        nlohmann::json data;
        std::promise<WebUtils::JsonResponse> promise;
    };

    static std::mutex queueMutex;
    static std::condition_variable queueCv;
    static std::deque<std::unique_ptr<Task>> queue;
    static std::atomic<bool> workerStarted{false};

    // Worker that processes tasks sequentially
    auto ensureWorker = []() {
        static std::mutex startMutex;
        if (workerStarted.load(std::memory_order_acquire)) return;
        std::lock_guard<std::mutex> lk(startMutex);
        if (workerStarted.load(std::memory_order_acquire)) return;
        std::thread([]() {
            while (true) {
                std::unique_ptr<Task> task;
                {
                    std::unique_lock<std::mutex> lk(queueMutex);
                    queueCv.wait(lk, [] { return !queue.empty(); });
                    task = std::move(queue.front());
                    queue.pop_front();
                }

                try {
                    // Build URL: if urlPath already starts with http, use it directly
                    std::string URL;
                    if (task->urlPath.rfind("http", 0) == 0) {
                        URL = task->urlPath;
                    } else {
                        const std::string getIp = getConfig().PCIPSetting.GetValue();
                        const std::string getPort = getConfig().PortSetting.GetValue();
                        URL = "http://" + getIp + ":" + getPort + task->urlPath;
                    }

                    std::string jsonStr = task->data.dump();

                    // Log the JSON body from the worker thread
                    logger.info("Request JSON: {}", jsonStr);

                    WebUtils::URLOptions path{URL};
                    path.noEscape = true;

                    // Copy JSON body into a heap buffer so the span remains valid
                    // for the duration of the async request handling.
                    std::shared_ptr<std::vector<uint8_t>> bodyBuf;

                    std::future<WebUtils::JsonResponse> response;
                    if (task->method == "GET") {
                        response = WebUtils::GetAsync<WebUtils::JsonResponse>(path);
                    } else if (task->method == "POST") {
                        bodyBuf = std::make_shared<std::vector<uint8_t>>(jsonStr.begin(), jsonStr.end());
                        std::span<const uint8_t> body(bodyBuf->data(), bodyBuf->size());
                        response = WebUtils::PostAsync<WebUtils::JsonResponse>(path, body);
                    } else {
                        throw std::runtime_error("Invalid method");
                    }

                    // Fulfill the task promise asynchronously so the worker isn't blocked.
                    // Capture `bodyBuf` to keep the buffer alive until the response is retrieved.
                    std::promise<WebUtils::JsonResponse> prom = std::move(task->promise);
                    std::shared_ptr<std::vector<uint8_t>> keepAlive = bodyBuf;
                    std::thread([
                        prom = std::move(prom),
                        resp = std::move(response),
                        keepAlive
                    ]() mutable {
                        try {
                            prom.set_value(resp.get());
                        } catch (...) {
                            prom.set_exception(std::current_exception());
                        }
                    }).detach();
                } catch (...) {
                    task->promise.set_exception(std::current_exception());
                }
            }
        }).detach();
        workerStarted.store(true, std::memory_order_release);
    };

    ensureWorker();

    auto t = std::make_unique<Task>();
    t->method = std::move(method);
    t->urlPath = std::move(URLPath);
    t->data = std::move(jsonData);
    auto fut = t->promise.get_future();

    {
        std::lock_guard<std::mutex> lk(queueMutex);
        queue.push_back(std::move(t));
    }
    queueCv.notify_one();

    return fut;
}

std::future<WebUtils::JsonResponse> GetLatestGithub() {
    // Enqueue the github latest release request to preserve ordering
    nlohmann::json empty;
    return CreateRequest("GET", "https://api.github.com/repos/RainzDev/BeatSaberBridgeAPI.CPP/releases/latest", empty);
}
