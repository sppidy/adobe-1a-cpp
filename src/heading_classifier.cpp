#include "heading_classifier.hpp"
#include <algorithm>
#include <iostream>
#include <regex>

HeadingClassifier::HeadingClassifier() {
    initialize_pattern_matchers();
}

bool HeadingClassifier::initialize(const std::string& model_path) {
    try {
        yolo_detector_ = std::make_unique<YOLOInference>();
        initialized_ = yolo_detector_->initialize(model_path);
        
        if (initialized_) {
            std::cout << "HeadingClassifier initialized with YOLO model: " << model_path << std::endl;
        } else {
            std::cout << "Warning: YOLO initialization failed, using fallback detection" << std::endl;
        }
        
        return initialized_;
    } catch (const std::exception& e) {
        std::cerr << "HeadingClassifier initialization error: " << e.what() << std::endl;
        return false;
    }
}

HeadingLevel HeadingClassifier::determine_heading_level(const std::string& text, 
                                                      const std::string& layout_label,
                                                      const cv::Rect& bbox,
                                                      int page_number) {
    if (text.empty() || text.length() < 3) {
        return HeadingLevel::UNKNOWN; // Too short to be meaningful
    }
    
    // Filter out obvious non-headings first
    if (is_likely_body_text(text)) {
        return HeadingLevel::UNKNOWN;
    }
    
    std::string lower_text = to_lower(text);
    
    // Priority 1: Check layout-based classification first (YOLO provides good hints)
    HeadingLevel layout_level = classify_by_layout_label(layout_label);
    if (layout_level != HeadingLevel::UNKNOWN) {
        // Validate layout classification with text patterns
        if (validate_heading_candidate(text, layout_level)) {
            return layout_level;
        }
    }
    
    // Priority 2: Pattern-based classification with strict validation
    HeadingLevel pattern_level = classify_by_patterns(text, page_number);
    if (pattern_level != HeadingLevel::UNKNOWN) {
        if (validate_heading_candidate(text, pattern_level)) {
            return pattern_level;
        }
    }
    
    // Priority 3: Structural analysis for remaining candidates
    if (layout_label == "text" && has_heading_structure(text)) {
        HeadingLevel structural_level = classify_by_structure(text, page_number);
        if (structural_level != HeadingLevel::UNKNOWN) {
            return structural_level;
        }
    }
    
    return HeadingLevel::UNKNOWN; // Default to not a heading
}

std::vector<LayoutRegion> HeadingClassifier::detect_layout_regions(const cv::Mat& image) {
    std::vector<LayoutRegion> regions;
    
    if (!initialized_ || !yolo_detector_) {
        std::cout << "Warning: Using fallback layout detection" << std::endl;
        // Fallback: Create basic regions based on image analysis
        LayoutRegion dummy;
        dummy.bbox = cv::Rect(0, 0, image.cols, image.rows / 10);
        dummy.label = "title";
        dummy.confidence = 0.8;
        regions.push_back(dummy);
        return regions;
    }
    
    try {
        // Use YOLO inference for layout detection
        std::vector<BBox> yolo_results = yolo_detector_->detect_layout(image);
        
        // Convert YOLO results to LayoutRegion format
        for (const auto& bbox : yolo_results) {
            LayoutRegion region;
            region.bbox = cv::Rect(static_cast<int>(bbox.x1), 
                                 static_cast<int>(bbox.y1),
                                 static_cast<int>(bbox.x2 - bbox.x1),
                                 static_cast<int>(bbox.y2 - bbox.y1));
            region.label = bbox.label;
            region.confidence = bbox.confidence;
            regions.push_back(region);
        }
        
        std::cout << "YOLO detected " << regions.size() << " layout regions" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Layout detection error: " << e.what() << std::endl;
    }
    
    return regions;
}

void HeadingClassifier::set_document_context(const std::string& title, int total_pages) {
    document_title_ = title;
    total_pages_ = total_pages;
}

bool HeadingClassifier::matches_h1_patterns(const std::string& text, int page_number) {
    // Check all H1 indicators
    for (const auto& indicator : h1_indicators_) {
        if (indicator(text)) {
            return true;
        }
    }
    
    // Page 1 titles with sufficient length are usually H1
    if (page_number == 1 && text.length() > 20) {
        return true;
    }
    
    return false;
}

bool HeadingClassifier::matches_h2_patterns(const std::string& text) {
    for (const auto& indicator : h2_indicators_) {
        if (indicator(text)) {
            return true;
        }
    }
    return false;
}

bool HeadingClassifier::matches_h3_patterns(const std::string& text) {
    for (const auto& indicator : h3_indicators_) {
        if (indicator(text)) {
            return true;
        }
    }
    return false;
}

bool HeadingClassifier::matches_h4_patterns(const std::string& text) {
    for (const auto& indicator : h4_indicators_) {
        if (indicator(text)) {
            return true;
        }
    }
    return false;
}

