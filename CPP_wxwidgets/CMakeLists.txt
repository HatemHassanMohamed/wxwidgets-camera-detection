cmake_minimum_required(VERSION 3.10)
project(wxWidgetsApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/frame.cpp
)

set(HEADERS
    include/frame.h
)

# Create executable
add_executable(wxapp ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(wxapp 
    ${wxWidgets_LIBRARIES}
    ${OpenCV_LIBS}
)
