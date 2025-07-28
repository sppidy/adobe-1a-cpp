#include "pdf_processor.hpp"
#include "text_corrector.hpp"
#include "heading_classifier.hpp" 
#include "yolo_inference.h"
#include "utils.hpp"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <cstdio>

#ifdef USE_MUPDF
#include <mupdf/fitz.h>
#endif

PDFProcessor::PDFProcessor() {
#ifdef USE_MUPDF
    // Initialize MuPDF context
    fz_ctx_ = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!fz_ctx_) {
        throw std::runtime_error("Failed to initialize MuPDF context");
    }
    fz_register_document_handlers(fz_ctx_);
#endif
    
    log_info("PDFProcessor initialized for sequential processing");
    
    // Initialize YOLO detector
    yolo_detector_ = std::make_unique<YOLOInference>();
    
    // Try to initialize with available models
    std::vector<std::string> model_paths = {
        "models/yolo_layout/",
        "models/PP-DocLayout-L/", 
        "models/PP-DocLayout-S/"
    };
    
    bool yolo_initialized = false;
    for (const auto& model_path : model_paths) {
        if (yolo_detector_->initialize(model_path)) {
            log_info("YOLO layout detector initialized with: " + model_path);
            yolo_initialized = true;
            break;
        }
    }
    
    if (!yolo_initialized) {
        log_info("YOLO not initialized - will use fallback detection");
    }
    
    // Initialize HeadingClassifier
    heading_classifier_ = std::make_unique<HeadingClassifier>();
    if (heading_classifier_->initialize("models/yolo_layout/")) {
        log_info("HeadingClassifier initialized successfully");
    } else {
        log_info("HeadingClassifier initialization failed - using basic patterns");
    }
    
    // Initialize HeadingClassifier
    heading_classifier_ = std::make_unique<HeadingClassifier>();
    if (heading_classifier_->initialize("models/yolo_layout/")) {
        log_info("HeadingClassifier initialized successfully");
    } else {
        log_info("HeadingClassifier initialization failed - using basic classification");
    }
}

PDFProcessor::~PDFProcessor() {
#ifdef USE_MUPDF
    if (fz_ctx_) {
        fz_drop_context(fz_ctx_);
    }
#endif
}

ProcessingResult PDFProcessor::process_pdf(const std::string& pdf_path, const std::string& output_json) {
    auto start_time = std::chrono::high_resolution_clock::now();
    ProcessingResult result;
    result.success = false;
    
    try {
        log_info("Processing PDF: " + pdf_path);
        
        // Step 1: Convert PDF to images
        TIME_BLOCK(pdf_conversion);
        auto images = pdf_to_images(pdf_path);
        TIME_END(pdf_conversion);
        
        if (images.empty()) {
            result.error_message = "No pages could be converted from PDF";
            return result;
        }
        
        // Step 2: Extract title
        result.title = extract_pdf_title(pdf_path);
        
        // Step 3: AI-powered heading detection (following 1.py workflow)
        TIME_BLOCK(heading_detection);
        current_pdf_path_ = pdf_path; // Store for table detection
        result.headings = ai_detect_headings(images, result.title);
        TIME_END(heading_detection);
        
        // Step 4: Save results
        save_results(result, output_json);
        
        result.success = true;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.processing_time_seconds = std::chrono::duration<double>(end_time - start_time).count();
        
        log_info("Processing completed successfully in " + std::to_string(result.processing_time_seconds) + "s");
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
        log_error("Processing failed: " + result.error_message);
    }
    
    return result;
}

