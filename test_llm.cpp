#include <iostream>
#include <string>
#include <cstdlib>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

// Callback for libcurl to write response data
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    ((std::string*)userp)->append((char*)contents, total_size);
    return total_size;
}

std::string call_llm_cerebras(const std::string& command_line) {
    const char* api_key = std::getenv("CEREBRAS_API_KEY");
    if (!api_key) {
        std::cerr << "Error: CEREBRAS_API_KEY not set" << std::endl;
        return "";
    }

    // Build JSON payload using nlohmann::json
    json request;
    request["model"] = "gpt-oss-120b";
    request["stream"] = false;
    request["max_tokens"] = 150;
    request["temperature"] = 1;
    request["top_p"] = 1;
    request["reasoning_effort"] = "medium";
    request["messages"] = json::array({
        {{"role", "system"}, {"content", "You complete shell commands. Return ONLY the complete command, no explanations.\n\n"
                                         "Examples:\n"
                                         "Input: list all files\n"
                                         "Output: ls -la\n\n"
                                         "Input: find pdf files\n"
                                         "Output: find . -name \"*.pdf\"\n\n"
                                         "Input: compress logs\n"
                                         "Output: tar -czf logs.tar.gz *.log"}},
        {{"role", "user"}, {"content", "Input: " + command_line + "\nOutput:"}}
    });

    std::string json_payload = request.dump();
    std::string response;

    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return "";
    }

    // Set curl options
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + std::string(api_key)).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.cerebras.ai/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Perform request
    CURLcode res = curl_easy_perform(curl);

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    // Parse JSON response using nlohmann::json (OpenAI-compatible format)
    try {
        auto j = json::parse(response);
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            if (j["choices"][0].contains("message") && j["choices"][0]["message"].contains("content")) {
                return j["choices"][0]["message"]["content"].get<std::string>();
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return "";
}

std::string call_llm(const std::string& command_line) {
    const char* api_key = std::getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        std::cerr << "Error: ANTHROPIC_API_KEY not set" << std::endl;
        return "";
    }

    // Build JSON payload using nlohmann::json
    json request;
    request["model"] = "claude-haiku-4-5-20251001";
    request["max_tokens"] = 150;
    request["system"] = "You complete shell commands. Return ONLY the complete command, no explanations.\n\n"
                        "Examples:\n"
                        "Input: list all files\n"
                        "Output: ls -la\n\n"
                        "Input: find pdf files\n"
                        "Output: find . -name \"*.pdf\"\n\n"
                        "Input: compress logs\n"
                        "Output: tar -czf logs.tar.gz *.log";
    request["messages"] = json::array({
        {{"role", "user"}, {"content", "Input: " + command_line + "\nOutput:"}}
    });

    std::string json_payload = request.dump();
    std::string response;

    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return "";
    }

    // Set curl options
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("x-api-key: " + std::string(api_key)).c_str());
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Perform request
    CURLcode res = curl_easy_perform(curl);

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    // Parse JSON response using nlohmann::json
    try {
        auto j = json::parse(response);
        if (j.contains("content") && j["content"].is_array() && !j["content"].empty()) {
            if (j["content"][0].contains("text")) {
                return j["content"][0]["text"].get<std::string>();
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return "";
}

int main() {
    bool has_anthropic = std::getenv("ANTHROPIC_API_KEY") != nullptr;
    bool has_cerebras = std::getenv("CEREBRAS_API_KEY") != nullptr;

    if (!has_anthropic && !has_cerebras) {
        std::cerr << "Error: Neither ANTHROPIC_API_KEY nor CEREBRAS_API_KEY is set" << std::endl;
        return 1;
    }

    bool all_passed = true;

    // Test Anthropic Claude
    if (has_anthropic) {
        std::cout << "=== Testing Anthropic Claude Haiku 4.5 ===\n" << std::endl;

        std::cout << "Test 1: Simple completion" << std::endl;
        std::cout << "Input: 'find all pdf files'" << std::endl;
        std::string result1 = call_llm("find all pdf files");
        std::cout << "Result: " << result1 << std::endl;
        std::cout << std::endl;

        std::cout << "Test 2: Command correction" << std::endl;
        std::cout << "Input: 'list files in current dir'" << std::endl;
        std::string result2 = call_llm("list files in current dir");
        std::cout << "Result: " << result2 << std::endl;
        std::cout << std::endl;

        std::cout << "Test 3: Complex command" << std::endl;
        std::cout << "Input: 'compress all log files'" << std::endl;
        std::string result3 = call_llm("compress all log files");
        std::cout << "Result: " << result3 << std::endl;
        std::cout << std::endl;

        if (result1.empty() || result2.empty() || result3.empty()) {
            std::cout << "Anthropic tests failed!" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Anthropic tests passed!" << std::endl;
        }
        std::cout << std::endl;
    }

    // Test Cerebras
    if (has_cerebras) {
        std::cout << "=== Testing Cerebras gpt-oss-120b ===\n" << std::endl;

        std::cout << "Test 1: Simple completion" << std::endl;
        std::cout << "Input: 'find all pdf files'" << std::endl;
        std::string result1 = call_llm_cerebras("find all pdf files");
        std::cout << "Result: " << result1 << std::endl;
        std::cout << std::endl;

        std::cout << "Test 2: Command correction" << std::endl;
        std::cout << "Input: 'list files in current dir'" << std::endl;
        std::string result2 = call_llm_cerebras("list files in current dir");
        std::cout << "Result: " << result2 << std::endl;
        std::cout << std::endl;

        std::cout << "Test 3: Complex command" << std::endl;
        std::cout << "Input: 'compress all log files'" << std::endl;
        std::string result3 = call_llm_cerebras("compress all log files");
        std::cout << "Result: " << result3 << std::endl;
        std::cout << std::endl;

        if (result1.empty() || result2.empty() || result3.empty()) {
            std::cout << "Cerebras tests failed!" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Cerebras tests passed!" << std::endl;
        }
        std::cout << std::endl;
    }

    if (all_passed) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed. Check API keys and connectivity." << std::endl;
        return 1;
    }
}
