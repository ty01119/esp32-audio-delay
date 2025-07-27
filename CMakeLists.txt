# ESP32 Audio Delay Project
cmake_minimum_required(VERSION 3.16)

# Set the project name
set(PROJECT_NAME "esp32_audio_delay")

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(${PROJECT_NAME})
