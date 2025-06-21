#pragma once
#include <string>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <unordered_map>

enum class ColorSupport;


class ColorSystem {
public:

    static std::unordered_map<std::string, std::string> colorMap;

    static bool isValidHexColor(const std::string& hex);
};


//--- Detect Color Support ---
ColorSupport detectColorSupport();

// --Hex to RGB ---
bool hexToRGB(const std::string& hex, int& r, int& g, int& b);

// --- RGB to ANSI 256 (approximation) ---
int rgbToANSI256(int r, int g, int b);

// --- Get ANSI Escape String ---
std::string getColorEscape(const std::string& hex, ColorSupport level = detectColorSupport(), bool background = false);

// ---Color a String ---
std::string colorText(const std::string& text, const std::string& hex, bool background = false);