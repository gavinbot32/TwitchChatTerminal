// TwitchChat.h
#pragma once

#include <asio.hpp>
#include <string>

class TwitchChat {
public:
    TwitchChat(asio::io_context& io_context);

    void connect();
    void setLoginInfo(const std::string& oauth, const std::string& user, const std::string& channel);
    void sendMessage(const std::string& msg);
    bool joinChannel(const std::string &channel);
    void disconnect();
    void wait_for_operations();
    std::string getChannel();
    std::string getUsername();
    std::string getOauth();
    std::string getUserColor();
    std::string badgeStr;
    void setUserColor(const std::string& color);
    std::string getChannelColor();
    void updateSettings();

private:
    asio::io_context& io;
    asio::ip::tcp::socket socket;
    asio::streambuf buffer;
    std::string username;
    std::string oauth;
    std::string channel;
    std::string user_color;
    std::atomic<bool> is_connecting{false};
    std::atomic<bool> is_disconnecting{false};

    std::string channelColor;

    static constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024;

    void login();
    void read_messages();
    void sendCapabilityRequest();
    bool verifyTwitchToken(const std::string& oauth_token);
    void loadAndLoginProcess();

};
