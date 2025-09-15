# Helper script to copy FFmpeg .so files for Linux
# This script is called during post-build to copy FFmpeg shared libraries

# Get environment variables passed from the main build
set(FFMPEG_LIB_DIR "$ENV{FFMPEG_LIB_DIR}")
set(TARGET_DIR "$ENV{TARGET_DIR}")

if(FFMPEG_LIB_DIR AND TARGET_DIR)
    # Find all .so files in the FFmpeg lib directory
    file(GLOB FFMPEG_SO_FILES "${FFMPEG_LIB_DIR}/*.so*")
    
    if(FFMPEG_SO_FILES)
        message(STATUS "Copying ${list(LENGTH FFMPEG_SO_FILES)} FFmpeg .so files...")
        foreach(SO_FILE ${FFMPEG_SO_FILES})
            get_filename_component(FILENAME ${SO_FILE} NAME)
            message(STATUS "  Copying ${FILENAME}")
            file(COPY ${SO_FILE} DESTINATION ${TARGET_DIR})
        endforeach()
    else()
        message(STATUS "No FFmpeg .so files found in ${FFMPEG_LIB_DIR}")
    endif()
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()