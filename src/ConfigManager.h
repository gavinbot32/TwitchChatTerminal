#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

#ifndef TWITCHCONSOLEVIEWER_CONFIGMANAGER_H
#define TWITCHCONSOLEVIEWER_CONFIGMANAGER_H

using json = nlohmann::json;
namespace fs = std::filesystem;

class ConfigManager {
private:
    fs::path config_path;
    std::string fileName;
    json config;

    void ensureConfigDirectory() {
        fs::path config_dir = config_path.parent_path();
        if (!fs::exists(config_dir)) {
            fs::create_directories(config_dir);
        }
    }

public:
    ConfigManager(const std::string& filename) : fileName(filename) {
        // Get path to the current executable
#ifdef _WIN32
        char path[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, path, MAX_PATH);
    fs::path exe_path(path);
#else
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        fs::path exe_path = std::string(result, (count > 0) ? count : 0);
#endif

        // Always use the executable's directory
        fs::path base_dir = exe_path.parent_path();
        config_path = base_dir / "config" / filename;

        loadConfig();
    }

    // Add copy constructor
    ConfigManager(const ConfigManager& other)
            : config_path(other.config_path)
            , fileName(other.fileName)
            , config(other.config) {
    }

    // Add move constructor
    ConfigManager(ConfigManager&& other) noexcept
            : config_path(std::move(other.config_path))
            , fileName(std::move(other.fileName))
            , config(std::move(other.config)) {
    }

    // Add copy assignment operator
    ConfigManager& operator=(const ConfigManager& other) {
        if (this != &other) {
            config_path = other.config_path;
            fileName = other.fileName;
            config = other.config;
        }
        return *this;
    }

    // Add move assignment operator
    ConfigManager& operator=(ConfigManager&& other) noexcept {
        if (this != &other) {
            config_path = std::move(other.config_path);
            fileName = std::move(other.fileName);
            config = std::move(other.config);
        }
        return *this;
    }

    std::string getConfigDir() const{
        std::string ret;
        fs::path retPath = fs::path(config_path);
        ret = retPath.remove_filename().generic_string();
        return ret;
    }


    void loadConfig() {
        try {
            if (fs::exists(config_path)) {
                std::ifstream file(config_path);
                file >> config;
            } else {
                config = json::object();  // Initialize empty config
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading config. " << std::endl
            << "Error Handling: Initializing a new empty file " << fileName << std::endl;

            config = json::object();  // Initialize empty config on error
        }
    }

    void saveConfig() {
        try {
            ensureConfigDirectory();
            std::ofstream file(config_path);
            file << std::setw(4) << config << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error saving config: " << e.what() << std::endl;
        }
    }

    // Generic get with default value
    template<typename T>
    T get(const std::string& key, const T& default_value) const {
        try {
            return config.value(key, default_value);
        } catch (...) {
            return default_value;
        }
    }

    // Generic set
    template<typename T>
    void set(const std::string& key, const T& value) {
        config[key] = value;
    }

    // Check if key exists
    bool hasKey(const std::string& key) const {
        return config.contains(key);
    }

    // Remove a key
    void remove(const std::string& key) {
        config.erase(key);
    }

    ConfigManager() {

    }
};


#endif //TWITCHCONSOLEVIEWER_CONFIGMANAGER_H
