#/**
# * @file AIManager.hpp
# */
#ifndef AI_MANAGER_HPP
#define AI_MANAGER_HPP

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class AIManager {
public:
    AIManager(std::string apiKey);
    ~AIManager();

    // 在独立线程中发送消息。非阻塞。
    void sendMessage(const std::string& userInput);

    // 检查 AI 当前是否正在处理请求。
    bool isBusy() const;

    // 获取 AI 的最新结果。如果没有新结果，则返回空字符串。
    std::string getResult();

private:
    // 将在后台线程中运行的请求执行函数。
    void performRequest(const std::string userInput);

    // libcurl 的回调函数必须为静态函数。
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    std::string apiKey;
    std::string modelUrl;
    json conversationHistory;

    // Threading and State Management
    std::thread workerThread;
    std::mutex resultMutex;
    std::string lastResult;
    std::atomic<bool> isProcessing;
    std::atomic<bool> hasNewResult;
};

#endif // AI_MANAGER_HPP
