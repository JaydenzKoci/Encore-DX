# Helper script to copy FFmpeg .so files for Linux
# This script is called during post-build to copy FFmpeg shared libraries

# Get environment variables passed from the main build
set(FFMPEG_LIB_DIR "$ENV{FFMPEG_LIB_DIR}")
set(TARGET_DIR "$ENV{TARGET_DIR}")

if(FFMPEG_LIB_DIR AND TARGET_DIR)
    # Define the specific FFmpeg libraries we want (base .so files and specific versioned ones)
    set(FFMPEG_LIBS
        "libavcodec.so"
        "libavformat.so"
        "libavutil.so"
        "libswscale.so"
        "libswresample.so"
        "libswresample.so.6"
        "libavfilter.so"
        "libavdevice.so"
    )
    
    set(COPIED_COUNT 0)
    foreach(LIB_NAME ${FFMPEG_LIBS})
        set(LIB_PATH "${FFMPEG_LIB_DIR}/${LIB_NAME}")
        if(EXISTS ${LIB_PATH})
            message(STATUS "  Copying ${LIB_NAME}")
            file(COPY ${LIB_PATH} DESTINATION ${TARGET_DIR})
            math(EXPR COPIED_COUNT "${COPIED_COUNT} + 1")
        else()
            message(STATUS "  Skipping ${LIB_NAME} (not found)")
        endif()
    endforeach()
    
    message(STATUS "Copied ${COPIED_COUNT} FFmpeg .so files (base versions + libswresample.so.6)")
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()