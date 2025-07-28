#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <opencv2/opencv.hpp>
#include "yolo_inference.h"

enum class HeadingLevel {
    H1,
    H2, 
    H3,
    H4,
    UNKNOWN
};

struct LayoutRegion {
    cv::Rect bbox;
    std::string label;
    double confidence;
};

class HeadingClassifier {
public:
    HeadingClassifier();
    ~HeadingClassifier() = default;
    
    // Initialization with YOLO model
    bool initialize(const std::string& model_path = "models/yolo_layout");
    
    // Main classification function
    HeadingLevel determine_heading_level(const std::string& text, 
                                       const std::string& layout_label = "",
                                       const cv::Rect& bbox = cv::Rect(),
                                       int page_number = 1);
    
    // Layout detection integration with YOLO
    std::vector<LayoutRegion> detect_layout_regions(const cv::Mat& image);
    
    // Configuration
    void set_document_context(const std::string& title, int total_pages);
    
private:
    // YOLO inference engine
    std::unique_ptr<YOLOInference> yolo_detector_;
    bool initialized_ = false;
    
    // Rule-based classification
    bool matches_h1_patterns(const std::string& text, int page_number);
    bool matches_h2_patterns(const std::string& text);
    bool matches_h3_patterns(const std::string& text);
    bool matches_h4_patterns(const std::string& text);
    
    // Improved classification methods
    bool is_likely_body_text(const std::string& text);
    HeadingLevel classify_by_patterns(const std::string& text, int page_number);
    bool validate_heading_candidate(const std::string& text, HeadingLevel proposed_level);
    bool has_heading_structure(const std::string& text);
    HeadingLevel classify_by_structure(const std::string& text, int page_number);
    
    // Context analysis
    HeadingLevel classify_by_length(const std::string& text);
    HeadingLevel classify_by_layout_label(const std::string& label);
    HeadingLevel convert_yolo_label_to_heading_level(const std::string& yolo_label);
    
    // Pattern matching functions
    std::vector<std::function<bool(const std::string&)>> h1_indicators_;
    std::vector<std::function<bool(const std::string&)>> h2_indicators_;
    std::vector<std::function<bool(const std::string&)>> h3_indicators_;
    std::vector<std::function<bool(const std::string&)>> h4_indicators_;
    
    // Document context
    std::string document_title_;
    int total_pages_ = 0;
    
    // Helper functions
    void initialize_pattern_matchers();
    std::string to_lower(const std::string& str);
};
