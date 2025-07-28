# PDF Processor - Usage Guide

## Quick Start with Docker (Recommended)

### Build the Docker Image
```bash
docker build -t pdf-processor .
```

### Process PDFs

#### Batch Processing (Process All PDFs in Input Folder)
```bash
# Create input and output directories
mkdir -p input output

# Place your PDF files in the input directory
cp your-documents*.pdf input/

# Process all PDFs automatically
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor

# Results will be saved in output/ with individual JSON files
```

#### Single File Processing
```bash
# Basic usage - single file
docker run --rm -v $(pwd):/workspace pdf-processor /workspace/your-document.pdf

# Custom output file
docker run --rm -v $(pwd):/workspace pdf-processor -o /workspace/results.json /workspace/your-document.pdf

# Higher quality rendering
docker run --rm -v $(pwd):/workspace pdf-processor --dpi 150 /workspace/your-document.pdf
```

## Native Build (Linux)

### Prerequisites
```bash
# Arch Linux
sudo pacman -S opencv tesseract tesseract-data-eng leptonica mupdf nlohmann-json cmake gcc

# Ubuntu/Debian
sudo apt update
sudo apt install libopencv-dev libtesseract-dev libleptonica-dev libmupdf-dev nlohmann-json3-dev cmake g++
```

### Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run
```bash
# Single file processing
./pdf_processor your-document.pdf

# Batch processing (requires /app/input and /app/output directories)
sudo mkdir -p /app/input /app/output
sudo cp *.pdf /app/input/
sudo chmod 755 /app/input /app/output
./pdf_processor

# Or create symlinks for local testing
mkdir -p input output
sudo ln -sf $(pwd)/input /app/input
sudo ln -sf $(pwd)/output /app/output
cp *.pdf input/
./pdf_processor
```

## Command Options

```
Usage: pdf_processor [OPTIONS] [pdf_file]

Options:
  --help, -h          Show help message
  --version, -v       Show version
  --dpi <value>       DPI for rendering (default: 100)
  --output, -o <file> Output file (default: /app/output/heading_schema.json)
  --verbose           Enable verbose logging

Behavior:
  If no PDF file is specified, processes all PDF files in /app/input/
  If a PDF file is specified, processes only that file
  Output files are saved to /app/output/ by default

Examples:
  pdf_processor                    # Process all PDFs in /app/input/
  pdf_processor document.pdf       # Process specific file
  pdf_processor --dpi 150 document.pdf
  pdf_processor -o results.json document.pdf
```

## Output

The tool generates JSON files with detected headings and their hierarchy:

### Single File Output
```json
{
  "title": "Document Title",
  "outline": [
    {
      "level": "H1",
      "text": "Chapter 1: Introduction",
      "page": 1
    }
  ]
}
```

### Batch Processing Output
When processing multiple files, each PDF gets its own output file:
- `input/document1.pdf` → `output/document1_headings.json`
- `input/document2.pdf` → `output/document2_headings.json`
- Progress summary shows total files processed and headings found

## Docker Examples

### Batch Processing Example
```bash
# Setup directories
mkdir -p input output
cp *.pdf input/

# Process all PDFs with verbose output
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor --verbose

# Results
ls output/
# document1_headings.json
# document2_headings.json
# document3_headings.json
```

### Advanced Docker Usage
```bash
# Custom DPI for all files
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor --dpi 150

# Process specific file with custom output
docker run --rm \
  -v $(pwd):/workspace \
  pdf-processor \
  --output /workspace/special-results.json \
  /workspace/important-document.pdf
```

## Requirements

- **For Batch Processing**: PDF files in `/app/input/` directory (Docker) or local `input/` directory
- **For Single File**: PDF input file path
- **YOLO Model**: `models/yolo_layout/yolov12.onnx` (included in Docker image)
- **Output Access**: Writable `/app/output/` directory (Docker) or local `output/` directory
- **Permissions**: Read access to input files, write access to output directory

```bash
# Clone and build
git clone <repository-url>
cd cpp_implementation
./build_docker.sh

# The script will:
# - Build the Docker image
# - Run comprehensive tests
# - Create helper scripts
```

### Method 2: Native Installation

For native installation, you need these dependencies:

