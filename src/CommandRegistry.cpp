//
// Created by micha on 6/4/2025.
//

#include "CommandRegistry.h"
#include "TwitchChat.h"

#include <utility>
#include <sstream>
#include <iostream>
#include <iomanip>

void CommandRegistry::registerCommand(const std::string &name, std::shared_ptr<Command> command) {
    commands[name] = std::move(command);
}

bool CommandRegistry::executeCommand(const std::string &inputLine) {
    if(inputLine.empty() || inputLine[0] != '/') return false;

    std::istringstream iss(inputLine.substr(1));
    std::string commandName;
    iss >> commandName;

    std::vector<std::string> args;
    std::string arg;
    while(iss >> std::quoted(arg)){
        args.push_back(arg);
    }

    auto it = commands.find(commandName);
    if(it != commands.end()){
        it->second->execute(args);
    }else{
        std::cout << "[Error] Unknown command: " << commandName << std::endl;
    }
    return true;
}

const std::unordered_map<std::string, std::shared_ptr<Command>> &CommandRegistry::getCommands() const {
    return commands;
}


