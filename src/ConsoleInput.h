#pragma once

#include <string>

std::string getLineWithTypingDetection(const std::string& channel, const std::string& username);

void toggleConsoleOutput();
void pauseConsoleOutput();
void resumeConsoleOutput();
bool isConsoleOutputPaused();
void flushBufferedMessages(); // Helper to print buffered messages
