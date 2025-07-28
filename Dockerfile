# PDF Processor Docker Image - Arch Linux (Single Stage)
FROM archlinux:latest

# Install all dependencies (build + runtime)
RUN pacman -Sy --noconfirm \
    base-devel \
    cmake \
    git \
    wget \
    curl \
    opencv \
    mupdf \
    mupdf-tools \
    tesseract \
    tesseract-data-eng \
    leptonica \
    onnxruntime \
    nlohmann-json \
    pkgconf \
    && pacman -Scc --noconfirm

# Set working directory
WORKDIR /app

# Copy source code
COPY src/ ./src/
COPY CMakeLists.txt ./
RUN cp models.zip && \
    unzip models.zip && \
    rm models.zip

# Build the application
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    cp pdf_processor /usr/local/bin/

# Create output and input directories
RUN mkdir -p /app/output /app/input && \
    chmod 755 /app/output /app/input

# Create non-root user for security
RUN useradd -m -u 1000 pdfuser && \
    chown -R pdfuser:pdfuser /app
USER pdfuser

# Expose working directory as volume for input/output
VOLUME ["/app/input", "/app/output"]

# Set default command
ENTRYPOINT ["pdf_processor"]
CMD ["--help"]

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD pdf_processor --version || exit 1

# Labels for metadata
LABEL version="1.0.0"
LABEL description="PDF Processor - C++ implementation with AI-powered heading detection"
LABEL maintainer="Team RRP"
