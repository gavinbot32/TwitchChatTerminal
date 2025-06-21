#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "TwitchChat.h"

void parseAndPrintMessage(const std::string& line, bool isTyping, TwitchChat& chat);

std::vector<std::string> parseBadges(const std::string& badgesStr);

std::unordered_map<std::string, std::string> parseTags(const std::string& line);
