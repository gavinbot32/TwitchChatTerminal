#include <asio.hpp>
#include <string>
#include <iostream>
#include <atomic>
#include <mutex>
#include <queue>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl/stream_base.hpp>
#include "ConsoleInput.h"
#include "TwitchChat.h"
#include "CommandRegistry.h"
#include "Command.h"
#include "ColorSystem.h"
#include "ConfigManager.h"
#include "JsonSettings.h"
#include "MessageParser.h"

std::atomic<bool> isTyping = false;
std::mutex messageMutex;
std::queue<std::string> messageBuffer;
extern bool rawMode;

extern std::unordered_map<std::string, std::string> badges;

using asio::ip::tcp;

//Use https://twitchtokengenerator.com to generate tokens.





class QuitCommand : public Command {
    TwitchChat& chat;
public:
    explicit QuitCommand(const TwitchChat& chat) : chat(const_cast<TwitchChat &>(chat)){}

    void execute(const std::vector<std::string> &args) override {
        std::cout << colorText("Shutting down...", "#5f0000") << std::endl;
        chat.disconnect();
        exit(0);
    }

    std::string getDescription() override{
        return "Quits the program";
    }

};

class ClearCommand : public Command {
public:
    void execute(const std::vector<std::string> &args) override {
        std::cout << std::string(80, '\n');
    }

    std::string getDescription() override{
        return "Clears the console";
    }

};

class HelpCommand : public Command {
    CommandRegistry registry;
public:
    explicit HelpCommand(const CommandRegistry& registry) : registry(const_cast<CommandRegistry &>(registry)) {}

    void execute(const std::vector<std::string> &args) override {
        std::cout << std::endl;
        std::cout << "Help Menu - prefix commands with '/':" << std::endl;
        std::cout << " ______________________________________________" << std::endl;
        std::cout << "|" << std::endl;
        //Help isn't added to the registry yet, so it's handled separately.
        std::cout << "| help - " << getDescription() << std::endl;

        for(auto& command : registry.getCommands()){
            std::cout << "| " << command.first << " - " << command.second->getDescription() << std::endl;
        }
        std::cout << "|______________________________________________" << std::endl;
        std::cout << "Press Enter to resume chat."
        << std::endl;

        pauseConsoleOutput();

        std::string dummy;
        std::getline(std::cin, dummy);

        resumeConsoleOutput();
    }

    std::string getDescription() override{
        return "Displays a list of commands and their descriptions.";
    }

};

class JoinCommand : public Command {
    TwitchChat& chat;
public:
    explicit JoinCommand(const TwitchChat& chat) : chat(const_cast<TwitchChat &>(chat)){

    }

    void execute(const std::vector<std::string> &args) override {
        if(args.size() < 1){
            std::cerr << "Usage: /join <channel>" << std::endl;
            return;
        }
        chat.joinChannel(args[0]);
    }
    std::string getDescription() override{
        return "Joins a channel.";
    }
};

class RawModeCommand : public Command {

    void execute(const std::vector<std::string> &args) override {
        rawMode = !rawMode;
        std::cout << "Raw mode is now " << (rawMode ? "enabled" : "disabled") << std::endl;
    }

    std::string getDescription() override{
        return "Toggles receiving messages from the Twitch IRC in raw format (may be unreadable).";
    }
};

class BadgeListCommand : public Command {

    void execute(const std::vector<std::string> &args) override {
        if(badges.empty()){
            std::cout << "No badges found." << std::endl;
            return;
        }
        std::cout << "Badges:" << std::endl;
        for(auto& badge : badges){
            std::cout << badge.first << ": " << badge.second << std::endl;
        }
        std::cout << "\nPress Enter to resume chat."
                  << std::endl;

        pauseConsoleOutput();

        std::string dummy;
        std::getline(std::cin, dummy);

        resumeConsoleOutput();
    }

    std::string getDescription() override{
        return "Displays a list of badges.";
    }


};

class DebugCommand : public Command {
public:
    TwitchChat& chat;

    DebugCommand(const TwitchChat& chat) : chat(const_cast<TwitchChat &>(chat)){}

    void execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cout << "Usage: /debug <test-type>" << std::endl;
            std::cout << "Available test types:" << std::endl;
            std::cout << "1. echo - Simulate an echoed message" << std::endl;
            std::cout << "2. priv - Simulate a basic PRIVMSG" << std::endl;
            std::cout << "2. priv - Simulate a raw message with tags" << std::endl;
            return;
        }

        std::string test = args[0];
        if (test == "echo") {
            // Simulate an echoed message
            parseAndPrintMessage(":username!username@username.tmi.twitch.tv PRIVMSG #channel :test message", isTyping, chat);
        }
        else if (test == "priv") {
            // Simulate a message without tags
            parseAndPrintMessage("PRIVMSG #channel :test message", isTyping, chat);
        }else if (test == "raw"){
            parseAndPrintMessage("@badge-info=;badges=broadcaster/1,streamer-awards-2024/1;color=#FF69B4;display-name=gavinbot32;"
                                 "emotes=;first-msg=0;flags=;id=2fc5544a-2fa5-4860-96f2-6ed68c306913;mod=0;returning-chatter=0;"
                                 "room-id=154649067;subscriber=0;tmi-sent-ts=1749175029323;turbo=0;user-id=154649067;"
                                 "user-type= :gavinbot32!gavinbot32@gavinbot32.tmi.twitch.tv PRIVMSG #gavinbot32 :kek",isTyping, chat);
        }
    }

    std::string getDescription() override {
        return "Debug command to test message parsing";
    }
};

