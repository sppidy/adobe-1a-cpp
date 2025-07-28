#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <memory>

#ifdef USE_MUPDF
#include <mupdf/fitz.h>
#endif

// Forward declarations
class YOLOInference;
class HeadingClassifier;

struct HeadingInfo {
    std::string level;  // "H1", "H2", "H3"
    std::string text;
    int page_number;
    cv::Rect bounding_box;
    double confidence;
};

struct ProcessingResult {
    std::string title;
    std::vector<HeadingInfo> headings;
    bool success;
    std::string error_message;
    double processing_time_seconds;
};

class PDFProcessor {
public:
    PDFProcessor();
    ~PDFProcessor();
    
    // Main processing function
    ProcessingResult process_pdf(const std::string& pdf_path, 
                               const std::string& output_json = "output/heading_schema.json");
    
    // Configuration options
    void set_dpi(int dpi) { dpi_ = dpi; }
    
    // Utility functions
    static std::string get_version() { return "1.0.0"; }
    
private:
    // Core processing steps
    std::vector<cv::Mat> pdf_to_images(const std::string& pdf_path);
    std::string extract_pdf_title(const std::string& pdf_path);
    std::vector<HeadingInfo> detect_headings(const std::vector<cv::Mat>& images);
    
    // AI-powered heading detection (following 1.py workflow)
    std::vector<HeadingInfo> ai_detect_headings(const std::vector<cv::Mat>& images, const std::string& title);
    std::vector<HeadingInfo> process_single_page_ai(const cv::Mat& image, int page_number);
    std::string crop_and_ocr_text(const cv::Mat& image, const cv::Rect& bbox);
    
    // Table detection using MuPDF
    std::vector<cv::Rect> detect_tables_on_page(const std::string& pdf_path, int page_number);
    bool is_region_overlapping_table(const cv::Rect& region, const std::vector<cv::Rect>& table_regions);
    
    void save_results(const ProcessingResult& result, const std::string& output_path);
    
    // Configuration
    int dpi_ = 100;  // Optimized for speed
    
    // Internal state
#ifdef USE_MUPDF
    fz_context* fz_ctx_ = nullptr;
#endif
    
    // YOLO inference for layout detection
    std::unique_ptr<YOLOInference> yolo_detector_;
    
    // Heading classification
    std::unique_ptr<HeadingClassifier> heading_classifier_;
    
    // Current PDF path for table detection
    std::string current_pdf_path_;
    
    // Error handling
    void log_error(const std::string& message);
    void log_info(const std::string& message);
};
