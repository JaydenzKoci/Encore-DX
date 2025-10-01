# Helper script to copy FFmpeg .so files for Linux
# This script is called during post-build to copy FFmpeg shared libraries

# Get environment variables passed from the main build
set(FFMPEG_LIB_DIR "$ENV{FFMPEG_LIB_DIR}")
set(TARGET_DIR "$ENV{TARGET_DIR}")

if(FFMPEG_LIB_DIR AND TARGET_DIR)
    # First, let's see what's actually available in the FFmpeg lib directory
    message(STATUS "Checking FFmpeg lib directory: ${FFMPEG_LIB_DIR}")
    if(EXISTS ${FFMPEG_LIB_DIR})
        file(GLOB ALL_LIB_FILES "${FFMPEG_LIB_DIR}/*")
        message(STATUS "Available files in FFmpeg lib directory:")
        foreach(FILE ${ALL_LIB_FILES})
            get_filename_component(FILENAME ${FILE} NAME)
            message(STATUS "  - ${FILENAME}")
        endforeach()
    else()
        message(STATUS "FFmpeg lib directory does not exist!")
    endif()
    
    # Copy all .so files (both base and versioned)
    file(GLOB FFMPEG_SO_FILES "${FFMPEG_LIB_DIR}/*.so*")
    
    set(COPIED_COUNT 0)
    foreach(LIB_FILE ${FFMPEG_SO_FILES})
        get_filename_component(LIB_NAME ${LIB_FILE} NAME)
        message(STATUS "  Copying ${LIB_NAME}")
        file(COPY ${LIB_FILE} DESTINATION ${TARGET_DIR})
        math(EXPR COPIED_COUNT "${COPIED_COUNT} + 1")
    endforeach()
    
    message(STATUS "Copied ${COPIED_COUNT} FFmpeg .so files (all versions)")
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()