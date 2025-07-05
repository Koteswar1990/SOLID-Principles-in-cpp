# CMakeLists.txt for Enhanced OtoscopyWorker with Click-to-Reposition

cmake_minimum_required(VERSION 3.16)
project(OtoscopyWorkerEnhanced)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Find other dependencies (adjust these based on your project structure)
find_package(xtensor REQUIRED)
find_package(utility REQUIRED)  # Your utility library

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utility
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/resource
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)

# Enhanced OtoscopyWorker library
set(OTOSCOPY_WORKER_SOURCES
    OtoscopyWorker.cpp
    OtoscopyWorker.h
    OtoscopeTransform.cpp
    OtoscopeTransform.h
    OtoScopyLabel.cpp
    OtoScopyLabel.h
    RemoteOssiviewRoutine.cpp
)

# Create the enhanced OtoscopyWorker library
add_library(OtoscopyWorkerEnhanced ${OTOSCOPY_WORKER_SOURCES})

# Link libraries
target_link_libraries(OtoscopyWorkerEnhanced
    Qt6::Core
    Qt6::Widgets
    xtensor
    utility::processing
    utility::logger
    app::Panel
    resource::OtoscopyOverlay
    config::VisualizationCalibration
)

# Set target properties
set_target_properties(OtoscopyWorkerEnhanced PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Compiler-specific options
target_compile_options(OtoscopyWorkerEnhanced PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)

# Export include directory
target_include_directories(OtoscopyWorkerEnhanced PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Optional: Create a test executable
if(BUILD_TESTING)
    add_executable(OtoscopyWorkerTest test/main.cpp)
    target_link_libraries(OtoscopyWorkerTest OtoscopyWorkerEnhanced)
endif()

# Installation
install(TARGETS OtoscopyWorkerEnhanced
    EXPORT OtoscopyWorkerEnhancedTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    OtoscopyWorker.h
    OtoscopeTransform.h
    OtoScopyLabel.h
    DESTINATION include
)

install(EXPORT OtoscopyWorkerEnhancedTargets
    FILE OtoscopyWorkerEnhancedTargets.cmake
    NAMESPACE OtoscopyWorkerEnhanced::
    DESTINATION lib/cmake/OtoscopyWorkerEnhanced
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/OtoscopyWorkerEnhancedConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/OtoscopyWorkerEnhancedConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/OtoscopyWorkerEnhancedConfig.cmake"
    INSTALL_DESTINATION lib/cmake/OtoscopyWorkerEnhanced
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OtoscopyWorkerEnhancedConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OtoscopyWorkerEnhancedConfigVersion.cmake"
    DESTINATION lib/cmake/OtoscopyWorkerEnhanced
)