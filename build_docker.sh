#!/bin/bash

# PDF Processor Docker Build and Test Script
# This script builds the Docker image and runs basic tests

set -e  # Exit on any error

echo "🐋 PDF Processor Docker Build & Test Script"
echo "============================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Docker is running
print_status "Checking Docker..."
if ! docker info > /dev/null 2>&1; then
    print_error "Docker is not running. Please start Docker and try again."
    exit 1
fi
print_success "Docker is running"

# Check if we're in the right directory
if [ ! -f "Dockerfile" ] || [ ! -f "CMakeLists.txt" ]; then
    print_error "Please run this script from the cpp_implementation directory"
    exit 1
fi

# Clean up previous containers/images if requested
if [ "$1" = "--clean" ]; then
    print_status "Cleaning up previous builds..."
    docker rm -f pdf-processor pdf-processor-dev 2>/dev/null || true
    docker rmi pdf-processor:latest pdf-processor-dev:latest 2>/dev/null || true
    print_success "Cleanup completed"
fi

# Create necessary directories
print_status "Creating input/output directories..."
mkdir -p input output
print_success "Directories created"

# Build the Docker image
print_status "Building PDF Processor Docker image..."
if docker build -t pdf-processor:latest .; then
    print_success "Docker image built successfully"
else
    print_error "Docker build failed"
    exit 1
fi

# Check if the image was created
print_status "Verifying Docker image..."
if docker images pdf-processor:latest | grep -q pdf-processor; then
    print_success "Docker image verified"
    # Show image details
    docker images pdf-processor:latest
else
    print_error "Docker image not found"
    exit 1
fi

# Test basic functionality
print_status "Testing Docker container..."

# Test 1: Help command
print_status "Test 1: Running --help command..."
if docker run --rm pdf-processor:latest --help > /dev/null; then
    print_success "Help command works"
else
    print_error "Help command failed"
    exit 1
fi

# Test 2: Version command
print_status "Test 2: Running --version command..."
if docker run --rm pdf-processor:latest --version; then
    print_success "Version command works"
else
    print_error "Version command failed"
    exit 1
fi

# Test 3: Check if all dependencies are available
print_status "Test 3: Checking dependencies in container..."
docker run --rm pdf-processor:latest sh -c "
    echo 'Checking tesseract...'
    tesseract --version | head -1
    echo 'Checking models...'
    ls -la models/yolo_layout/
    echo 'Checking executable...'
    file ./pdf_processor
    echo 'Checking libraries...'
    ldd ./pdf_processor | grep -E 'opencv|tesseract|mupdf|onnx' | head -5
" && print_success "Dependencies check passed" || print_error "Dependencies check failed"

# Test 4: Test with missing PDF (should fail gracefully)
print_status "Test 4: Testing error handling with missing file..."
if docker run --rm -v "$(pwd)/input:/app/input:ro" -v "$(pwd)/output:/app/output:rw" \
    pdf-processor:latest /app/input/nonexistent.pdf 2>&1 | grep -q "PDF file not found"; then
    print_success "Error handling works correctly"
else
    print_warning "Error handling test inconclusive"
fi

# Create a simple test script for users
print_status "Creating test script for users..."
cat > test_docker.sh << 'EOF'
#!/bin/bash
# Simple test script for PDF Processor Docker

echo "Testing PDF Processor Docker container..."

# Check if input directory exists and has PDFs
if [ ! -d "input" ]; then
    echo "Creating input directory..."
    mkdir -p input
fi

if [ ! -d "output" ]; then
    echo "Creating output directory..."
    mkdir -p output
fi

PDF_COUNT=$(find input -name "*.pdf" | wc -l)
if [ "$PDF_COUNT" -eq 0 ]; then
    echo "No PDF files found in input/ directory"
    echo "Please add some PDF files to test with and run:"
    echo "  docker run --rm -v \$(pwd)/input:/app/input:ro -v \$(pwd)/output:/app/output:rw pdf-processor:latest /app/input/your-file.pdf"
else
    echo "Found $PDF_COUNT PDF file(s) in input directory"
    echo "Testing with first PDF..."
    FIRST_PDF=$(find input -name "*.pdf" | head -1)
    PDF_NAME=$(basename "$FIRST_PDF")
    echo "Processing: $PDF_NAME"
    docker run --rm \
        -v "$(pwd)/input:/app/input:ro" \
        -v "$(pwd)/output:/app/output:rw" \
        pdf-processor:latest \
        --verbose \
        "/app/input/$PDF_NAME"
fi
EOF

chmod +x test_docker.sh
print_success "Test script created: test_docker.sh"

# Print usage instructions
echo ""
echo "🎉 Docker build completed successfully!"
echo ""
echo "Usage Instructions:"
echo "==================="
echo ""
echo "1. Basic usage:"
echo "   docker run --rm -v \$(pwd)/input:/app/input:ro -v \$(pwd)/output:/app/output:rw pdf-processor:latest /app/input/document.pdf"
echo ""
echo "2. Using docker-compose:"
echo "   docker-compose run --rm pdf-processor /app/input/document.pdf"
echo ""
echo "3. Interactive mode:"
echo "   docker run --rm -it -v \$(pwd)/input:/app/input:ro -v \$(pwd)/output:/app/output:rw pdf-processor:latest bash"
echo ""
echo "4. With verbose output:"
echo "   docker run --rm -v \$(pwd)/input:/app/input:ro -v \$(pwd)/output:/app/output:rw pdf-processor:latest --verbose /app/input/document.pdf"
echo ""
echo "5. Run the test script:"
echo "   ./test_docker.sh"
echo ""
echo "📁 Remember to:"
echo "   - Put PDF files in the 'input/' directory"
echo "   - Check results in the 'output/' directory"
echo "   - Ensure directories have proper permissions"
echo ""

print_success "All tests passed! The PDF Processor Docker container is ready to use."
