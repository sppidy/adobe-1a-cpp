#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <memory>

class TextCorrector {
public:
    TextCorrector();
    ~TextCorrector() = default;
    
    // Main correction function
    std::string correct_text(const std::string& input);
    
    // Configuration
    void set_aggressive_mode(bool enabled) { aggressive_mode_ = enabled; }
    void load_custom_corrections(const std::string& file_path);
    
private:
    // Basic OCR error corrections
    std::string apply_basic_fixes(const std::string& text);
    
    // Pattern-based corrections
    std::string apply_regex_fixes(const std::string& text);
    
    // Validation helpers
    bool is_valid_correction(const std::string& original, const std::string& corrected);
    
    // Correction dictionaries
    std::unordered_map<std::string, std::string> basic_fixes_;
    std::vector<std::pair<std::regex, std::string>> regex_fixes_;
    
    // Configuration
    bool aggressive_mode_ = false;
    
    // Pre-compiled patterns for performance
    void initialize_corrections();
};