**Ubuntu/Debian:**
```bash
sudo apt-get update && sudo apt-get install -y \
  build-essential cmake pkg-config \
  libopencv-dev libopencv-contrib-dev \
  libmupdf-dev mupdf-tools \
  tesseract-ocr tesseract-ocr-eng \
  libtesseract-dev libleptonica-dev \
  libonnxruntime-dev \
  nlohmann-json3-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake pkgconf \
  opencv mupdf mupdf-tools \
  tesseract tesseract-data-eng \
  onnxruntime nlohmann-json
```

**Build from source:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Docker Usage

### Basic Commands

```bash
# Show help
docker run --rm pdf-processor:latest --help

# Show version and features
docker run --rm pdf-processor:latest --version

# Process a PDF with default settings
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  /app/input/document.pdf
```

### Docker Compose

Use the provided `docker-compose.yml` for easier management:

```bash
# Process with docker-compose
docker-compose run --rm pdf-processor /app/input/document.pdf

# Development mode with source mounted
docker-compose run --rm pdf-processor-dev
```

### Advanced Docker Usage

```bash
# Custom output file
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  --output /app/output/custom-results.json \
  /app/input/document.pdf

# Verbose mode with higher DPI
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  --verbose --dpi 150 \
  /app/input/document.pdf

# Interactive shell for debugging
docker run --rm -it \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest bash
```

## Native Usage

Once compiled natively, use the executable directly:

```bash
# Basic usage
./pdf_processor document.pdf

# With custom options
./pdf_processor --verbose --dpi 150 --output results.json document.pdf

# Check capabilities
./pdf_processor --version
```

## Configuration Options

### Command Line Arguments

| Option | Short | Description | Default |
|--------|-------|-------------|---------|
| `--help` | `-h` | Show help message | - |
| `--version` | `-v` | Show version and features | - |
| `--dpi <value>` | - | PDF rendering resolution | 100 |
| `--output <file>` | `-o` | Output JSON file path | `output/heading_schema.json` |
| `--verbose` | - | Enable detailed logging | disabled |

### Performance Tuning

- **DPI Settings:**
  - `--dpi 100`: Fast processing, good for most documents
  - `--dpi 150`: Better accuracy for complex layouts
  - `--dpi 200`: High accuracy, slower processing

- **Docker Resource Limits:**
  ```bash
  docker run --rm \
    --memory=4g \
    --cpus=2 \
    -v $(pwd)/input:/app/input:ro \
    -v $(pwd)/output:/app/output:rw \
    pdf-processor:latest \
    /app/input/document.pdf
  ```

## Input/Output

### Input Requirements

- **Format:** PDF files (.pdf)
- **Size:** No strict limit, but larger files take more time
- **Content:** Works best with text-based PDFs (not scanned images)
- **Location:** 
  - Docker: Place files in `input/` directory
  - Native: Any accessible file path

### Output Format

The application generates a JSON file with this structure:

```json
{
  "title": "Document Title",
  "headings": [
    {
      "level": "H1",
      "text": "Main Heading",
      "page_number": 1,
      "bounding_box": {
        "x": 100,
        "y": 200,
        "width": 300,
        "height": 50
      },
      "confidence": 0.95
    }
  ],
  "processing_time_seconds": 2.45,
  "success": true,
  "error_message": ""
}
```

### Output Files

- **Default:** `output/heading_schema.json`
- **Custom:** Specify with `--output` option
- **Permissions:** Ensure output directory is writable

## Examples

### Example 1: Basic Document Processing

```bash
# Prepare files
mkdir -p input output
cp research-paper.pdf input/

# Process with Docker
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  /app/input/research-paper.pdf

# Check results
cat output/heading_schema.json | jq .
```

### Example 2: Batch Processing

```bash
# Process multiple PDFs
for pdf in input/*.pdf; do
  filename=$(basename "$pdf" .pdf)
  echo "Processing $filename..."
  docker run --rm \
    -v $(pwd)/input:/app/input:ro \
    -v $(pwd)/output:/app/output:rw \
    pdf-processor:latest \
    --output "/app/output/${filename}_headings.json" \
    "/app/input/$pdf"
done
```

### Example 3: High-Quality Processing

