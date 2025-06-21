#include "MessageParser.h"
#include <iostream>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <sstream>
#include "ColorSystem.h"
#include "TwitchChat.h"
#include "JsonSettings.h"

// These could eventually be passed in or wrapped in a context object.
extern std::mutex messageMutex;
extern std::queue<std::string> messageBuffer;

bool rawMode = false;

/*{{"moderator", colorText("MD", "#005f00",true)},
{"vip", colorText("VP", "#af00af",true)},
{"broadcaster", colorText("BC", "#870000",true)},
{"subscriber", colorText("SB", "#af00af",true)},
{"turbo", colorText("TB", "#5f00af",true)},
{"staff", colorText("SF", "#875f5f",true)},
{"partner", colorText("PR", "#5f00ff",true)},};*/

extern std::unordered_map<std::string, std::string> badges;


std::unordered_map<std::string, std::string> parseTags(const std::string& line) {
    std::unordered_map<std::string, std::string> tagMap;

    //If the line is empty or doesn't start with @ return an empty map
    if(line.empty() || line[0] != '@') return tagMap;

    size_t endOfTags = line.find(' ');
    std::string tagSection = line.substr(1, endOfTags - 1); //Remove the @ symbol
    size_t startOfMessage = endOfTags + 1;

    std::stringstream tagStream(tagSection);
    std::string tagPair;

    while(std::getline(tagStream, tagPair, ';')){
        size_t eqPos = tagPair.find('=');
        if(eqPos != std::string::npos){
            std::string key = tagPair.substr(0, eqPos);
            std::string value = tagPair.substr(eqPos + 1);
            tagMap[key] = value;
        } else {
            tagMap[tagPair] = "";
        }
    }
    tagMap["message"] = line.substr(startOfMessage);
    return tagMap;
}

std::vector<std::string> parseBadges(const std::string& badgesStr) {
    std::vector<std::string> badges;
    std::istringstream stream(badgesStr);
    std::string badge;

    // Split by comma
    while (std::getline(stream, badge, ',')) {
        // Find the position of the '/' character
        size_t slashPos = badge.find('/');
        if (slashPos != std::string::npos) {
            // Extract only the badge name (everything before the '/')
            badges.push_back(badge.substr(0, slashPos));
        }
    }

    return badges;
}


void parseAndPrintMessage(const std::string& line, bool isTyping, TwitchChat& chat) {
    //std::cout << colorText("Parse And Print: ", "#101010",true) + line << std::endl;
    // First check if it's a server message (starts with :tmi.twitch.tv)
    if (line.find(":tmi.twitch.tv") != std::string::npos) {
        // If it's an error message (like 421)
        if (line.find(" 421 ") != std::string::npos) {
            if(rawMode){
                std::cerr << colorText("Server Error: " + line, "#ff0000") << std::endl;
            }
            return;
        }

        // Other server messages
        if (rawMode) {
            std::cerr << colorText("Server: " + line, "#3f3f3f") << std::endl;
        }
        return;
    }

    // Handle PRIVMSG
    if (line.find("PRIVMSG") != std::string::npos) {
        try {
            std::unordered_map<std::string, std::string> tags = parseTags(line);

            // Check if message exists in tags
            if (tags.find("message") == tags.end()) {
                std::cerr << "Error: No message field in tags" << std::endl;
                std::cerr << colorText("Raw message: " + line, "#3f3f3f") << std::endl;
                return;
            }

            const std::string& line_message = tags["message"];
            if (line_message.empty()) {
                return;
            }

            // Find and validate all positions before using them
            size_t start = line_message.find(':');
            size_t exclam = line_message.find('!');
            if (start == std::string::npos || exclam == std::string::npos || start >= exclam) {
                return;
            }

            std::string user = line_message.substr(start + 1, exclam - start - 1);

            size_t msg_start = line_message.find(':', exclam + 1);
            if (msg_start == std::string::npos) {
                return;
            }

            std::string message = line_message.substr(msg_start + 1);

            size_t chnl_start = line_message.find('#');
            if (chnl_start == std::string::npos || chnl_start >= msg_start) {
                return;
            }

            std::string channel = line_message.substr(chnl_start, msg_start - chnl_start);


            std::vector<std::string> userBadges;

            std::string badgeStr = "";
            if (tags.find("badges") != tags.end()) {
                userBadges = parseBadges(tags["badges"]);
                for (const auto& userBadge : userBadges) {
                    if (badges.find(userBadge) != badges.end()) {
                        if(!badgeStr.empty()) {
                            badgeStr += "\u2009";
                        }
                        badgeStr += badges[userBadge];
                    }
                }
                if(!badgeStr.empty()) badgeStr += " ";
            }


            // Highlight color if the badge is a highlight
            std::string highlightColor;

            if(!userBadges.empty()){
                for(auto& userBadge : userBadges){
                    if(JsonSettings::highlights.find(userBadge) != JsonSettings::highlights.end()){
                        std::unordered_map<std::string, std::string> highlight = JsonSettings::highlights[userBadge];
                        if(highlight.find("type") != highlight.end() && highlight["type"] == "badge"){
                            if(highlight.find("color") != highlight.end()){
                                highlightColor = highlight["color"];
                            }
                        }
                    }
                }
            }



            // Make sure display-name and color exist in tags before using them
            std::string displayName = tags.find("display-name") != tags.end() ? tags["display-name"] : user;
            std::string color = tags.find("color") != tags.end() ? tags["color"] : "#FFFFFF";

            // Highlight color if the user is a highlight *user takes priority over badge*
            if(JsonSettings::highlights.find(displayName) != JsonSettings::highlights.end()){
                std::unordered_map<std::string, std::string> highlight = JsonSettings::highlights[displayName];
                if(highlight.find("type") != highlight.end() && highlight["type"] == "user"){
                    if(highlight.find("color") != highlight.end()){
                        highlightColor = highlight["color"];
                    }
                }
            }

            //Put the message together.
            std::string msg;
            if(!highlightColor.empty()){
                msg = rawMode ? line :
                                  colorText(channel, chat.getChannelColor()) + " " +
                                  badgeStr +
                                  colorText(colorText(displayName + ": ", color,false),highlightColor,true)+
                                  colorText(message,highlightColor,true);

            }
            else {
                 msg = rawMode ? line :
                                  colorText(channel, chat.getChannelColor()) + " " +
                                  badgeStr +
                                  colorText(displayName + ": ", color) +
                                  message;
            }

            if (isTyping) {
                std::lock_guard<std::mutex> lock(messageMutex);
                messageBuffer.push(msg);
            } else {
                std::cout << msg << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        }
    }
}