bool HeadingClassifier::is_likely_body_text(const std::string& text) {
    // Filter out obvious body text patterns
    size_t word_count = std::count(text.begin(), text.end(), ' ') + 1;
    
    // Very long text is usually body text
    if (text.length() > 200 || word_count > 25) {
        return true;
    }
    
    // Text ending with periods (except abbreviations) is usually body text
    if (text.back() == '.' && text.length() > 50) {
        return true;
    }
    
    // Text with multiple sentences is usually body text
    size_t sentence_count = std::count(text.begin(), text.end(), '.') + 
                           std::count(text.begin(), text.end(), '!') + 
                           std::count(text.begin(), text.end(), '?');
    if (sentence_count > 1) {
        return true;
    }
    
    // Check for common body text indicators
    std::string lower = to_lower(text);
    if (lower.find("the ") == 0 || lower.find("this ") == 0 || 
        lower.find("in ") == 0 || lower.find("for ") == 0 ||
        lower.find("with ") == 0 || lower.find("as ") == 0) {
        if (word_count > 8) { // Only for longer text
            return true;
        }
    }
    
    return false;
}

HeadingLevel HeadingClassifier::classify_by_patterns(const std::string& text, int page_number) {
    // Check H1 patterns first (most important)
    if (matches_h1_patterns(text, page_number)) {
        return HeadingLevel::H1;
    }
    
    // Check H4 patterns (most specific)
    if (matches_h4_patterns(text)) {
        return HeadingLevel::H4;
    }
    
    // Check H3 patterns (specific sections)
    if (matches_h3_patterns(text)) {
        return HeadingLevel::H3;
    }
    
    // Check H2 patterns (general sections)
    if (matches_h2_patterns(text)) {
        return HeadingLevel::H2;
    }
    
    return HeadingLevel::UNKNOWN;
}

bool HeadingClassifier::validate_heading_candidate(const std::string& text, HeadingLevel proposed_level) {
    size_t word_count = std::count(text.begin(), text.end(), ' ') + 1;
    
    // Length validation based on heading level
    switch (proposed_level) {
        case HeadingLevel::H1:
            // H1 should be substantial but not too long
            return text.length() >= 10 && text.length() <= 150 && word_count <= 20;
            
        case HeadingLevel::H2:
            // H2 can be shorter than H1
            return text.length() >= 5 && text.length() <= 120 && word_count <= 15;
            
        case HeadingLevel::H3:
            // H3 should be concise
            return text.length() >= 3 && text.length() <= 100 && word_count <= 12;
            
        case HeadingLevel::H4:
            // H4 should be very concise
            return text.length() >= 3 && text.length() <= 80 && word_count <= 10;
            
        default:
            return false;
    }
}

bool HeadingClassifier::has_heading_structure(const std::string& text) {
    // Check for structural patterns that indicate headings
    
    // 1. Numbered sections (1., 2.1, I., A., etc.)
    if (std::regex_search(text, std::regex(R"(^\s*(?:\d+\.|\d+\.\d+\.?|[IVX]+\.?|[A-Z]\.)\s)"))) {
        return true;
    }
    
    // 2. Text ending with colon (section headers)
    if (text.back() == ':' && text.length() > 5 && text.length() < 80) {
        return true;
    }
    
    // 3. All caps text (but not too long)
    if (text.length() > 3 && text.length() < 50) {
        bool all_caps = true;
        int letter_count = 0;
        for (char c : text) {
            if (std::isalpha(c)) {
                letter_count++;
                if (std::islower(c)) {
                    all_caps = false;
                    break;
                }
            }
        }
        if (all_caps && letter_count > 2) {
            return true;
        }
    }
    
    // 4. Title case with most words capitalized
    std::istringstream iss(text);
    std::string word;
    int capitalized = 0, total = 0;
    while (iss >> word && total < 10) {
        if (!word.empty() && std::isalpha(word[0])) {
            total++;
            if (std::isupper(word[0])) {
                capitalized++;
            }
        }
    }
    
    // At least 70% of words capitalized and reasonable word count
    if (total >= 2 && total <= 8 && (float)capitalized / total >= 0.7) {
        return true;
    }
    
    return false;
}

HeadingLevel HeadingClassifier::classify_by_structure(const std::string& text, int page_number) {
    size_t word_count = std::count(text.begin(), text.end(), ' ') + 1;
    
    // First page long titles are likely H1
    if (page_number == 1 && text.length() > 20 && word_count >= 3) {
        return HeadingLevel::H1;
    }
    
    // Numbered major sections are likely H1 or H2
    if (std::regex_search(text, std::regex(R"(^\s*(?:\d+\.|\d+\s+[A-Z]|[IVX]+\.)\s)"))) {
        return word_count <= 6 ? HeadingLevel::H1 : HeadingLevel::H2;
    }
    
    // Text ending with colon suggests section headers
    if (text.back() == ':') {
        return word_count <= 4 ? HeadingLevel::H3 : HeadingLevel::H4;
    }
    
    // Based on length and capitalization
    if (word_count <= 3) {
        return HeadingLevel::H4;
    } else if (word_count <= 6) {
        return HeadingLevel::H3;
    } else if (word_count <= 10) {
        return HeadingLevel::H2;
    }
    
    return HeadingLevel::UNKNOWN;
}