```bash
# For complex documents with small text
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  --verbose \
  --dpi 200 \
  --output /app/output/detailed-analysis.json \
  /app/input/complex-document.pdf
```

### Example 4: Development and Debugging

```bash
# Interactive debugging
docker run --rm -it \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest bash

# Inside container:
./pdf_processor --verbose /app/input/document.pdf
ls -la output/
cat output/heading_schema.json | head -20
```

## Troubleshooting

### Common Issues

#### 1. Permission Denied Errors

```bash
# Fix directory permissions
chmod 755 input output
chmod 644 input/*.pdf

# Or run with user mapping
docker run --rm \
  --user $(id -u):$(id -g) \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  /app/input/document.pdf
```

#### 2. File Not Found

```bash
# Check file path
ls -la input/
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  ls -la /app/input/
```

#### 3. Memory Issues

```bash
# For large PDFs, increase memory
docker run --rm \
  --memory=8g \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  /app/input/large-document.pdf
```

#### 4. Docker Build Failures

```bash
# Clean and rebuild
./build_docker.sh --clean

# Check Docker space
docker system df
docker system prune -f
```

### Debug Information

```bash
# Check application features
docker run --rm pdf-processor:latest --version

# Inspect dependencies
docker run --rm pdf-processor:latest ldd ./pdf_processor

# Check container contents
docker run --rm -it pdf-processor:latest bash
```

### Log Analysis

```bash
# Verbose mode provides detailed logs
docker run --rm \
  -v $(pwd)/input:/app/input:ro \
  -v $(pwd)/output:/app/output:rw \
  pdf-processor:latest \
  --verbose \
  /app/input/document.pdf 2>&1 | tee processing.log
```

## Performance Tips

### Optimization Strategies

1. **DPI Selection:**
   - Use `--dpi 100` for speed
   - Use `--dpi 150` for balance
   - Use `--dpi 200` only for complex documents

2. **Resource Allocation:**
   ```bash
   # Optimal for most systems
   docker run --rm \
     --memory=4g \
     --cpus=4 \
     [...]
   ```

3. **Batch Processing:**
   ```bash
   # Process multiple files efficiently
   find input -name "*.pdf" -print0 | \
   xargs -0 -n1 -P4 -I{} bash -c '
     name=$(basename "{}" .pdf)
     docker run --rm [...] --output "output/${name}.json" "{}"
   '
   ```

### Performance Benchmarks

| DPI | Speed | Accuracy | Use Case |
|-----|--------|----------|----------|
| 100 | Fast | Good | Quick processing |
| 150 | Medium | Better | Standard documents |
| 200 | Slow | Best | Complex layouts |

## Technical Details

### AI Models

- **YOLO Model:** Document layout detection
- **Location:** `models/yolo_layout/yolov12.onnx`
- **Purpose:** Identifies text regions, headings, tables, figures

### Processing Pipeline

1. **PDF Rendering:** Convert PDF pages to images
2. **Layout Detection:** AI-powered region identification
3. **Text Extraction:** OCR with Tesseract
4. **Classification:** Heading level determination
5. **Output Generation:** JSON schema creation

### Dependencies

| Component | Purpose | Docker | Native |
|-----------|---------|---------|---------|
| OpenCV | Image processing | ✓ | Required |
| MuPDF | PDF parsing | ✓ | Required |
| Tesseract | OCR | ✓ | Required |
| ONNX Runtime | AI inference | ✓ | Required |
| C++17 Compiler | Build | ✓ | Required |

### Architecture

```
PDF Input → MuPDF → Image Pages → YOLO Detection → 
Text Regions → Tesseract OCR → Text Analysis → 
Heading Classification → JSON Output
```

## Support

### Getting Help

1. **Check the logs:** Use `--verbose` flag
2. **Verify setup:** Run `--version` command
3. **Test with simple PDF:** Start with a basic document
4. **Check permissions:** Ensure read/write access

### Reporting Issues

Include this information:
- PDF Processor version (`--version` output)
- Input file characteristics
- Command used
- Error messages
- System information (Docker version, OS)

### Testing

Use the provided test script:
```bash
./test_docker.sh
```

This guide covers all aspects of using the PDF Processor. For additional questions, refer to the source code documentation or create an issue in the project repository.
