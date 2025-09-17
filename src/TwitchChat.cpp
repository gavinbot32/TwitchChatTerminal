#include <queue>
#include <iostream>
#include "MessageParser.h"
#include <asio.hpp>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <future>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream_base.hpp>
#include <asio/ssl/stream.hpp>
#include "TwitchChat.h"
#include "ColorSystem.h"
#include "ConfigManager.h"
#include "JsonSettings.h"

extern std::atomic<bool> isTyping;
extern std::mutex messageMutex;
extern std::queue<std::string> messageBuffer;

extern std::unordered_map<std::string, std::string> badges;

using asio::ip::tcp;

TwitchChat::TwitchChat(asio::io_context& io_context)
        : io(io_context), socket(io_context) {
    setUserColor("#008787");
    loadAndLoginProcess();
    updateSettings();
}

void TwitchChat::setLoginInfo(const std::string& oauth, const std::string& user, const std::string& channel){
    this->oauth = oauth;
    username = user;
    this->channel = channel;
}

bool TwitchChat::joinChannel(const std::string &channel) {
    if (channel.empty()) {
        return false;
    }

    std::string formattedChannel = (channel[0] != '#') ? "#" + channel : channel;

    // Basic channel name validation
    if (formattedChannel.length() < 2) {  // '#' plus at least one character
        return false;
    }

    disconnect();
    wait_for_operations();

    // Use a mutex if thread safety is needed
    this->channel = formattedChannel;

    // Wait for disconnect to complete or timeout
    // This could be implemented with a future/promise pattern

    connect();
    wait_for_operations();

    return true;
}




void TwitchChat::login(){
    auto auth_msg = "PASS "+ oauth + "\r\n"
                                     "NICK " + username + "\r\n"
                                                          "JOIN "+channel+"\r\n";
    asio::async_write(socket, asio::buffer(auth_msg),
                      [this](const asio::error_code& ec, std::size_t){
                          if(!ec){
                              std::cout << colorText("Connected to ", "#008700") << colorText(channel, channelColor) << colorText("!", "#008700") << std::endl;
                              read_messages();
                          }else{
                              std::cerr << "Login error: " << ec.message() << std::endl;
                          }
                      });
}

void TwitchChat::read_messages() {
    if(!socket.is_open()) return;

    if(buffer.size() > MAX_BUFFER_SIZE) {
        buffer.consume(buffer.size());
    }

    asio::async_read_until(socket, buffer, "\r\n",
                           [this](const asio::error_code& ec, std::size_t length) {
                               if(!ec) {
                                   std::istream stream(&buffer);
                                   std::string line;
                                   std::getline(stream, line);

                                   if(line.substr(0,4) == "PING") {
                                       //std::cout << colorText("Server: PING", "#55005f") << std::endl;
                                       asio::async_write(socket,
                                                         asio::buffer("PONG :tmi.twitch.tv\r\n"),
                                                         [](const asio::error_code& ec, std::size_t) {
                                                             if(ec) {
                                                                 std::cerr << "PONG error: " << ec.message() << std::endl;
                                                             }else{
                                                                 //std::cout << colorText("Server: PONG", "#55005f") << std::endl;
                                                             }
                                                         });
                                       sendMessage(" ");
                                   }

                                   //If server message
                                   if (line.find(":tmi.twitch.tv") != std::string::npos) {
                                       if(line.find(" USERSTATE ") != std::string::npos){
                                           std::unordered_map<std::string, std::string> usTags = parseTags(line);
                                          /* for(auto& tag : usTags){
                                               std::cout << colorText("Userstate: ", "#008787") << tag.first << " : " << tag.second << std::endl;
                                           }*/
                                           if(usTags.find("display-name") != usTags.end()){
                                               if(usTags["display-name"] == username){
                                                   //Set user color
                                                   if(usTags.find("color") != usTags.end()){
                                                       setUserColor(usTags["color"]);
                                                   }
                                                   //set badges
                                                   badgeStr = "";
                                                   if (usTags.find("badges") != usTags.end()) {
                                                       std::vector<std::string> userBadges = parseBadges(usTags["badges"]);
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

                                               }
                                           }
                                       }
                                   }





                                   //std::cout << colorText("Read Pre-Parse: ", "#101010",true) + line << std::endl;
                                   parseAndPrintMessage(line, isTyping, *this);
                                   read_messages();
                               }else if (ec != asio::error::operation_aborted){
                                   std::cerr << "Read error: " << ec.message() << std::endl;

                                   std::this_thread::sleep_for(std::chrono::seconds(1));
                                   connect();
                               }
                           });
}

void TwitchChat::sendMessage(const std::string& msg) {
    if (!socket.is_open()) return;

    std::string fullMsg = "PRIVMSG " + channel + " :" + msg + "\r\n";
    asio::async_write(socket, asio::buffer(fullMsg),
                      [](const asio::error_code& ec, std::size_t) {
                          if (ec) {
                              std::cerr << "Send error: " << ec.message() << std::endl;
                          }else{
                              //std::cout << colorText("Sending ", "#101010",true) << std::endl;
                          }
                      });
}

void TwitchChat::connect() {
    if (socket.is_open()) return;

    is_connecting = true;
    auto connect_promise = std::make_shared<std::promise<void>>();
    auto connect_future = connect_promise->get_future();
    
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve("irc.chat.twitch.tv", "6667");

    asio::async_connect(socket, endpoints,
        [this, promise = std::move(connect_promise)](const asio::error_code& ec, const tcp::endpoint&) {
            if (!ec) {
                sendCapabilityRequest();
            } else {
                std::cerr << "Connect error: " << ec.message() << std::endl;
            }
            is_connecting = false;
            promise->set_value();
        });

    std::cout << colorText("Connecting to ", "#008700") << colorText(channel, channelColor) << colorText("...", "#008700")<< std::endl;
    // Wait for connect to complete with timeout
    if (connect_future.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
        is_connecting = false;
    }
}

void TwitchChat::disconnect() {
    if (!socket.is_open()) return;


    is_disconnecting = true;
    auto disconnect_promise = std::make_shared<std::promise<void>>();
    auto disconnect_future = disconnect_promise->get_future();
    
    asio::post(io, [this, promise = std::move(disconnect_promise)](){
        asio::error_code ec;
        socket.cancel(ec);  // Cancel any pending operations
        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);
        is_disconnecting = false;
        promise->set_value();
    });

    std::cout << colorText("Disconnecting from ","#5f0000") << colorText(channel, channelColor) << colorText("...", "#5f0000") << std::endl;
    // Wait for disconnect to complete with timeout
    if (disconnect_future.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
        is_disconnecting = false;
    }
}

