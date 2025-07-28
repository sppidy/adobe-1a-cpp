#include "text_corrector.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>

TextCorrector::TextCorrector() {
    initialize_corrections();
}

std::string TextCorrector::correct_text(const std::string& input) {
    if (input.empty()) return input;
    
    std::string result = input;
    
    // Apply basic fixes first (fastest)
    result = apply_basic_fixes(result);
    
    // Apply regex fixes if needed
    if (aggressive_mode_) {
        result = apply_regex_fixes(result);
    }
    
    return result;
}

void TextCorrector::load_custom_corrections(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open corrections file: " << file_path << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Expected format: "wrong_word=correct_word"
        size_t delimiter = line.find('=');
        if (delimiter != std::string::npos) {
            std::string wrong = line.substr(0, delimiter);
            std::string correct = line.substr(delimiter + 1);
            basic_fixes_[wrong] = correct;
        }
    }
}

std::string TextCorrector::apply_basic_fixes(const std::string& text) {
    std::string result = text;
    
    // Apply all basic corrections
    for (const auto& fix : basic_fixes_) {
        size_t pos = 0;
        while ((pos = result.find(fix.first, pos)) != std::string::npos) {
            result.replace(pos, fix.first.length(), fix.second);
            pos += fix.second.length();
        }
    }
    
    // Clean up whitespace
    // Remove extra spaces
    result = std::regex_replace(result, std::regex("\\s+"), " ");
    
    // Trim leading/trailing whitespace
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    
    return result;
}

std::string TextCorrector::apply_regex_fixes(const std::string& text) {
    std::string result = text;
    
    // Apply all regex-based corrections
    for (const auto& fix : regex_fixes_) {
        result = std::regex_replace(result, fix.first, fix.second);
    }
    
    return result;
}

bool TextCorrector::is_valid_correction(const std::string& original, const std::string& corrected) {
    // Simple validation: don't make corrections that are too different
    if (corrected.empty()) return false;
    if (corrected.length() > original.length() * 2) return false;
    if (original.length() > corrected.length() * 2) return false;
    
    return true;
}

void TextCorrector::initialize_corrections() {
    // Basic OCR character confusions
    basic_fixes_ = {
        // Common OCR character substitutions
        {"rn", "m"}, {"vv", "w"}, {"ii", "ll"}, {"oo", "co"}, {"cl", "d"},
        
        // Word-level corrections (from Python implementation)
        {"rnatch", "match"}, {"vvork", "work"}, {"cornpany", "company"},
        {"rnoney", "money"}, {"rnanage", "manage"}, {"rnarket", "market"},
        {"tilie", "title"}, {"nieet", "meet"}, {"rnust", "must"},
        {"vvill", "will"}, {"vvith", "with"}, {"vvhen", "when"},
        {"vvhere", "where"}, {"vvhat", "what"}, {"vvhy", "why"},
        {"rnight", "might"}, {"rnore", "more"}, {"rnark", "mark"},
        
        // Technical terms
        {"Aadile", "Agile"}, {"aadile", "agile"},
        {"Testina", "Testing"}, {"testina", "testing"},
        {"Entrv", "Entry"}, {"entrv", "entry"},
        {"lntroduction", "Introduction"}, {"lntroduction", "introduction"},
        {"Reguirements", "Requirements"}, {"reguirements", "requirements"},
        {"Develooment", "Development"}, {"develooment", "development"},
        {"Manaaement", "Management"}, {"manaaement", "management"},
        {"Orqanization", "Organization"}, {"orqanization", "organization"},
        {"Backaround", "Background"}, {"backaround", "background"},
        {"Technoloaical", "Technological"}, {"technoloaical", "technological"},
        
        // Common spelling mistakes
        {"recieve", "receive"}, {"seperate", "separate"}, {"occured", "occurred"},
        {"definately", "definitely"}, {"managment", "management"},
        {"enviroment", "environment"}, {"accomodate", "accommodate"},
        {"begining", "beginning"}, {"beleive", "believe"}, {"occassion", "occasion"},
        {"profesional", "professional"}, {"recomend", "recommend"},
        {"neccessary", "necessary"}, {"accross", "across"}, {"untill", "until"},
        {"thier", "their"}, {"freind", "friend"}, {"sence", "sense"},
        
        // Document-specific terms
        {"qgovernance", "governance"}, {"governance", "governance"},
        {"decision-makina", "decision-making"}, {"fundina", "funding"},
        {"reallv", "really"}, {"librarv", "library"}, {"fullv", "fully"},
        {"aovernment", "government"}, {"tc", "to"}, {"Strateqy", "Strategy"},
        
        // Punctuation fixes
        {"timeline-", "Timeline:"}, {"summary-", "Summary:"},
        {"background-", "Background:"}, {"guidance-", "Guidance:"}
    };
    
    // Regex-based fixes for more complex patterns
    regex_fixes_ = {
        // Fix number patterns: "1 2 3" -> "1.2.3"
        {std::regex(R"((\d)\s+(\d))"), "$1.$2"},
        
        // Fix spaced periods: "1 . 2" -> "1.2"  
        {std::regex(R"((\d)\s*\.\s+(\d))"), "$1.$2"},
        
        // Fix ordinal numbers: "lst" -> "1st", "2ncl" -> "2nd"
        {std::regex(R"(\b(\d+)lst\b)"), "$1st"},
        {std::regex(R"(\b(\d+)ncl\b)"), "$1nd"},
        {std::regex(R"(\b(\d+)rcl\b)"), "$1rd"},
        {std::regex(R"(\b(\d+)th\b)"), "$1th"}
    };
}
