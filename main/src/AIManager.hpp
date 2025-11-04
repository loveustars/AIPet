/**
 * @file AIManager.hpp
 */
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

    // Sends a message in a separate thread. Non-blocking.
    void sendMessage(const std::string& userInput);

    // Checks if the AI is currently processing a request.
    bool isBusy() const;

    // Gets the latest result from the AI. Returns empty string if no new result.
    std::string getResult();

private:
    // This is the function that will run in the background thread.
    void performRequest(const std::string userInput);

    // libcurl's callback function must be static.
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
