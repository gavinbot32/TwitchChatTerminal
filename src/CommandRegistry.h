#include <unordered_map>
#include <string>
#include <memory>
#include "Command.h"

#ifndef TWITCHCONSOLEVIEWER_COMMANDREGISTRY_H
#define TWITCHCONSOLEVIEWER_COMMANDREGISTRY_H


class CommandRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<Command>> commands;
public:
    void registerCommand(const std::string& name, std::shared_ptr<Command> command);

    bool executeCommand(const std::string& inputLine);

    const std::unordered_map<std::string, std::shared_ptr<Command>> &getCommands() const;

};


#endif //TWITCHCONSOLEVIEWER_COMMANDREGISTRY_H
