/**
 * @file AIManager.cpp
 * AI 管理器的实现
 */
#include "AIManager.hpp"
#include <iostream>
#include <curl/curl.h>

// 静态回调函数
size_t AIManager::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AIManager::AIManager(std::string key) : apiKey(std::move(key)), isProcessing(false), lastRequestId(0) {
    // 在初始化期间只对 API 密钥进行一次 URL 编码
    CURL* curl = curl_easy_init();
    if (curl) {
        char* escapedKey = curl_easy_escape(curl, apiKey.c_str(), 0);
        if (escapedKey) {
            modelUrl = "https://generativelanguage.googleapis.com/v1/models/gemini-2.5-flash:generateContent?key=" + std::string(escapedKey);
            curl_free(escapedKey);
        }
        curl_easy_cleanup(curl);
    }
}

AIManager::~AIManager() {
    // 如果线程仍在运行，应该等待它结束
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

uint64_t AIManager::sendMessage(const std::string& userInput) {
    if (isProcessing) {
        std::cout << "[AIManager] Still processing previous request. Please wait." << std::endl;
        return 0;
    }

    // 为防万一，等待前一个线程结束
    if (workerThread.joinable()) {
        workerThread.join();
    }

    // 生成新的 requestId
    uint64_t reqId = ++lastRequestId;

    // 在后台线程中启动新请求
    isProcessing = true;
    workerThread = std::thread(&AIManager::performRequest, this, userInput, reqId);
    return reqId;
}

bool AIManager::isBusy() const {
    return isProcessing;
}

std::string AIManager::getResult() {
    // 兼容旧接口：如果有成功响应则返回并移除第一个的 text
    std::lock_guard<std::mutex> lk(respMutex);
    if (!respQueue.empty()) {
        AIResponse r = respQueue.front();
        respQueue.pop_front();
        return r.text;
    }
    return std::string();
}

bool AIManager::popAIResponse(AIResponse &out) {
    std::lock_guard<std::mutex> lk(respMutex);
    if (!respQueue.empty()) {
        out = respQueue.front();
        respQueue.pop_front();
        return true;
    }
    return false;
}

bool AIManager::popErrorResponse(AIResponse &out) {
    std::lock_guard<std::mutex> lk(errMutex);
    if (!errQueue.empty()) {
        out = errQueue.front();
        errQueue.pop_front();
        return true;
    }
    return false;
}

void AIManager::performRequest(const std::string userInput, uint64_t requestId) {
    // 关键修复：JSON 主体不应进行 URL 编码。
    // 我们将原始用户输入加入历史。nlohmann::json 会处理 JSON 特定的转义。
    conversationHistory.push_back({
        {"role", "user"},
        {"parts", {{{"text", userInput}}}}
    });

    json requestData = {{"contents", conversationHistory}};
    std::string postData = requestData.dump();
    std::string readBuffer;

    CURL* curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, modelUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    // 设置连接与传输超时，避免长时间阻塞
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        std::string responseMessage;
        AIResponse resp;
        resp.requestId = requestId;
        resp.ts = std::chrono::steady_clock::now();

        if (res != CURLE_OK) {
            resp.success = false;
            resp.errorText = std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res);
            resp.code = static_cast<int>(res);
            // push to error queue
            std::lock_guard<std::mutex> lk(errMutex);
            errQueue.push_back(std::move(resp));
        } else if (httpCode != 200) {
            resp.success = false;
            resp.errorText = std::string("HTTP request failed with code ") + std::to_string(httpCode) + ".";
            resp.code = static_cast<int>(httpCode);
            resp.errorText += std::string(" Response: ") + readBuffer;
            std::lock_guard<std::mutex> lk(errMutex);
            errQueue.push_back(std::move(resp));
        } else {
            try {
                json response = json::parse(readBuffer);
                std::string text = response["candidates"][0]["content"]["parts"][0]["text"];
                resp.success = true;
                resp.text = text;
                // 将模型的回复加入历史，以便下一轮使用
                conversationHistory.push_back({
                    {"role", "model"},
                    {"parts", {{{"text", resp.text}}}}
                });
                std::lock_guard<std::mutex> lk(respMutex);
                respQueue.push_back(std::move(resp));
            } catch (const json::exception& e) {
                resp.success = false;
                resp.errorText = std::string("JSON parsing failed: ") + e.what();
                std::lock_guard<std::mutex> lk(errMutex);
                errQueue.push_back(std::move(resp));
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        AIResponse resp;
        resp.requestId = requestId;
        resp.success = false;
        resp.errorText = "Failed to initialize libcurl.";
        resp.ts = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lk(errMutex);
        errQueue.push_back(std::move(resp));
    }
    
    // 标记已完成
    isProcessing = false;
}
