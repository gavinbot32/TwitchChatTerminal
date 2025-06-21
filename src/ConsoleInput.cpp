#include "ConsoleInput.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <queue>

extern std::atomic<bool> isTyping; // declared elsewhere
extern std::mutex messageMutex;
extern std::queue<std::string> messageBuffer;

void toggleConsoleOutput() {
    if (isTyping) {
        resumeConsoleOutput();
    } else {
        pauseConsoleOutput();
    }
}

void pauseConsoleOutput() {
    isTyping = true;
}

void resumeConsoleOutput() {
    if(!isTyping) return;
    std::cout << std::endl;
    isTyping = false;
    flushBufferedMessages();

}

bool isConsoleOutputPaused() {
    return isTyping;
}

void flushBufferedMessages() {
    std::lock_guard<std::mutex> lock(messageMutex);
    while(!messageBuffer.empty()) {
        std::cout << messageBuffer.front() << std::endl;
        messageBuffer.pop();
    }
}


std::string getLineWithTypingDetection(const std::string& channel, const std::string& username) {
    std::string input;
    size_t cursor_pos = 0;
    resumeConsoleOutput();

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char ch;
    std::string escape_seq;
    bool in_escape_seq = false;

    static std::vector<std::string> history;
    static int historyIndex = -1;
    std::string inputBuffer;

    while (true) {
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            pauseConsoleOutput();

            if (ch == '\x1B') {
                in_escape_seq = true;
                escape_seq.clear();
                escape_seq += ch;
                continue;
            }

            if (in_escape_seq) {
                escape_seq += ch;
                if (escape_seq.length() == 3 && escape_seq[1] == '[') {
                    char dir = escape_seq[2];
                    if (dir == 'D' && cursor_pos > 0) {
                        cursor_pos--;
                    } else if (dir == 'C' && cursor_pos < input.length()) {
                        cursor_pos++;
                    } else if (dir == 'A') { // up
                        if (!history.empty() && historyIndex + 1 < (int)history.size()) {
                            historyIndex++;
                            input = history[history.size() - 1 - historyIndex];
                            cursor_pos = input.length();
                        }
                    } else if (dir == 'B') { // down
                        if (historyIndex > 0) {
                            historyIndex--;
                            input = history[history.size() - 1 - historyIndex];
                            cursor_pos = input.length();
                        } else if (historyIndex == 0) {
                            historyIndex = -1;
                            input.clear();
                            cursor_pos = 0;
                        }
                    }

                    std::cout << "\r\033[K> " << input << std::flush;
                    if (cursor_pos < input.length()) {
                        std::cout << "\x1B[" << (input.length() - cursor_pos) << "D" << std::flush;
                    }

                    in_escape_seq = false;
                    continue;
                }
                continue;
            }

            if (ch == '\n') {
                std::cout << "\r\x1B[K" << std::flush;
                std::cout << "\x1B[1A" << std::flush;
                if (!input.empty()) {
                    history.push_back(input);
                }
                historyIndex = -1;
                break;
            } else if (ch == 127 || ch == '\b') {
                if (cursor_pos > 0) {
                    input.erase(cursor_pos - 1, 1);
                    cursor_pos--;
                    std::cout << "\r\033[K> " << input << std::flush;
                    if (cursor_pos < input.length()) {
                        std::cout << "\x1B[" << (input.length() - cursor_pos) << "D" << std::flush;
                    }
                }
            } else if (isprint(ch)) {
                input.insert(cursor_pos, 1, ch);
                cursor_pos++;
                std::cout << "\r\033[K> " << input << std::flush;
                if (cursor_pos < input.length()) {
                    std::cout << "\x1B[" << (input.length() - cursor_pos) << "D" << std::flush;
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, flags);
    resumeConsoleOutput();
    return input;
}