HeadingLevel HeadingClassifier::classify_by_length(const std::string& text) {
    size_t word_count = std::count(text.begin(), text.end(), ' ') + 1;
    
    // Be more generous with short text that could be headings
    if (word_count == 1) {
        return HeadingLevel::H4; // Single words might be H4
    } else if (word_count <= 2) {
        return HeadingLevel::H3; // Very short texts might be H3
    } else if (word_count <= 6) {
        return HeadingLevel::H3; // Short texts are likely H3
    } else if (word_count <= 12) {
        return HeadingLevel::H2; // Medium texts might be H2
    } else if (word_count >= 25) {
        return HeadingLevel::UNKNOWN; // Very long texts are likely body text
    } else {
        return HeadingLevel::H3; // Default to H3 for intermediate lengths
    }
}

HeadingLevel HeadingClassifier::classify_by_layout_label(const std::string& label) {
    return convert_yolo_label_to_heading_level(label);
}

HeadingLevel HeadingClassifier::convert_yolo_label_to_heading_level(const std::string& yolo_label) {
    // Map YOLO layout labels to heading levels
    if (yolo_label == "title") {
        return HeadingLevel::H1;
    }
    
    if (yolo_label == "text") {
        return HeadingLevel::H2;
    }
    
    if (yolo_label == "list") {
        return HeadingLevel::H3;
    }
    
    // Non-heading labels
    if (yolo_label == "figure" || yolo_label == "table" || 
        yolo_label == "header" || yolo_label == "footer" ||
        yolo_label == "reference" || yolo_label == "equation") {
        return HeadingLevel::UNKNOWN;
    }
    
    // Default fallback
    return HeadingLevel::UNKNOWN;
}

void HeadingClassifier::initialize_pattern_matchers() {
    // H1 indicators (major document sections only)
    h1_indicators_ = {
        [](const std::string& t) {
            // Major document sections only
            std::string lower = t;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            return lower.find("abstract") == 0 || lower.find("introduction") == 0 ||
                   lower.find("executive summary") == 0 || lower.find("conclusion") == 0 ||
                   lower.find("appendix") == 0 || lower.find("summary") == 0 ||
                   lower.find("phase i") != std::string::npos || 
                   lower.find("phase ii") != std::string::npos ||
                   lower.find("phase iii") != std::string::npos;
        },
        [](const std::string& t) {
            // Major numbered sections (1., I., etc.)
            return std::regex_search(t, std::regex(R"(^(chapter|section|part|phase)\s+[IVX0-9])", std::regex_constants::icase));
        }
    };
    
    // H2 indicators (section headers)  
    h2_indicators_ = {
        [](const std::string& t) {
            // Common section headers
            std::string lower = t;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            return lower == "background" || lower == "methodology" || 
                   lower == "results" || lower == "discussion" || 
                   lower == "references" || lower == "bibliography" ||
                   lower == "acknowledgments" || lower.find("timeline:") == 0 ||
                   lower.find("evaluation") == 0 || lower.find("funding") == 0;
        },
        [](const std::string& t) {
            // Numbered subsections (2.1, 3.2, etc.)
            return std::regex_search(t, std::regex(R"(^\d+\.\d+)"));
        }
    };
    
    // H3 indicators (subsection headers)
    h3_indicators_ = {
        [](const std::string& t) {
            // Numbered lists and subsections
            return std::regex_search(t, std::regex(R"(^\d+\.?\s)")) ||
                   std::regex_search(t, std::regex(R"(^[a-z]\)\s)", std::regex_constants::icase));
        },
        [](const std::string& t) {
            // Text ending with colon (section markers)
            return t.back() == ':' && t.length() > 5 && t.length() < 60;
        }
    };
    
    // H4 indicators (minor headings)
    h4_indicators_ = {
        [](const std::string& t) {
            // Dates and timestamps
            return std::regex_search(t, std::regex(R"(\b\d{1,2}[/-]\d{1,2}[/-]\d{2,4}\b)")) ||
                   std::regex_search(t, std::regex(R"(\b(january|february|march|april|may|june|july|august|september|october|november|december)\s+\d{1,2},?\s+\d{4})", std::regex_constants::icase)) ||
                   std::regex_search(t, std::regex(R"(\btimeline:\s*)", std::regex_constants::icase));
        },
        [](const std::string& t) {
            // Short bullet points or labels
            return (t.front() == '-' || t.front() == '*') && t.length() < 50;
        }
    };
}

std::string HeadingClassifier::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
