#include "AIManager.hpp"
#include <iostream>
#include <curl/curl.h>

// Static callback function for libcurl
size_t AIManager::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AIManager::AIManager(std::string key) : apiKey(std::move(key)), isProcessing(false), hasNewResult(false) {
    // URL-encode the API key once during initialization
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
    // If a thread is running, we should wait for it to finish
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void AIManager::sendMessage(const std::string& userInput) {
    if (isProcessing) {
        std::cout << "[AIManager] Still processing previous request. Please wait." << std::endl;
        return;
    }

    // Wait for the previous thread to finish, just in case
    if (workerThread.joinable()) {
        workerThread.join();
    }

    // Start a new request in a background thread
    isProcessing = true;
    workerThread = std::thread(&AIManager::performRequest, this, userInput);
}

bool AIManager::isBusy() const {
    return isProcessing;
}

std::string AIManager::getResult() {
    if (hasNewResult) {
        std::lock_guard<std::mutex> lock(resultMutex);
        hasNewResult = false; // Mark result as "read"
        return lastResult;
    }
    return "";
}

void AIManager::performRequest(const std::string userInput) {
    // *** CRITICAL FIX: The JSON body should NOT be URL-encoded. ***
    // We add the raw user input to the history. nlohmann::json handles JSON-specific escaping.
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
                // Add model's response to history for context in the next turn
                conversationHistory.push_back({
                    {"role", "model"},
                    {"parts", {{{"text", responseMessage}}}}
                });
            } catch (const json::exception& e) {
                responseMessage = "Error: JSON parsing failed: " + std::string(e.what());
            }
        }
        
        // Safely store the result
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
    
    // Signal that we are done and have a new result
    hasNewResult = true;
    isProcessing = false;
}