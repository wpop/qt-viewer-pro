#!/bin/bash

set -e

echo "Creating qt-viewer project structure..."

# Directories
mkdir -p .github/workflows
mkdir -p cmake
mkdir -p docker
mkdir -p docs/images
mkdir -p include/qtviewerpro/ui
mkdir -p include/qtviewerpro/io
mkdir -p include/qtviewerpro/processing
mkdir -p src/ui
mkdir -p src/io
mkdir -p src/processing
mkdir -p resources/icons
mkdir -p samples
mkdir -p tests

# Source files
touch src/main.cpp
touch src/ui/MainWindow.cpp
touch src/ui/ImageViewer.cpp
touch src/io/ImageLoader.cpp
touch src/processing/ImageProcessor.cpp

# Header files
touch include/qtviewerpro/ui/MainWindow.h
touch include/qtviewerpro/ui/ImageViewer.h
touch include/qtviewerpro/io/ImageLoader.h
touch include/qtviewerpro/processing/ImageProcessor.h

# Resources
touch resources/resources.qrc

# Documentation
touch docs/architecture.md
touch README.md
touch LICENSE

# Build files
touch CMakeLists.txt
touch .gitignore
touch .clang-format

# Docker
touch docker/Dockerfile
touch docker/docker-compose.yml

echo
echo "Project structure created successfully!"
