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

AIManager::AIManager(std::string key) : apiKey(std::move(key)), isProcessing(false), hasNewResult(false) {
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

void AIManager::sendMessage(const std::string& userInput) {
    if (isProcessing) {
        std::cout << "[AIManager] Still processing previous request. Please wait." << std::endl;
        return;
    }

    // 为防万一，等待前一个线程结束
    if (workerThread.joinable()) {
        workerThread.join();
    }

    // 在后台线程中启动新请求
    isProcessing = true;
    workerThread = std::thread(&AIManager::performRequest, this, userInput);
}

bool AIManager::isBusy() const {
    return isProcessing;
}

std::string AIManager::getResult() {
    if (hasNewResult) {
        std::lock_guard<std::mutex> lock(resultMutex);
        hasNewResult = false; // 标记结果为“已读取”
        return lastResult;
    }
    return "";
}

void AIManager::performRequest(const std::string userInput) {
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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        std::string responseMessage;
        if (res != CURLE_OK) {
            responseMessage = "Error: curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
        } else if (httpCode != 200) {
            responseMessage = "Error: HTTP request failed with code " + std::to_string(httpCode) + ". Response: " + readBuffer;
        } else {
            try {
                json response = json::parse(readBuffer);
                responseMessage = response["candidates"][0]["content"]["parts"][0]["text"];
                // 将模型的回复加入历史，以便下一轮使用
                conversationHistory.push_back({
                    {"role", "model"},
                    {"parts", {{{"text", responseMessage}}}}
                });
            } catch (const json::exception& e) {
                responseMessage = "Error: JSON parsing failed: " + std::string(e.what());
            }
        }
        
        // 安全地存储结果
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            lastResult = responseMessage;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
         {
            std::lock_guard<std::mutex> lock(resultMutex);
            lastResult = "Error: Failed to initialize libcurl.";
        }
    }
    
    // 标记已完成并有新结果
    hasNewResult = true;
    isProcessing = false;
}
