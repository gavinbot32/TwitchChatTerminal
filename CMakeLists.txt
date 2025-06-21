cmake_minimum_required(VERSION 3.31)
project(TwitchConsoleViewer)

include(FetchContent)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)


find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 20)

# Define ASIO_STANDALONE before including ASIO
add_definitions(-DASIO_STANDALONE)

# Update include directory to point to the include folder
include_directories(external/asio/asio/include)

add_executable(TwitchConsoleViewer src/main.cpp
        src/Command.h
        src/CommandRegistry.cpp
        src/CommandRegistry.h
        src/TwitchChat.cpp
        src/TwitchChat.h
        src/MessageParser.cpp
        src/MessageParser.h
        src/ConsoleInput.cpp
        src/ConsoleInput.h
        src/ColorSystem.h
        src/ConfigManager.h
        src/JsonSettings.h
        src/JsonSettings.cpp
        src/ColorSystem.cpp
)

# Link against threads library
find_package(Threads REQUIRED)
target_link_libraries(TwitchConsoleViewer PRIVATE
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
)
