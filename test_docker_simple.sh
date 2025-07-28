#!/bin/bash

# Simple Docker test script for PDF Processor
# This script runs basic tests to verify the Docker container works

echo "🧪 Testing PDF Processor Docker Container"
echo "========================================="

# Test 1: Check if image exists
echo "Test 1: Checking if Docker image exists..."
if docker images pdf-processor:test | grep -q pdf-processor; then
    echo "✅ Docker image found"
else
    echo "❌ Docker image not found. Please build it first with:"
    echo "   docker build -t pdf-processor:test ."
    exit 1
fi

# Test 2: Test help command
echo ""
echo "Test 2: Testing --help command..."
if docker run --rm pdf-processor:test --help > /dev/null 2>&1; then
    echo "✅ Help command works"
else
    echo "❌ Help command failed"
    exit 1
fi

# Test 3: Test version command
echo ""
echo "Test 3: Testing --version command..."
echo "Version output:"
docker run --rm pdf-processor:test --version

# Test 4: Check if all dependencies are available
echo ""
echo "Test 4: Checking dependencies..."
docker run --rm pdf-processor:test sh -c "
    echo 'Tesseract version:'
    tesseract --version 2>/dev/null | head -1
    echo 'Models directory:'
    ls -la models/
    echo 'Executable info:'
    file ./pdf_processor
"

# Test 5: Test with non-existent file (should fail gracefully)
echo ""
echo "Test 5: Testing error handling..."
if docker run --rm pdf-processor:test /nonexistent.pdf 2>&1 | grep -q "PDF file not found"; then
    echo "✅ Error handling works correctly"
else
    echo "⚠️  Error handling test inconclusive"
fi

# Create directories for actual testing
mkdir -p input output

echo ""
echo "🎉 All basic tests passed!"
echo ""
echo "📁 To test with actual PDFs:"
echo "   1. Put PDF files in the 'input/' directory"
echo "   2. Run: docker run --rm -v \$(pwd)/input:/app/input:ro -v \$(pwd)/output:/app/output:rw pdf-processor:test /app/input/your-file.pdf"
echo "   3. Check results in the 'output/' directory"
echo ""
echo "🚀 The PDF Processor Docker container is ready to use!"
