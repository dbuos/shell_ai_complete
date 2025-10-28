#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"' || c == '\\') o << '\\';
        else if (c == '\n') { o << "\\n"; continue; }
        else if (c == '\r') { o << "\\r"; continue; }
        else if (c == '\t') { o << "\\t"; continue; }
        o << c;
    }
    return o.str();
}

std::string call_llm(const std::string& command_line) {
    const char* api_key = std::getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        std::cerr << "Error: ANTHROPIC_API_KEY not set" << std::endl;
        return "";
    }

    std::string system_prompt = "You complete shell commands. Return ONLY the complete command, no explanations.\\n\\n"
                                "Examples:\\n"
                                "Input: list all files\\n"
                                "Output: ls -la\\n\\n"
                                "Input: find pdf files\\n"
                                "Output: find . -name \\\"*.pdf\\\"\\n\\n"
                                "Input: compress logs\\n"
                                "Output: tar -czf logs.tar.gz *.log";

    std::string user_prompt = "Input: " + command_line + "\\nOutput:";

    std::string json_payload = "{"
        "\"model\": \"claude-haiku-4-5-20251001\","
        "\"max_tokens\": 150,"
        "\"system\": \"" + system_prompt + "\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"" + escape_json(user_prompt) + "\"}]"
        "}";

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        std::cerr << "pipe failed" << std::endl;
        return "";
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "fork failed" << std::endl;
        return "";
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execlp("curl", "curl", "-s",
               "https://api.anthropic.com/v1/messages",
               "-H", "Content-Type: application/json",
               "-H", ("x-api-key: " + std::string(api_key)).c_str(),
               "-H", "anthropic-version: 2023-06-01",
               "-d", json_payload.c_str(),
               nullptr);
        exit(1);
    } else {
        close(pipefd[1]);

        std::string response;
        char buffer[4096];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            response.append(buffer, count);
        }
        close(pipefd[0]);

        waitpid(pid, nullptr, 0);
        return response;
    }

    return "";
}

std::string extract_text(const std::string& response) {
    size_t text_pos = response.find("\"text\":");
    if (text_pos != std::string::npos) {
        size_t start = response.find('"', text_pos + 7);
        if (start != std::string::npos) {
            size_t end = response.find('"', start + 1);
            if (end != std::string::npos) {
                return response.substr(start + 1, end - start - 1);
            }
        }
    }
    return "";
}

int main() {
    std::cout << "Testing LLM invocation with Claude Haiku 4.5...\n" << std::endl;

    std::cout << "Test 1: Simple completion" << std::endl;
    std::cout << "Input: 'find all pdf files'" << std::endl;
    std::string response1 = call_llm("find all pdf files");
    std::string result1 = extract_text(response1);
    std::cout << "Result: " << result1 << std::endl;
    std::cout << std::endl;

    std::cout << "Test 2: Command correction" << std::endl;
    std::cout << "Input: 'list files in current dir'" << std::endl;
    std::string response2 = call_llm("list files in current dir");
    std::string result2 = extract_text(response2);
    std::cout << "Result: " << result2 << std::endl;
    std::cout << std::endl;

    std::cout << "Test 3: Complex command" << std::endl;
    std::cout << "Input: 'compress all log files'" << std::endl;
    std::string response3 = call_llm("compress all log files");
    std::string result3 = extract_text(response3);
    std::cout << "Result: " << result3 << std::endl;
    std::cout << std::endl;

    if (!result1.empty() && !result2.empty() && !result3.empty()) {
        std::cout << "All tests passed! LLM is working correctly." << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed. Check API key and connectivity." << std::endl;
        std::cout << "\nRaw response from last test:" << std::endl;
        std::cout << response3 << std::endl;
        return 1;
    }
}