std::vector<cv::Mat> PDFProcessor::pdf_to_images(const std::string& pdf_path) {
    std::vector<cv::Mat> images;
    
    if (!utils::file_exists(pdf_path)) {
        throw std::runtime_error("PDF file not found: " + pdf_path);
    }
    
#ifdef USE_MUPDF
    // Use MuPDF for optimal performance
    fz_document* doc = NULL;
    fz_page* page = NULL;
    fz_pixmap* pix = NULL;
    
    fz_try(fz_ctx_) {
        doc = fz_open_document(fz_ctx_, pdf_path.c_str());
        int page_count = fz_count_pages(fz_ctx_, doc);
        
        log_info("Converting " + std::to_string(page_count) + " pages at " + std::to_string(dpi_) + " DPI");
        
        for (int i = 0; i < page_count; ++i) {
            page = fz_load_page(fz_ctx_, doc, i);
            
            // Create transformation matrix for DPI
            fz_matrix transform = fz_scale(dpi_ / 72.0f, dpi_ / 72.0f);
            
            // Render page to pixmap
            pix = fz_new_pixmap_from_page(fz_ctx_, page, transform, fz_device_rgb(fz_ctx_), 0);
            
            // Convert to OpenCV Mat
            int width = fz_pixmap_width(fz_ctx_, pix);
            int height = fz_pixmap_height(fz_ctx_, pix);
            int stride = fz_pixmap_stride(fz_ctx_, pix);
            unsigned char* samples = fz_pixmap_samples(fz_ctx_, pix);
            
            // Create OpenCV Mat (RGB format)
            cv::Mat img(height, width, CV_8UC3);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int src_idx = y * stride + x * 3;
                    int dst_idx = y * width * 3 + x * 3;
                    img.data[dst_idx + 2] = samples[src_idx + 0]; // R
                    img.data[dst_idx + 1] = samples[src_idx + 1]; // G  
                    img.data[dst_idx + 0] = samples[src_idx + 2]; // B
                }
            }
            
            images.push_back(img.clone());
            
            fz_drop_pixmap(fz_ctx_, pix);
            fz_drop_page(fz_ctx_, page);
            pix = NULL;
            page = NULL;
        }
    }
    fz_always(fz_ctx_) {
        if (pix) fz_drop_pixmap(fz_ctx_, pix);
        if (page) fz_drop_page(fz_ctx_, page);
        if (doc) fz_drop_document(fz_ctx_, doc);
    }
    fz_catch(fz_ctx_) {
        throw std::runtime_error("MuPDF error during PDF conversion");
    }
    
#else
    // Fallback: This would require a different PDF library or external tool
    log_error("MuPDF not available. PDF processing not implemented in fallback mode.");
    throw std::runtime_error("PDF processing requires MuPDF library");
#endif
    
    return images;
}

std::string PDFProcessor::extract_pdf_title(const std::string& pdf_path) {
    // Universal title extraction approach
    
#ifdef USE_MUPDF
    try {
        fz_document* doc = fz_open_document(fz_ctx_, pdf_path.c_str());
        if (doc) {
            // Try PDF metadata first
            char info[512];
            if (fz_lookup_metadata(fz_ctx_, doc, FZ_META_INFO_TITLE, info, sizeof(info)) >= 0 && strlen(info) > 0) {
                fz_drop_document(fz_ctx_, doc);
                return std::string(info);
            }
            fz_drop_document(fz_ctx_, doc);
        }
    } catch (...) {
        // Fallback if metadata fails
    }
#endif
    
    // Fallback 1: Try to extract title from first page (largest/centered text)
    // This would require first page analysis - for now use filename
    
    // Fallback 2: Use clean filename
    std::string filename = utils::get_filename_without_extension(pdf_path);
    
    // Clean up common filename patterns
    std::regex pattern(R"([_\-\.]+)");
    std::string clean_title = std::regex_replace(filename, pattern, " ");
    
    // Capitalize first letter of each word
    std::stringstream ss(clean_title);
    std::string word, result;
    while (ss >> word) {
        if (!word.empty()) {
            word[0] = std::toupper(word[0]);
            result += (result.empty() ? "" : " ") + word;
        }
    }
    
    return result.empty() ? filename : result;
}

std::vector<HeadingInfo> PDFProcessor::detect_headings(const std::vector<cv::Mat>& images) {
    std::vector<HeadingInfo> all_headings;
    
    // Initialize components
    HeadingClassifier classifier;
    bool classifier_ready = classifier.initialize("models/yolo_layout");
    
    if (!classifier_ready) {
        std::cout << "Warning: Failed to initialize YOLO model, using fallback detection" << std::endl;
    }

    // Sequential processing only
    log_info("Processing " + std::to_string(images.size()) + " pages sequentially");
    
    for (size_t i = 0; i < images.size(); ++i) {
        // TODO: Implement actual heading detection
        // For now, create dummy heading for testing
        HeadingInfo dummy;
        dummy.level = "H2";
        dummy.text = "Sample heading from page " + std::to_string(i + 1);
        dummy.page_number = static_cast<int>(i + 1);
        dummy.confidence = 0.8;
        all_headings.push_back(dummy);
    }

    log_info("Found " + std::to_string(all_headings.size()) + " headings");
    return all_headings;
}

