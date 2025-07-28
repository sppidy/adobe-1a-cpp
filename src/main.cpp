#include <iostream>
#include <string>
#include <chrono>

#include "pdf_processor.hpp"
#include "utils.hpp"

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <pdf_file>\n"
              << "\nOptions:\n"
              << "  --help, -h          Show this help message\n"
              << "  --version, -v       Show version information\n"
              << "  --dpi <value>       Set DPI for PDF rendering (default: 100)\n"
              << "  --output, -o <file> Output JSON file path (default: output/heading_schema.json)\n"
              << "  --verbose           Enable verbose logging\n"
              << "\nExamples:\n"
              << "  " << program_name << " document.pdf\n"
              << "  " << program_name << " --dpi 150 document.pdf\n"
              << "  " << program_name << " -o results.json document.pdf\n";
}

void print_version() {
    std::cout << "PDF Processor C++ Implementation\n"
              << "Version: " << PDFProcessor::get_version() << "\n"
              << "Built: " << __DATE__ << " " << __TIME__ << "\n"
              << "\nFeatures:\n";
              
#ifdef USE_MUPDF
    std::cout << "  ✓ MuPDF support\n";
#else
    std::cout << "  ✗ MuPDF support\n";
#endif

#ifdef USE_ONNX_RUNTIME  
    std::cout << "  ✓ ONNX Runtime support\n";
#else
    std::cout << "  ✗ ONNX Runtime support (using OpenCV DNN fallback)\n";
#endif

    std::cout << "  ✓ OpenCV " << CV_VERSION << "\n"
              << "  ✓ Sequential processing only\n";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string pdf_file;
    std::string output_file = "output/heading_schema.json";
    int dpi = 100;
    bool verbose = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        else if (arg == "--version" || arg == "-v") {
            print_version();
            return 0;
        }
        else if (arg == "--verbose") {
            verbose = true;
        }
        else if (arg == "--dpi" && i + 1 < argc) {
            dpi = std::stoi(argv[++i]);
        }
        else if ((arg == "--output" || arg == "-o") && i + 1 < argc) {
            output_file = argv[++i];
        }
        else if (arg[0] != '-') {
            if (pdf_file.empty()) {
                pdf_file = arg;
            } else {
                std::cerr << "Error: Multiple PDF files specified\n";
                return 1;
            }
        }
        else {
            std::cerr << "Error: Unknown option " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Validate arguments
    if (pdf_file.empty()) {
        std::cerr << "Error: No PDF file specified\n";
        print_usage(argv[0]);
        return 1;
    }
    
    if (!utils::file_exists(pdf_file)) {
        std::cerr << "Error: PDF file not found: " << pdf_file << "\n";
        return 1;
    }
    
    // Create processor and configure
    PDFProcessor processor;
    processor.set_dpi(dpi);
    
    if (verbose) {
        std::cout << "Configuration:\n"
                  << "  PDF File: " << pdf_file << "\n"
                  << "  Output: " << output_file << "\n"
                  << "  DPI: " << dpi << "\n"
                  << "  Processing: Sequential only\n"
                  << "\n";
    }
    
    // Process PDF
    utils::Timer total_timer("Total processing");
    
    try {
        auto result = processor.process_pdf(pdf_file, output_file);
        
        if (result.success) {
            std::cout << "✓ Processing completed successfully!\n"
                      << "  Title: " << result.title << "\n"
                      << "  Headings found: " << result.headings.size() << "\n"
                      << "  Processing time: " << result.processing_time_seconds << "s\n"
                      << "  Output saved to: " << output_file << "\n";
            
            if (verbose) {
                std::cout << "\nHeading breakdown:\n";
                int h1_count = 0, h2_count = 0, h3_count = 0;
                for (const auto& heading : result.headings) {
                    if (heading.level == "H1") h1_count++;
                    else if (heading.level == "H2") h2_count++;
                    else if (heading.level == "H3") h3_count++;
                }
                std::cout << "  H1: " << h1_count << ", H2: " << h2_count << ", H3: " << h3_count << "\n";
            }
            
            return 0;
        } else {
            std::cerr << "✗ Processing failed: " << result.error_message << "\n";
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Unexpected error: " << e.what() << "\n";
        return 1;
    }
}
