#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cctype>

#include "pdf_processor.hpp"
#include "utils.hpp"

// Helper function to find all PDF files in a directory
std::vector<std::string> find_pdf_files(const std::string& directory) {
    std::vector<std::string> pdf_files;
    
    if (!std::filesystem::exists(directory)) {
        return pdf_files;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();
                
                // Convert extension to lowercase for comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".pdf") {
                    pdf_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing directory " << directory << ": " << e.what() << "\n";
    }
    
    // Sort files for consistent processing order
    std::sort(pdf_files.begin(), pdf_files.end());
    return pdf_files;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [pdf_file]\n"
              << "\nOptions:\n"
              << "  --help, -h          Show this help message\n"
              << "  --version, -v       Show version information\n"
              << "  --dpi <value>       Set DPI for PDF rendering (default: 100)\n"
              << "  --output, -o <file> Output JSON file path (default: /app/output/heading_schema.json)\n"
              << "  --verbose           Enable verbose logging\n"
              << "\nBehavior:\n"
              << "  If no PDF file is specified, processes all PDF files in /app/input/\n"
              << "  If a PDF file is specified, processes only that file\n"
              << "  Output files are saved to /app/output/ by default\n"
              << "\nExamples:\n"
              << "  " << program_name << "                    # Process all PDFs in /app/input/\n"
              << "  " << program_name << " document.pdf       # Process specific file\n"
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
    std::string output_file = "/app/output/heading_schema.json";
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
    
    // Determine files to process
    std::vector<std::string> files_to_process;
    
    if (pdf_file.empty()) {
        // No specific file provided, process all PDFs in /app/input/
        files_to_process = find_pdf_files("/app/input");
        if (files_to_process.empty()) {
            std::cerr << "Error: No PDF files found in /app/input/\n";
            std::cerr << "Either specify a PDF file or place PDF files in /app/input/\n";
            return 1;
        }
        std::cout << "Found " << files_to_process.size() << " PDF file(s) in /app/input/\n";
    } else {
        // Specific file provided
        if (!utils::file_exists(pdf_file)) {
            std::cerr << "Error: PDF file not found: " << pdf_file << "\n";
            return 1;
        }
        files_to_process.push_back(pdf_file);
    }
    
    // Create processor and configure
    PDFProcessor processor;
    processor.set_dpi(dpi);
    
    // Process each file
    int successful_files = 0;
    int total_headings = 0;
    double total_time = 0.0;
    
    for (size_t i = 0; i < files_to_process.size(); ++i) {
        const std::string& current_file = files_to_process[i];
        
        // Generate output filename for each file
        std::string current_output;
        if (files_to_process.size() == 1) {
            current_output = output_file;
        } else {
            // Multiple files: generate unique output names
            std::filesystem::path input_path(current_file);
            std::string base_name = input_path.stem().string();
            std::filesystem::path output_path(output_file);
            std::string output_dir = output_path.parent_path().string();
            std::string output_ext = output_path.extension().string();
            
            if (output_dir.empty()) output_dir = "/app/output";
            current_output = output_dir + "/" + base_name + "_headings" + output_ext;
        }
        
        if (verbose || files_to_process.size() > 1) {
            std::cout << "\n[" << (i + 1) << "/" << files_to_process.size() << "] Processing: " << current_file << "\n";
            std::cout << "Configuration:\n"
                      << "  PDF File: " << current_file << "\n"
                      << "  Output: " << current_output << "\n"
                      << "  DPI: " << dpi << "\n"
                      << "  Processing: Sequential only\n"
                      << "\n";
        }
        
        // Process PDF
        utils::Timer file_timer("File processing");
        
        try {
            auto result = processor.process_pdf(current_file, current_output);
            
            if (result.success) {
                successful_files++;
                total_headings += result.headings.size();
                total_time += result.processing_time_seconds;
                
                std::cout << "✓ " << current_file << " processed successfully!\n"
                          << "  Title: " << result.title << "\n"
                          << "  Headings found: " << result.headings.size() << "\n"
                          << "  Processing time: " << result.processing_time_seconds << "s\n"
                          << "  Output saved to: " << current_output << "\n";
                
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
            } else {
                std::cerr << "✗ Failed to process " << current_file << ": " << result.error_message << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "✗ Unexpected error processing " << current_file << ": " << e.what() << "\n";
        }
    }
    
    // Summary for multiple files
    if (files_to_process.size() > 1) {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "BATCH PROCESSING SUMMARY\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Files processed: " << successful_files << "/" << files_to_process.size() << "\n";
        std::cout << "Total headings found: " << total_headings << "\n";
        std::cout << "Total processing time: " << total_time << "s\n";
        std::cout << "Average time per file: " << (successful_files > 0 ? total_time / successful_files : 0.0) << "s\n";
    }
    
    return (successful_files > 0) ? 0 : 1;
}
