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
#include <deque>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct AIResponse {
    uint64_t requestId = 0;
    bool success = false; // true => text valid, false => error
    std::string text;     // valid if success==true
    std::string errorText; // valid if success==false
    int code = 0; // optional (e.g., curl code or http code)
    std::chrono::steady_clock::time_point ts;
};

class AIManager {
public:
    AIManager(std::string apiKey);
    ~AIManager();

    // 在独立线程中发送消息。返回本次请求的 requestId
    uint64_t sendMessage(const std::string& userInput);

    // 检查 AI 当前是否正在处理请求。
    bool isBusy() const;

    // 轮询接口：弹出一个成功响应（若有），返回 true 并把响应赋值到 out
    bool popAIResponse(AIResponse &out);

    // 轮询接口：弹出一个错误响应（若有），返回 true 并把响应赋值到 out
    bool popErrorResponse(AIResponse &out);

    // 兼容旧接口（返回空字符串）
    std::string getResult();

private:
    // 将在后台线程中运行的请求执行函数。
    void performRequest(const std::string userInput, uint64_t requestId);

    // libcurl 的回调函数必须为静态函数。
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    std::string apiKey;
    std::string modelUrl;
    json conversationHistory;

    // Threading and State Management
    std::thread workerThread;

    std::mutex respMutex;
    std::deque<AIResponse> respQueue; // 成功响应

    std::mutex errMutex;
    std::deque<AIResponse> errQueue;  // 错误响应

    std::atomic<bool> isProcessing;
    std::atomic<uint64_t> lastRequestId;
};

#endif // AI_MANAGER_HPP
