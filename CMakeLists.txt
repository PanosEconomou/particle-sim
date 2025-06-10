cmake_minimum_required(VERSION 3.10)
project(VulkanComputeCore)

set(CMAKE_CXX_STANDARD 17)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Source files
set(SOURCES
    main.cpp
    vulkan_context.cpp
    compute_pipeline.cpp
)

# Header files (optional, helps IDEs)
set(HEADERS
    vulkan_context.h
    compute_pipeline.h
)

add_executable(particles ${SOURCES} ${HEADERS})

target_include_directories(particles PRIVATE ${Vulkan_INCLUDE_DIR})
target_link_libraries(particles PRIVATE ${Vulkan_LIBRARY})

# Optionally enable warnings
target_compile_options(particles PRIVATE -Wall -Wextra -pedantic)

