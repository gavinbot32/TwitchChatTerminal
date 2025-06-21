#include <string>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include "ColorSystem.h"
#include <unordered_map>

enum class ColorSupport{
    None,
    ANSI16,
    ANSI256,
    TrueColor,
};


bool ColorSystem::isValidHexColor(const std::string& hex) {
    if (hex.size() < 6 || hex.size() > 7) {
        return false;
    }
    std::string color = hex;
    if (color[0] == '#') {
        color = color.substr(1);
    }

    for (char c: color) {
        if (!std::isxdigit(c)) {
            return false;
        }
    }
    return true;
}


std::unordered_map<std::string, std::string> ColorSystem::colorMap = {{"red","#ff0000"}, {"green", "#00ff00"}, {"blue", "#0000ff"}, {"yellow", "#ffff00"},
                                                                      {"magenta","#ff00ff"}, {"cyan", "#00ffff"}, {"white", "#ffffff"}, {"black", "#000000"},
                                                                      {"pink","#eb67eb"}, {"orange", "#ff8000"}, {"purple", "#4c00ff"}, {"brown", "#804e2d"},
                                                                      {"light-red","#ff6666"}, {"light-green", "#66ff66"}, {"light-blue", "#6694ff"},
                                                                      {"light-yellow","#ffff66"}, {"light-magenta", "#ff66ff"}, {"light-cyan", "#66ffff"}};

//--- Detect Color Support ---
ColorSupport detectColorSupport() {
    const char* colorterm = std::getenv("COLORTERM");
    if(colorterm){
        std::string ct(colorterm);
        if(ct == "truecolor" || ct == "24bit"){
            return ColorSupport::TrueColor;
        }
    }

    const char* term = std::getenv("TERM");
    if(term){
        std::string t(term);
        if(t.find("256color") != std::string::npos){
            return ColorSupport::ANSI256;
        }
        if(t.find("color") != std::string::npos){
            return ColorSupport::ANSI16;
        }
    }

    return ColorSupport::None;
}

// --Hex to RGB ---
bool hexToRGB(const std::string& hex, int& r, int& g, int& b) {
    if(hex.size() != 7 || hex[0] != '#'){
        return false;
    }

    try {
        r = std::stoi(hex.substr(1,2), nullptr, 16);
        g = std::stoi(hex.substr(3,2), nullptr, 16);
        b = std::stoi(hex.substr(5,2), nullptr, 16);
        return true;
    } catch (...){
        return false;
    }
}

// --- RGB to ANSI 256 (approximation) ---
int rgbToANSI256(int r, int g, int b) {
    //Map 0-255 to 0-5
    auto to_ansi = [](int val) {return static_cast<int>(6 * val / 256);};
    int ansi = 16 + 36 * to_ansi(r) + 6 * to_ansi(g) + to_ansi(b);
    return ansi;
}

// --- Get ANSI Escape String ---
std::string getColorEscape(const std::string& hex, ColorSupport level, bool background ) {
    int r, g, b;
    if(!hexToRGB(hex, r, g, b)){
        return "";
    }

    std::ostringstream out;
    if(!background) {
        switch (level) {
            case ColorSupport::TrueColor:
                out << "\033[38;2;" << r << ";" << g << ";" << b << "m";
                break;
            case ColorSupport::ANSI256:
                out << "\033[38;5;" << rgbToANSI256(r, g, b) << "m";
                break;
            case ColorSupport::ANSI16:
                out << "\033[34m";
                break;
            default:
                return "";
        }
    } else{
        switch (level) {
            case ColorSupport::TrueColor:
                out << "\033[48;2;" << r << ";" << g << ";" << b << "m";
                break;
            case ColorSupport::ANSI256:
                out << "\033[48;5;" << rgbToANSI256(r, g, b) << "m";
                break;
            case ColorSupport::ANSI16:
                out << "\033[44m";
                break;
            default:
                return "";
        }
    }

    return out.str();
}

// ---Color a String ---
std::string colorText(const std::string& text, const std::string& hex, bool background){
    ColorSupport level = detectColorSupport();
    std::string color = getColorEscape(hex, level, background);
    if(color.empty()) return text;
    return color + text + "\033[0m";
}