void TwitchChat::wait_for_operations() {
    while(is_connecting || is_disconnecting){
        std::cout << "Waiting for operations to complete... " << channel << std::endl;
        std::cout << "Connecting: "<< is_connecting << " Disconnecting: " << is_disconnecting << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::string TwitchChat::getChannel() {
    return channel;
}

std::string TwitchChat::getUsername() {
    return username;
}

// ***Used only to verify the status of the oauth token.***
std::string TwitchChat::getOauth() {
    return oauth;
}

void TwitchChat::sendCapabilityRequest() {
    std::string caps = "CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership\r\n";
    asio::async_write(socket, asio::buffer(caps),
                      [this](const asio::error_code& ec, std::size_t) {
                          if (!ec) {
                              login();  // continue to PASS, NICK, JOIN
                          } else {
                              std::cerr << "Capability request error: " << ec.message() << std::endl;
                          }
                      });
}

bool TwitchChat::verifyTwitchToken(const std::string &oauth_token) {
    try {
        asio::io_context io_context;
        asio::ssl::context ssl_context(asio::ssl::context::sslv23_client);

        // Create HTTPS client
        asio::ssl::stream<asio::ip::tcp::socket> socket(io_context, ssl_context);

        // Resolve and connect to api.twitch.tv
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("id.twitch.tv", "443");
        asio::connect(socket.lowest_layer(), endpoints);

        socket.handshake(asio::ssl::stream_base::client);

        // Format token (remove "oauth:" prefix if present)
        std::string token = oauth_token;
        if (token.substr(0, 6) == "oauth:") {
            token = token.substr(6);
        }

        // Create HTTP request
        std::string request =
                "GET /oauth2/validate HTTP/1.1\r\n"
                "Host: id.twitch.tv\r\n"
                "Authorization: OAuth " + token + "\r\n"
                                                  "Connection: close\r\n\r\n";

        asio::write(socket, asio::buffer(request));

        // Read response
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");

        std::string http_version;
        unsigned int status_code;
        std::string status_message;

        std::istream response_stream(&response);
        response_stream >> http_version >> status_code >> std::ws;
        std::getline(response_stream, status_message);

        // Status code 200 means token is valid
        return status_code == 200;

    } catch (std::exception& e) {
        std::cerr << "Token verification failed: " << e.what() << std::endl;
        return false;
    }}

void TwitchChat::loadAndLoginProcess() {

    try {
        ConfigManager& credentials = JsonSettings::getJsonFiles()["credentials"];

        std::string oauth = credentials.get("oauth", std::string(""));
        std::string user = credentials.get("user", std::string(""));
        std::string channel = credentials.get("channel", std::string(""));

        if (user.empty()) {
            std::cout << "Enter your username: ";
            std::getline(std::cin, user);
        }

        if (oauth.empty()) {
            std::cout << "Enter your oauth token: ";
            std::getline(std::cin, oauth);
            while (!verifyTwitchToken(oauth)) {
                std::cout << "Invalid token. Please enter a valid token: ";
                std::getline(std::cin, oauth);
            }

        } else if (!verifyTwitchToken(oauth)) {
            std::cout << "Token invalid. Please enter a valid token: ";
            std::getline(std::cin, oauth);
            while (!verifyTwitchToken(oauth)) {
                std::cout << "Token invalid. Please enter a valid token: ";
                std::getline(std::cin, oauth);
            }
        }
        if (oauth.substr(0, 6) != "oauth:") {
            oauth = "oauth:" + oauth;
        }

        if (channel.empty()) {
            std::cout << "Enter a channel to join: ";
            std::getline(std::cin, channel);
            if (channel[0] != '#') {
                channel = "#" + channel;
            }
        }

        credentials.set("oauth", oauth);
        credentials.set("user", user);
        credentials.set("channel", channel);
        credentials.saveConfig();

        setLoginInfo(oauth, user, channel);

        /*chat.setLoginInfo(
                "oauth:REDACTED",
                "gavinbot32",
                "#gavinbot32"
        );*/
    } catch(const std::exception& e) {
        std::cerr << "Error loading credentials: " << e.what() << std::endl;
        return;
    }
}

std::string TwitchChat::getUserColor() {
    return user_color;
}

void TwitchChat::setUserColor(const std::string &color) {
    TwitchChat::user_color = color;
}

void TwitchChat::updateSettings() {
    ConfigManager& user_settings = JsonSettings::jsonFiles["user-settings"];
    TwitchChat::channelColor = user_settings.get("channel_color", std::string("#800000"));;
}

std::string TwitchChat::getChannelColor() {
    return TwitchChat::channelColor;
}



