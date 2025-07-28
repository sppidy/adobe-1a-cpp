#include "utils.hpp"
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <vector>

namespace utils {

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string get_filename_without_extension(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

void ensure_directory_exists(const std::string& path) {
    std::filesystem::create_directories(path);
}

std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }
    
    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::string(start, end + 1);
}

std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_valid_heading_text(const std::string& text) {
    if (text.empty() || text.length() < 2) {
        return false;
    }
    
    // Count alphabetic characters
    int alpha_count = 0;
    for (char c : text) {
        if (std::isalpha(c)) {
            alpha_count++;
        }
    }
    
    // Must have at least 3 alphabetic characters
    if (alpha_count < 3) {
        return false;
    }
    
    // Check for too many special characters
    int special_count = 0;
    for (char c : text) {
        if (!std::isalnum(c) && !std::isspace(c) && c != '.' && c != '-' && c != ':' && c != ',') {
            special_count++;
        }
    }
    
    // More than 30% special characters is suspicious
    if (special_count > static_cast<int>(text.length() * 0.3)) {
        return false;
    }
    
    // Filter out pure numbers
    bool all_numeric = true;
    for (char c : text) {
        if (!std::isdigit(c) && c != '.' && !std::isspace(c)) {
            all_numeric = false;
            break;
        }
    }
    
    if (all_numeric) {
        return false;
    }
    
    return true;
}

bool contains_mostly_letters(const std::string& text, double threshold) {
    if (text.empty()) return false;
    
    int letter_count = 0;
    for (char c : text) {
        if (std::isalpha(c)) {
            letter_count++;
        }
    }
    
    return static_cast<double>(letter_count) / text.length() >= threshold;
}

} // namespace utils