void PDFProcessor::save_results(const ProcessingResult& result, const std::string& output_path) {
    // TODO: Implement JSON output using nlohmann/json
    // For now, create a simple text output
    auto parent_path = std::filesystem::path(output_path).parent_path();
    if (!parent_path.empty()) {
        utils::ensure_directory_exists(parent_path);
    }
    
    std::ofstream file(output_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output file: " + output_path);
    }
    
    file << "{\n";
    file << "  \"title\": \"" << result.title << "\",\n";
    file << "  \"outline\": [\n";
    
    for (size_t i = 0; i < result.headings.size(); ++i) {
        const auto& heading = result.headings[i];
        file << "    {\n";
        file << "      \"level\": \"" << heading.level << "\",\n";
        file << "      \"text\": \"" << heading.text << "\",\n";
        file << "      \"page\": " << heading.page_number << "\n";
        file << "    }";
        if (i < result.headings.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    log_info("Results saved to: " + output_path);
}

void PDFProcessor::log_error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void PDFProcessor::log_info(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

// AI-powered heading detection using YOLO layout detection
std::vector<HeadingInfo> PDFProcessor::ai_detect_headings(const std::vector<cv::Mat>& images, const std::string& title) {
    std::vector<HeadingInfo> all_headings;
    
    if (!yolo_detector_ || !yolo_detector_->is_initialized()) {
        log_error("YOLO layout detector not available - falling back to basic detection");
        return detect_headings(images); // Use fallback method
    }
    
    log_info("Using YOLO-powered layout detection for " + std::to_string(images.size()) + " pages");
    
    // Sequential processing only
    log_info("Processing pages sequentially with YOLO inference");
    
    for (size_t i = 0; i < images.size(); ++i) {
        auto page_headings = process_single_page_ai(images[i], static_cast<int>(i + 1));
        all_headings.insert(all_headings.end(), page_headings.begin(), page_headings.end());
    }
    
    log_info("Found " + std::to_string(all_headings.size()) + " headings using AI detection");
    return all_headings;
}

std::vector<HeadingInfo> PDFProcessor::process_single_page_ai(const cv::Mat& image, int page_number) {
    std::vector<HeadingInfo> page_headings;
    
    try {
        // Step 1: Detect tables on this page using MuPDF  
        std::vector<cv::Rect> table_regions = detect_tables_on_page(current_pdf_path_, page_number);
        
        // Step 2: YOLO Layout Detection 
        if (!yolo_detector_ || !yolo_detector_->is_initialized()) {
            log_error("YOLO detector not available for page " + std::to_string(page_number));
            return page_headings;
        }
        
        // Get YOLO layout detection results
        auto layout_detections = yolo_detector_->detect_layout(image);
        
        log_info("Page " + std::to_string(page_number) + ": YOLO detected " + 
                std::to_string(layout_detections.size()) + " layout regions");
        
        // Step 3: Process each detected heading region, skipping tables
        for (const auto& detection : layout_detections) {
            // Convert BBox to cv::Rect
            cv::Rect bbox(static_cast<int>(detection.x1), 
                         static_cast<int>(detection.y1),
                         static_cast<int>(detection.x2 - detection.x1),
                         static_cast<int>(detection.y2 - detection.y1));
            
            // Skip if this region overlaps with tables
            if (is_region_overlapping_table(bbox, table_regions)) {
                continue;
            }
            
            // Only process title, paragraph_title, and text regions (potential headings)
            if (detection.label == "title" || detection.label == "paragraph_title" || detection.label == "text") {
                // Step 4: Crop heading region from image
                cv::Rect safe_bbox = bbox & cv::Rect(0, 0, image.cols, image.rows);
                if (safe_bbox.width <= 0 || safe_bbox.height <= 0) continue;
                
                // Step 5: OCR text extraction using Tesseract (like Python)
                std::string extracted_text = crop_and_ocr_text(image, safe_bbox);

                if (!extracted_text.empty() && extracted_text.length() > 2) {
                    // Step 6: T5 text correction (currently simplified)
                    TextCorrector corrector;
                    std::string corrected_text = corrector.correct_text(extracted_text);
                    
                    // Step 7: Use sophisticated HeadingClassifier to determine level
                    HeadingLevel classified_level = HeadingLevel::H2; // Default
                    if (heading_classifier_) {
                        classified_level = heading_classifier_->determine_heading_level(
                            corrected_text, detection.label, safe_bbox, page_number);
                    }
                    
                    // Skip if classified as non-heading (especially for "text" regions)
                    if (classified_level == HeadingLevel::UNKNOWN) {
                        continue;
                    }
                    
                    // Convert HeadingLevel enum to string
                    std::string heading_level = "H2"; // Default fallback
                    switch (classified_level) {
                        case HeadingLevel::H1: heading_level = "H1"; break;
                        case HeadingLevel::H2: heading_level = "H2"; break;
                        case HeadingLevel::H3: heading_level = "H3"; break;
                        case HeadingLevel::H4: heading_level = "H4"; break;
                        default: heading_level = "H2"; break;
                    }
                    
                    // Step 8: Create heading info
                    HeadingInfo heading;
                    heading.level = heading_level;
                    heading.text = corrected_text;
                    heading.page_number = page_number;
                    heading.bounding_box = bbox;
                    heading.confidence = detection.confidence;
                    
                    page_headings.push_back(heading);
                    
                    log_info("Page " + std::to_string(page_number) + ": Found " + heading_level + 
                            " heading: \"" + corrected_text.substr(0, 50) + "...\"");
                }
            }
        }
        
    } catch (const std::exception& e) {
        log_error("Error processing page " + std::to_string(page_number) + ": " + e.what());
    }
    
    return page_headings;
}

std::string PDFProcessor::crop_and_ocr_text(const cv::Mat& image, const cv::Rect& bbox) {
    try {
        // Step 1: Crop the region
        cv::Mat cropped = image(bbox);
        
        if (cropped.empty()) {
            return "";
        }
        
        // Step 2: Save cropped image temporarily for Tesseract
        std::string temp_crop = "/tmp/temp_crop_" + std::to_string(std::rand()) + ".png";
        cv::imwrite(temp_crop, cropped);
        
        // Step 3: Use Tesseract OCR (system call - matching Python behavior)
        std::string command = "tesseract " + temp_crop + " stdout --psm 6 2>/dev/null";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::remove(temp_crop.c_str());
            return "";
        }
        
        std::string result;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        pclose(pipe);
        
        // Step 4: Clean up temporary file
        std::remove(temp_crop.c_str());
        
        // Step 5: Clean text (remove extra whitespace, newlines)
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
        
        // Trim whitespace
        size_t start = result.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = result.find_last_not_of(" \t");
        result = result.substr(start, end - start + 1);
        
        return result;
        
    } catch (const std::exception& e) {
        log_error("OCR error: " + std::string(e.what()));
        return "";
    }
}

// Table detection using MuPDF structured text analysis
std::vector<cv::Rect> PDFProcessor::detect_tables_on_page(const std::string& pdf_path, int page_number) {
    std::vector<cv::Rect> table_regions;
    
#ifdef USE_MUPDF
    fz_document* doc = NULL;
    fz_page* page = NULL;
    fz_stext_page* stext = NULL;
    
    // Use a separate context for table detection to avoid stack issues
    fz_context* table_ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!table_ctx) {
        log_error("Failed to create MuPDF context for table detection");
        return table_regions;
    }
    
    fz_try(table_ctx) {
        fz_register_document_handlers(table_ctx);
        doc = fz_open_document(table_ctx, pdf_path.c_str());
        
        if (page_number - 1 >= fz_count_pages(table_ctx, doc)) {
            log_error("Page number out of range for table detection");
            fz_drop_document(table_ctx, doc);
            fz_drop_context(table_ctx);
            return table_regions;
        }
        
        page = fz_load_page(table_ctx, doc, page_number - 1); // Convert to 0-based index
        
        // Extract structured text with simplified options
        fz_stext_options opts = { 0 };
        opts.flags = 0; // Use default flags
        stext = fz_new_stext_page_from_page(table_ctx, page, &opts);
        
        // Simplified table detection: look for rectangular text arrangements
        // This is a basic implementation - real table detection would be more sophisticated
        
        std::vector<fz_rect> text_blocks;
        
        // Collect all text block positions
        for (fz_stext_block* block = stext->first_block; block; block = block->next) {
            if (block->type != FZ_STEXT_BLOCK_TEXT) continue;
            
            // Get bounding box of the entire text block
            fz_rect block_bbox = block->bbox;
            text_blocks.push_back(block_bbox);
        }
        
        // Simple heuristic: if we find many small, aligned text blocks, 
        // they might form a table
        if (text_blocks.size() >= 6) { // At least 6 text blocks for a potential table
            // Look for alignment patterns
            const float alignment_tolerance = 10.0f; // pixels
            
            // Group blocks by similar X positions (columns)
            std::vector<std::vector<fz_rect>> columns;
            
            for (const auto& block : text_blocks) {
                bool found_column = false;
                
                for (auto& column : columns) {
                    if (!column.empty()) {
                        float x_diff = std::abs(column[0].x0 - block.x0);
                        if (x_diff < alignment_tolerance) {
                            column.push_back(block);
                            found_column = true;
                            break;
                        }
                    }
                }
                
                if (!found_column) {
                    columns.push_back({block});
                }
            }
            
            // If we have 2+ columns with 2+ blocks each, it might be a table
            int valid_columns = 0;
            for (const auto& column : columns) {
                if (column.size() >= 2) {
                    valid_columns++;
                }
            }
            
            if (valid_columns >= 2) {
                // Create a table region encompassing all these blocks
                float min_x = 1000000, min_y = 1000000;
                float max_x = 0, max_y = 0;
                
                for (const auto& block : text_blocks) {
                    min_x = std::min(min_x, block.x0);
                    min_y = std::min(min_y, block.y0);
                    max_x = std::max(max_x, block.x1);
                    max_y = std::max(max_y, block.y1);
                }
                
                // Convert to OpenCV Rect (scale to match image coordinates)
                float scale = dpi_ / 72.0f;
                cv::Rect table_rect(
                    static_cast<int>(min_x * scale),
                    static_cast<int>(min_y * scale),
                    static_cast<int>((max_x - min_x) * scale),
                    static_cast<int>((max_y - min_y) * scale)
                );
                
                table_regions.push_back(table_rect);
            }
        }
        
    } fz_catch(table_ctx) {
        log_error("Table detection failed for page " + std::to_string(page_number) + ": " + 
                 std::string(fz_caught_message(table_ctx)));
    }
    
    // Clean up resources
    if (stext) fz_drop_stext_page(table_ctx, stext);
    if (page) fz_drop_page(table_ctx, page);
    if (doc) fz_drop_document(table_ctx, doc);
    fz_drop_context(table_ctx);
    
#else
    // Fallback: no table detection without MuPDF
    log_info("MuPDF not available - skipping table detection for page " + std::to_string(page_number));
#endif
    
    if (!table_regions.empty()) {
        log_info("Detected " + std::to_string(table_regions.size()) + " table(s) on page " + std::to_string(page_number));
    }
    
    return table_regions;
}

bool PDFProcessor::is_region_overlapping_table(const cv::Rect& region, const std::vector<cv::Rect>& table_regions) {
    for (const auto& table_rect : table_regions) {
        // Check for intersection
        cv::Rect intersection = region & table_rect;
        if (intersection.area() > 0) {
            // Calculate overlap percentage
            float overlap_percentage = static_cast<float>(intersection.area()) / static_cast<float>(region.area());
            
            // If more than 30% of the region overlaps with a table, consider it a table region
            if (overlap_percentage > 0.3f) {
                return true;
            }
        }
    }
    return false;
}
