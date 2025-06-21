#include <string>
#include <vector>

#ifndef TWITCHCONSOLEVIEWER_COMMAND_H
#define TWITCHCONSOLEVIEWER_COMMAND_H


class Command {
public:
    virtual ~Command() = default;
    virtual void execute(const std::vector<std::string>& args) = 0;
    virtual std::string getDescription() = 0;

};



#endif //TWITCHCONSOLEVIEWER_COMMAND_H
