#!/bin/bash

# Setup script for wxWidgets YOLO Person Detection Application

echo "=========================================="
echo "wxWidgets YOLO Person Detection Setup"
echo "=========================================="
echo ""

# Check if OpenCV is installed
echo "Checking for OpenCV..."
if ! pkg-config --exists opencv4 && ! pkg-config --exists opencv; then
    echo "OpenCV not found. Installing..."
    echo "Ubuntu/Debian:"
    echo "  sudo apt-get install libopencv-dev"
    echo ""
    echo "Fedora/RHEL:"
    echo "  sudo dnf install opencv-devel"
    echo ""
    read -p "Install OpenCV? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        if command -v apt-get &> /dev/null; then
            sudo apt-get update
            sudo apt-get install -y libopencv-dev
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y opencv-devel
        elif command -v brew &> /dev/null; then
            brew install opencv
        fi
    fi
else
    echo "OpenCV found."
fi

echo ""
echo "Building project..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Build the project
cd build
cmake ..
make

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "Next steps:"
    echo "1. Download YOLOv8 Small model:"
    echo "   cd build"
    echo "   wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8s.onnx"
    echo ""
    echo "2. Run the application:"
    echo "   ./wxapp"
    echo ""
else
    echo ""
    echo "Build failed. Please check the error messages above."
    exit 1
fi
