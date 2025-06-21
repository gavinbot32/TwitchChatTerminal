
#include "JsonSettings.h"
#include "ColorSystem.h"


/*{"vip", colorText("VP", "#af00af",true)},
{"broadcaster", colorText("BC", "#870000",true)},
{"subscriber", colorText("SB", "#af00af",true)},
{"turbo", colorText("TB", "#5f00af",true)},
{"staff", colorText("SF", "#875f5f",true)},
{"partner", colorText("PR", "#5f00ff",true)}*/



json default_badges = {
     {"moderator", {
         {"text","MD"},
         {"color","#005f00"},
         {"isBackground",true}
     }},
     {"vip", {
         {"text","VP"},
         {"color","#af00af"},
         {"isBackground",true}
     }},
     {"broadcaster", {
         {"text","BC"},
         {"color","#870000"},
         {"isBackground",true}
     }},
     {"subscriber", {
         {"text","SB"},
         {"color","#af00af"},
         {"isBackground",true}
     }},
     {"turbo", {
         {"text","TB"},
         {"color","#5f00af"},
         {"isBackground",true}
     }},
     {"staff", {
         {"text","SF"},
         {"color","#875f5f"},
         {"isBackground",true}
     }},
     {"partner", {
         {"text","PR"},
         {"color","#5f00ff"},
         {"isBackground",true}
     }}
};

//---Global Variables---
std::unordered_map<std::string, std::string> badges;

//---Class Variables---
std::unordered_map<std::string, ConfigManager> JsonSettings::jsonFiles;

std::unordered_map<std::string, std::unordered_map<std::string, std::string>> JsonSettings::highlights;

void loadBadges(){
    //If user-settings.json exists
    if(JsonSettings::jsonFiles.find("user-settings") != JsonSettings::jsonFiles.end()){
        ConfigManager& uSettings = JsonSettings::jsonFiles["user-settings"];
        auto badgesList = uSettings.get<json>("badges", {});
        if(badgesList.empty()){
            uSettings.set("badges", default_badges);
            uSettings.saveConfig();
            badgesList = std::ref(default_badges);
        }
        for(auto& badge : badgesList.items()){
            std::string badgeKey = badge.key();
            std::string badgeValue = colorText(badge.value()["text"], badge.value()["color"], badge.value()["isBackground"]);
            badges.emplace(badgeKey, badgeValue);
        }
    }
}

void initializeChannelColor(){
    if(JsonSettings::jsonFiles.find("user-settings") != JsonSettings::jsonFiles.end()){
        ConfigManager& uSettings = JsonSettings::jsonFiles["user-settings"];
        if(!uSettings.hasKey("channel_color")){
            uSettings.set("channel_color", "#800000");
            uSettings.saveConfig();
        }
    }
}


/* Highlight Json Example
 * "highlights" : {
 *  "gavinbot32" : {
 *      "type" : "user",
 *      "color" : "#0000ff"
 *  },
 *  "partner" : {
 *      "type" : "badge",
 *      "color" : #880088
 *  }
 */

void initializeHighlights(){
    if(JsonSettings::jsonFiles.find("user-settings") == JsonSettings::jsonFiles.end()) return;
    ConfigManager& uSettings = JsonSettings::jsonFiles["user-settings"];
    json highlightsJson = uSettings.get<json>("highlights", {});
    if(highlightsJson.empty()){
        uSettings.set("highlights", json::object());;
        uSettings.saveConfig();
        highlightsJson = {};
    }

    for(auto& highlight : highlightsJson.items()){
        std::unordered_map<std::string, std::string> highlightMap = highlight.value();
        JsonSettings::highlights.emplace(highlight.key(), highlightMap);
    }
}

void JsonSettings::initializeJsonFiles() {
    std::cout << "Initializing JSON files..." << std::endl;
    JsonSettings::jsonFiles.emplace("user-settings", ConfigManager("user-settings.json"));
    JsonSettings::jsonFiles.emplace("credentials", ConfigManager("credentials.json"));
    std::cout << "Config Directory: " << JsonSettings::jsonFiles["user-settings"].getConfigDir() << std::endl;

    loadBadges();
    initializeChannelColor();
    initializeHighlights();

}

std::unordered_map<std::string, ConfigManager> &JsonSettings::getJsonFiles() {
    return jsonFiles;
}


