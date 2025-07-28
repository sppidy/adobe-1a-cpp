#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>

// Utility macros for timing
#define TIME_BLOCK(name) auto start_##name = std::chrono::high_resolution_clock::now()
#define TIME_END(name) do { \
    auto end_##name = std::chrono::high_resolution_clock::now(); \
    auto duration_##name = std::chrono::duration_cast<std::chrono::milliseconds>(end_##name - start_##name); \
    std::cout << #name << " took: " << duration_##name.count() << "ms" << std::endl; \
} while(0)

namespace utils {
    
    // File system utilities
    bool file_exists(const std::string& path);
    std::string get_filename_without_extension(const std::string& path);
    void ensure_directory_exists(const std::string& path);
    
    // String utilities
    std::string trim(const std::string& str);
    std::string to_lower(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    bool starts_with(const std::string& str, const std::string& prefix);
    bool ends_with(const std::string& str, const std::string& suffix);
    
    // Validation utilities
    bool is_valid_heading_text(const std::string& text);
    bool contains_mostly_letters(const std::string& text, double threshold = 0.5);
    
    // Performance utilities
    class Timer {
    public:
        Timer(const std::string& name) : name_(name) {
            start_ = std::chrono::high_resolution_clock::now();
        }
        
        ~Timer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
            std::cout << name_ << " took: " << duration.count() << "ms" << std::endl;
        }
        
        double elapsed_ms() const {
            auto current = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(current - start_).count();
        }
        
    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
    };
    
    // Memory utilities
    template<typename T>
    class ObjectPool {
    public:
        ObjectPool(size_t initial_size = 10) {
            for (size_t i = 0; i < initial_size; ++i) {
                available_.push_back(std::make_unique<T>());
            }
        }
        
        std::unique_ptr<T> acquire() {
            if (available_.empty()) {
                return std::make_unique<T>();
            }
            
            auto obj = std::move(available_.back());
            available_.pop_back();
            return obj;
        }
        
        void release(std::unique_ptr<T> obj) {
            if (available_.size() < max_pool_size_) {
                available_.push_back(std::move(obj));
            }
        }
        
    private:
        std::vector<std::unique_ptr<T>> available_;
        static constexpr size_t max_pool_size_ = 50;
    };
    
} // namespace utils
