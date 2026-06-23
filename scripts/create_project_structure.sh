#!/bin/bash

set -e

echo "Creating qt-viewer project structure..."

# Directories
mkdir -p .github/workflows
mkdir -p cmake
mkdir -p docker
mkdir -p docs/images
mkdir -p include
mkdir -p src
mkdir -p resources/icons
mkdir -p samples
mkdir -p tests

# Source files
touch src/main.cpp
touch src/MainWindow.cpp
touch src/ImageViewer.cpp
touch src/ImageLoader.cpp

# Header files
touch include/MainWindow.h
touch include/ImageViewer.h
touch include/ImageLoader.h

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