class SetCommand : public Command {
    TwitchChat& chat;
public:

    SetCommand(const TwitchChat& chat) : chat(const_cast<TwitchChat &>(chat)){}

    void execute(const std::vector<std::string>& args) override {
        if(args.empty()){
            std::cerr << "Usage: /set <channel> | <channel_color>" << std::endl;
            return;
        }
        if(args[0] == "channel"){
            if(args.size() < 2){
                std::cerr << "Usage: /set channel <channel>" << std::endl;
                return;
            }
            std::cout << "Setting channel to " << args[1] << std::endl;
            ConfigManager& credentials = JsonSettings::getJsonFiles()["credentials"];
            credentials.set("channel", args[1]);
            credentials.saveConfig();
            return;
        }
        if(args[0] == "channel_color"){
            if(args.size() < 2){
                std::cerr << "Usage: /set channel_color <#hexcode> | <color>" << std::endl;
                return;
            }
            std::string color;
            std::unordered_map<std::string, std::string>& colorsMap = ColorSystem::colorMap;
            if(colorsMap.find(args[1]) != colorsMap.end()){
                color = colorsMap[args[1]];
            }else if(ColorSystem::isValidHexColor(args[1])){
                color = args[1];
            }
            else{
                std::cerr << "Invalid color: " << args[1] << std::endl;
                return;
            }
            if(color[0] != '#'){
                color = "#" + color;
            }
            std::cout << "Setting channel color to " << colorText(args[1], color) << std::endl;
            ConfigManager& user_settings = JsonSettings::jsonFiles["user-settings"];
            user_settings.set("channel_color", color);
            user_settings.saveConfig();
            chat.updateSettings();
        }else{
            std::cerr << "Usage: /set <channel> | <channel_color>" << std::endl;
        }
    }

    std::string getDescription() override{
        return "Sets a default config value, options: <channel>";
    }

};

class HighlightCommand : public Command {

    void execute(const std::vector<std::string> &args) override{
        if(args.empty()){
            std::cout << "\nHighlights: " << std::endl
            << " ______________________________________________\n|" << std::endl;
            for (auto& highlight : JsonSettings::highlights) {
                std::cout << "| " << colorText(highlight.first, highlight.second["color"],true) << std::endl;
            }
            std::cout << "|______________________________________________" << std::endl
            << "Usage: /highlights <help>|<add>|<remove>|<clear|<default>" << std::endl;;
            return;
        }
        if(args[0] == "help"){
            std::cout << "\nHighlights Menu: " << std::endl
            << " ______________________________________________" << std::endl;
            std::cout << "|"<<std::endl;
            std::cout << "| help - Displays this menu." << std::endl;
            std::cout << R"(| add - Adds a highlight. Usage: /highlights add <highlight> <"user" | "badge"> <color>)" << std::endl;
            std::cout << R"(| remove - Removes a highlight. Usage: /highlights remove <highlight>)" << std::endl;
            std::cout << R"(| clear - Clears all highlights.)" << std::endl;
            std::cout << R"(| default - Sets to default highlights.)" << std::endl;
            std::cout << "|______________________________________________" << std::endl;
            return;
        }
        if(args[0] == "add"){
            if(args.size() < 4){
                std::cerr << colorText(R"(Usage: /highlights add <highlight> <"user" | "badge"> <color>)", "#880000") << std::endl;
                return;
            }
            std::string highlight = args[1];
            std::string type = args[2];
            std::string color = args[3];
            if(JsonSettings::highlights.find(highlight) != JsonSettings::highlights.end()){
                std::cerr << colorText("Highlight already exists: " + highlight, "#880000") << std::endl;
                return;
            }
            if(type != "user" && type != "badge"){
                std::cerr << colorText("Invalid type: " + type, "#880000") << std::endl;
                std::cerr << colorText(R"(Usage: /highlights add <highlight> <"user" | "badge"> <color>)", "#880000") << std::endl;
                return;
            }
            if(!ColorSystem::isValidHexColor(color)){
                std::cerr << colorText("Invalid color: " + color, "#880000") << std::endl;
                std::cerr << colorText("Please enter a valid hex color code.", "#880000") << std::endl;
                std::cerr << colorText(R"(Usage: /highlights add <highlight> <"user" | "badge"> <color>)", "#880000") << std::endl;
                return;
            }

            if(color[0] != '#'){
                color = "#" + color;
            }

            std::cout << colorText("Adding highlight: " + highlight, color, true) << std::endl;

            JsonSettings::highlights.emplace(highlight, std::unordered_map<std::string, std::string>{{"type",type},{"color",color}});
            ConfigManager& userSettings = JsonSettings::jsonFiles["user-settings"];
            userSettings.set("highlights", JsonSettings::highlights);
            userSettings.saveConfig();
            return;

        }
        if(args[0] == "remove"){
            if(args.size() < 2){
                std::cerr << colorText(R"(Usage: /highlights remove <highlight>)", "#880000") << std::endl;
                return;
            }
            if(JsonSettings::highlights.find(args[1]) == JsonSettings::highlights.end()){
                std::cerr << colorText("Highlight does not exist: " + args[1], "#880000") << std::endl;
                return;
            }
            std::cout << colorText("Removing highlight: ", "#880000",false) << colorText(args[1], JsonSettings::highlights[args[1]]["color"],true) << std::endl;
            JsonSettings::highlights.erase(args[1]);
            ConfigManager& userSettings = JsonSettings::jsonFiles["user-settings"];
            userSettings.set("highlights", JsonSettings::highlights);
            userSettings.saveConfig();
            return;
        }
        if(args[0] == "clear"){
            JsonSettings::highlights.clear();
            ConfigManager& userSettings = JsonSettings::jsonFiles["user-settings"];
            userSettings.set("highlights", JsonSettings::highlights);
            userSettings.saveConfig();
            std::cout << colorText("Clearing all highlights.", "#880000") << std::endl;
        }
        if(args[0] == "default"){
            json default_highlights = {
                    {"partner", {
                            {"type", "badge"},
                            {"color", "#3b073b"}
                    }}
            };
            JsonSettings::highlights.clear();
            for(auto& highlight : default_highlights.items()){
                std::unordered_map<std::string, std::string> highlightMap = highlight.value();
                JsonSettings::highlights.emplace(highlight.key(), highlightMap);
            }
            ConfigManager& userSettings = JsonSettings::jsonFiles["user-settings"];
            userSettings.set("highlights", JsonSettings::highlights);
            userSettings.saveConfig();
            std::cout << colorText("Default highlights set.", "#880000") << std::endl;
        }
    }

