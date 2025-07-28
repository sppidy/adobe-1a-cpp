#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>
#include "common_types.h"

// ONNX Runtime headers
#ifdef USE_ONNX_RUNTIME
#include <onnxruntime_cxx_api.h>
#endif

class YOLOInference {
public:
    YOLOInference();
    ~YOLOInference();
    
    // Initialize with ONNX model
    bool initialize(const std::string& model_dir);
    
    // Detect layout regions in image
    std::vector<BBox> detect_layout(const cv::Mat& image);
    
    // Check if YOLO model is available
    bool is_initialized() const { return initialized_; }
    
private:
    bool initialized_;
    
#ifdef USE_ONNX_RUNTIME
    std::unique_ptr<Ort::Env> ort_env_;
    std::unique_ptr<Ort::Session> ort_session_;
    std::unique_ptr<Ort::SessionOptions> session_options_;
    
    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::vector<int64_t> input_shape_;
#endif
    
    float conf_threshold_;
    float nms_threshold_;
    std::vector<std::string> class_names_;
    
    // Preprocessing
    cv::Mat preprocess_image(const cv::Mat& image);
    
    // Postprocessing  
    std::vector<BBox> postprocess_detections(const std::vector<float>& output_data,
                                           int output_width, int output_height,
                                           float scale_x, float scale_y);
    
    // YOLO11-specific postprocessing for [batch, attributes, detections] format
    std::vector<BBox> postprocess_yolo11_detections(const std::vector<float>& output_data,
                                                   int num_attributes, int num_detections,
                                                   float scale_x, float scale_y);
    
    // Map COCO class ID to layout class name
    std::string map_coco_to_layout_class(int coco_class_id);
    
    // Map document layout class ID to layout class name  
    std::string map_doclayout_to_class(int class_id);
    
    // Non-Maximum Suppression
    std::vector<int> nms(const std::vector<BBox>& boxes, float threshold);
    
    // Fallback detection
    std::vector<BBox> create_fallback_layout(const cv::Mat& image);
    std::vector<BBox> create_fallback_layout_detections();
    
    // Load configuration
    bool load_config(const std::string& config_path);
    
    // Initialization methods
#ifdef USE_ONNX_RUNTIME
    bool initialize_onnx(const std::string& model_path, const std::string& config_path);
#endif
    
    // Class names mapping
    const std::vector<std::string> DEFAULT_CLASSES = {
        "text", "title", "list", "table", "figure",
        "paragraph_title", "formula", "reference", 
        "caption", "footnote", "header", "footer"
    };
};
