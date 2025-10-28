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

    std::string prompt = "You are a shell command completion assistant. "
                        "Given a partial command, suggest the most likely completion or correction. "
                        "Respond with ONLY the complete command, nothing else. "
                        "Current partial command: " + command_line;

    std::string json_payload = "{"
        "\"model\": \"claude-3-5-sonnet-20241022\","
        "\"max_tokens\": 1024,"
        "\"messages\": [{\"role\": \"user\", \"content\": \"" + escape_json(prompt) + "\"}]"
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

        // Parse JSON response to extract text
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
    }

    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <partial_command>" << std::endl;
        return 1;
    }

    std::string command_line;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) command_line += " ";
        command_line += argv[i];
    }

    std::string completion = call_llm(command_line);
    if (!completion.empty()) {
        std::cout << completion << std::endl;
    }

    return 0;
}