    std::string getDescription() override{
        return "Displays the list of highlights, and allows you to add, remove or clear highlights.";
    }

};

void registerCommands(CommandRegistry& registry, const TwitchChat& chat){
    registry.registerCommand("quit", std::make_shared<QuitCommand>(chat));
    registry.registerCommand("clear", std::make_shared<ClearCommand>());
    registry.registerCommand("join", std::make_shared<JoinCommand>(chat));
    registry.registerCommand("raw", std::make_shared<RawModeCommand>());
    registry.registerCommand("badges", std::make_shared<BadgeListCommand>());
    registry.registerCommand("debug", std::make_shared<DebugCommand>(chat));
    registry.registerCommand("set", std::make_shared<SetCommand>(chat));
    registry.registerCommand("highlights", std::make_shared<HighlightCommand>());

    //Keep help command at bottom.
    registry.registerCommand("help", std::make_shared<HelpCommand>(registry));
    std::cout << "Type /help for a list of commands." << std::endl;
}

std::string formattedInputString(const std::string& input, TwitchChat& chat){

    return colorText(chat.getChannel(), chat.getChannelColor()) + "  " +
           chat.badgeStr +
           colorText(chat.getUsername()+ ": ", chat.getUserColor()) +
           input;
}

int main() {

    // ---Load config Json files---
    try{
        JsonSettings::initializeJsonFiles();
        std::cout << colorText("Loading config files...", "#005b5b") << std::endl;
    } catch(const std::exception& e){
        std::cerr << "Error loading config files: " << e.what() << std::endl
        << "Aborting..."<< std::endl;
        exit(1);
    }

    // ---Start Twitch Chat---
    try{
        std::cout << std::unitbuf;
        asio::io_context io;
        asio::executor_work_guard<asio::io_context::executor_type> work_guard =
                asio::make_work_guard(io);  // Keep io_context running

        // ---Create, initialize and login with the TwitchChat object.---
        TwitchChat chat(io);

        // ---Register commands---
        CommandRegistry registry;
        registerCommands(registry, chat);

        // ---Connect to chat---
        chat.connect();

        // ---Start main threads---
        std::thread io_thread([&io]() {
            io.run();
        });

        // --Start Input Thread--
        std::thread inputThread([&](){
           std::string userInput;
           while (true){
                userInput = getLineWithTypingDetection(chat.getChannel(), chat.getUsername());
                //std::cout << colorText("You: ", "#008700") << colorText(userInput, "#800000") << std::endl;


               if(registry.executeCommand(userInput)){
                   continue;
               }

               //If input is not empty or whitespace, send it to chat.
               if(!userInput.empty() && !std::all_of(userInput.begin(), userInput.end(), [](char c){return c == ' ';}) ){
                   chat.sendMessage(userInput);
               }
               messageBuffer.push(formattedInputString(userInput, chat));;
               flushBufferedMessages();

           }
        });

        // ---Join threads.---

        inputThread.join();
        work_guard.reset();
        io_thread.join();
    }catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}