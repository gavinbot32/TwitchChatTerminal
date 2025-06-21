#include <string>
#include <unordered_map>
#include "ConfigManager.h"

#ifndef TWITCHCONSOLEVIEWER_JSONSETTINGS_H
#define TWITCHCONSOLEVIEWER_JSONSETTINGS_H


class JsonSettings {
private:

public:
    static std::unordered_map<std::string, ConfigManager> jsonFiles;
    static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> highlights;
    static void initializeJsonFiles();

    static std::unordered_map<std::string, ConfigManager> &getJsonFiles();

};


#endif //TWITCHCONSOLEVIEWER_JSONSETTINGS_H
