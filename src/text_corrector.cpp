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
        {"0", "O"}, {"1", "l"}, {"5", "S"}, {"8", "B"}, {"6", "G"},
        {"l", "I"}, {"o", "0"}, {"S", "5"}, {"B", "8"}, {"G", "6"},
        
        // More OCR character confusions
        {"rri", "m"}, {"nn", "n"}, {"ur", "n"}, {"ni", "m"}, {"iu", "n"},
        {"fi", "h"}, {"li", "h"}, {"ti", "h"}, {"ri", "n"}, {"rr", "n"},
        {"cc", "c"}, {"ee", "e"}, {"tt", "t"}, {"pp", "p"}, {"bb", "b"},
        {"dd", "d"}, {"gg", "g"}, {"ff", "f"}, {"ss", "s"}, {"zz", "z"},
        
        // Letter-number confusions
        {"0ne", "one"}, {"1ike", "like"}, {"5ame", "same"}, {"8est", "best"},
        {"6ood", "good"}, {"1eft", "left"}, {"r1ght", "right"}, {"w0rd", "word"},
        {"numb3r", "number"}, {"t1me", "time"}, {"p1ace", "place"}, {"0ther", "other"},
        {"1arge", "large"}, {"5mall", "small"}, {"w0rk", "work"}, {"st0p", "stop"},
        
        // Common OCR word confusions
        {"tlie", "the"}, {"tlle", "the"}, {"t11e", "the"}, {"t1le", "the"},
        {"anci", "and"}, {"anct", "and"}, {"ancl", "and"}, {"arid", "and"},
        {"witli", "with"}, {"witll", "with"}, {"w1th", "with"}, {"wi1h", "with"},
        {"tliat", "that"}, {"t11at", "that"}, {"tl1at", "that"}, {"tllet", "that"},
        {"wlien", "when"}, {"w11en", "when"}, {"wl1en", "when"}, {"wheri", "when"},
        {"wliere", "where"}, {"w11ere", "where"}, {"wl1ere", "where"}, {"wllere", "where"},
        {"wliat", "what"}, {"w11at", "what"}, {"wl1at", "what"}, {"wllat", "what"},
        {"wliy", "why"}, {"w11y", "why"}, {"wl1y", "why"}, {"whv", "why"},
        {"liow", "how"}, {"l1ow", "how"}, {"ll0w", "how"}, {"h0w", "how"},
        {"wlio", "who"}, {"w11o", "who"}, {"wl1o", "who"}, {"wh0", "who"},
        
        // Punctuation OCR errors
        {".", ","}, {",", "."}, {";", ":"}, {":", ";"}, 
        {"'", "'"}, {"'", "'"}, {"\"", "\""}, {"\"", "\""},
        {"—", "-"}, {"–", "-"}, {"…", "..."}, {"'", "'"},
        
        // Capital letter OCR errors
        {"Tlie", "The"}, {"Anci", "And"}, {"Witli", "With"}, {"Tliat", "That"},
        {"Wlien", "When"}, {"Wliere", "Where"}, {"Wliat", "What"}, {"Wliy", "Why"},
        {"Liow", "How"}, {"Wlio", "Who"}, {"Tliis", "This"}, {"Tliey", "They"},
        {"Tliese", "These"}, {"Tliose", "Those"}, {"Tlirough", "Through"},
        {"Tliree", "Three"}, {"Tliirty", "Thirty"}, {"Tliink", "Think"},
        
        // Word-level corrections (from Python implementation)
        {"rnatch", "match"}, {"vvork", "work"}, {"cornpany", "company"},
        {"rnoney", "money"}, {"rnanage", "manage"}, {"rnarket", "market"},
        {"tilie", "title"}, {"nieet", "meet"}, {"rnust", "must"},
        {"vvill", "will"}, {"vvith", "with"}, {"vvhen", "when"},
        {"vvhere", "where"}, {"vvhat", "what"}, {"vvhy", "why"},
        {"rnight", "might"}, {"rnore", "more"}, {"rnark", "mark"},
        
        // More comprehensive word fixes
        {"rnake", "make"}, {"rnany", "many"}, {"rnain", "main"}, {"rnale", "male"},
        {"rnail", "mail"}, {"rnap", "map"}, {"rnass", "mass"}, {"rnaster", "master"},
        {"rnatter", "matter"}, {"rnax", "max"}, {"rnay", "may"}, {"rnean", "mean"},
        {"rneasure", "measure"}, {"rneet", "meet"}, {"rnember", "member"},
        {"rnention", "mention"}, {"rnethod", "method"}, {"rniddle", "middle"},
        {"rnile", "mile"}, {"rnillion", "million"}, {"rnind", "mind"},
        {"rnine", "mine"}, {"rninus", "minus"}, {"rniss", "miss"},
        {"rnix", "mix"}, {"rnodel", "model"}, {"rnoder", "modern"},
        {"rnorning", "morning"}, {"rnost", "most"}, {"rnother", "mother"},
        {"rnotion", "motion"}, {"rnount", "mount"}, {"rnouse", "mouse"},
        {"rnove", "move"}, {"rnuch", "much"}, {"rnusic", "music"},
        
        // V-W confusions
        {"vvant", "want"}, {"vvar", "war"}, {"vvarm", "warm"}, {"vvash", "wash"},
        {"vvaste", "waste"}, {"vvatch", "watch"}, {"vvater", "water"},
        {"vvave", "wave"}, {"vvay", "way"}, {"vve", "we"}, {"vveak", "weak"},
        {"vvear", "wear"}, {"vveather", "weather"}, {"vveb", "web"},
        {"vveek", "week"}, {"vveight", "weight"}, {"vvelcome", "welcome"},
        {"vvell", "well"}, {"vvest", "west"}, {"vvet", "wet"},
        {"vvhite", "white"}, {"vvhole", "whole"}, {"vvide", "wide"},
        {"vvin", "win"}, {"vvind", "wind"}, {"vvindow", "window"},
        {"vvinter", "winter"}, {"vvise", "wise"}, {"vvith", "with"},
        {"vvoman", "woman"}, {"vvomen", "women"}, {"vvon", "won"},
        {"vvood", "wood"}, {"vvord", "word"}, {"vvork", "work"},
        {"vvorld", "world"}, {"vvorry", "worry"}, {"vvorth", "worth"},
        {"vvould", "would"}, {"vvrite", "write"}, {"vvrong", "wrong"},
        
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
        
        // More technical OCR errors
        {"Prograrnming", "Programming"}, {"prograrnming", "programming"},
        {"Softvvare", "Software"}, {"softvvare", "software"},
        {"Cornputer", "Computer"}, {"cornputer", "computer"},
        {"Systern", "System"}, {"systern", "system"},
        {"Netvvork", "Network"}, {"netvvork", "network"},
        {"Databa5e", "Database"}, {"databa5e", "database"},
        {"Algorlthm", "Algorithm"}, {"algorlthm", "algorithm"},
        {"Functlon", "Function"}, {"functlon", "function"},
        {"Varlable", "Variable"}, {"varlable", "variable"},
        {"Strlng", "String"}, {"strlng", "string"},
        {"Objecť", "Object"}, {"objecť", "object"},
        {"Cla55", "Class"}, {"cla55", "class"},
        {"Methocl", "Method"}, {"methocl", "method"},
        {"lnterface", "Interface"}, {"lnterface", "interface"},
        {"Modulé", "Module"}, {"modulé", "module"},
        
        // Common spelling mistakes
        {"recieve", "receive"}, {"seperate", "separate"}, {"occured", "occurred"},
        {"definately", "definitely"}, {"managment", "management"},
        {"enviroment", "environment"}, {"accomodate", "accommodate"},
        {"begining", "beginning"}, {"beleive", "believe"}, {"occassion", "occasion"},
        {"profesional", "professional"}, {"recomend", "recommend"},
        {"neccessary", "necessary"}, {"accross", "across"}, {"untill", "until"},
        {"thier", "their"}, {"freind", "friend"}, {"sence", "sense"},
        {"calender", "calendar"}, {"buisness", "business"}, {"succesful", "successful"},
        {"tomorow", "tomorrow"}, {"febuary", "february"}, {"wenesday", "wednesday"},
        {"wieght", "weight"}, {"heigth", "height"}, {"lenght", "length"},
        {"knowlege", "knowledge"}, {"priviledge", "privilege"}, {"embarass", "embarrass"},
        
        // Document-specific terms
        {"qgovernance", "governance"}, {"governance", "governance"},
        {"decision-makina", "decision-making"}, {"fundina", "funding"},
        {"reallv", "really"}, {"librarv", "library"}, {"fullv", "fully"},
        {"aovernment", "government"}, {"tc", "to"}, {"Strateqy", "Strategy"},
        {"policv", "policy"}, {"analvsis", "analysis"}, {"researcli", "research"},
        {"studv", "study"}, {"reportina", "reporting"}, {"meetina", "meeting"},
        {"plannina", "planning"}, {"budaet", "budget"}, {"proiect", "project"},
        
        // Punctuation fixes
        {"timeline-", "Timeline:"}, {"summary-", "Summary:"},
        {"background-", "Background:"}, {"guidance-", "Guidance:"},
        {"overview-", "Overview:"}, {"conclusion-", "Conclusion:"},
        {"introduction-", "Introduction:"}, {"methodology-", "Methodology:"},
        {"results-", "Results:"}, {"discussion-", "Discussion:"}
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
        {std::regex(R"(\b(\d+)th\b)"), "$1th"},
        
        // Fix isolated single characters that should be words
        {std::regex(R"(\b1\b)"), "I"},  // Standalone "1" -> "I"
        {std::regex(R"(\b0\b)"), "O"},  // Standalone "0" -> "O"
        {std::regex(R"(\b5\b)"), "S"},  // Standalone "5" -> "S" (context dependent)
        
        // Fix multiple spaces
        {std::regex(R"(\s{2,})"), " "},
        
        // Fix common OCR line break errors
        {std::regex(R"(\b([a-z])-\s*\n\s*([a-z]))"), "$1$2"},  // word- \n continuation
        
        // Fix mis-recognized punctuation
        {std::regex(R"(\s+,)"), ","},   // Remove space before comma
        {std::regex(R"(\s+\.)"), "."},  // Remove space before period
        {std::regex(R"(\(\s+)"), "("},  // Remove space after opening paren
        {std::regex(R"(\s+\))"), ")"},  // Remove space before closing paren
        
        // Fix capitalization after periods
        {std::regex(R"(\.\s+([a-z]))"), ". $1"},  // Ensure space after period
        
        // Fix common OCR artifacts
        {std::regex(R"([|]{2,})"), "||"},  // Multiple vertical bars
        {std::regex(R"([-]{3,})"), "---"}, // Multiple dashes
        {std::regex(R"([_]{2,})"), "__"},  // Multiple underscores
    };
}
