#pragma once

#include <string>

// Common bounding box structure for layout detection
struct BBox {
    float x1, y1, x2, y2;  // Coordinates
    float confidence;       // Detection confidence
    int class_id;          // Class identifier
    std::string label;     // Human-readable label
};
