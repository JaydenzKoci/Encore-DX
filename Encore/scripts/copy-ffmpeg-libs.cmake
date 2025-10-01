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
    
    # Create symlinks for versioned libraries if they don't exist
    # This ensures that libname.so points to libname.so.X
    set(FFMPEG_LIBS_TO_LINK
        "swresample"
        "avcodec"
        "avformat"
        "avutil"
        "swscale"
        "avfilter"
        "avdevice"
    )
    
    foreach(LIB_BASE ${FFMPEG_LIBS_TO_LINK})
        set(BASE_SO "${TARGET_DIR}/lib${LIB_BASE}.so")
        
        # Find the highest versioned library for this base name
        file(GLOB VERSIONED_LIBS "${TARGET_DIR}/lib${LIB_BASE}.so.*")
        if(VERSIONED_LIBS)
            list(GET VERSIONED_LIBS 0 HIGHEST_VERSION)
            get_filename_component(HIGHEST_VERSION_NAME ${HIGHEST_VERSION} NAME)
            
            # Create symlink if base .so doesn't exist or is not a symlink to the right target
            if(NOT EXISTS ${BASE_SO})
                message(STATUS "  Creating symlink: lib${LIB_BASE}.so -> ${HIGHEST_VERSION_NAME}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E create_symlink ${HIGHEST_VERSION_NAME} lib${LIB_BASE}.so
                    WORKING_DIRECTORY ${TARGET_DIR}
                    RESULT_VARIABLE SYMLINK_RESULT
                )
                if(NOT SYMLINK_RESULT EQUAL 0)
                    message(STATUS "  Warning: Failed to create symlink for lib${LIB_BASE}.so")
                endif()
            endif()
        endif()
    endforeach()
    
    message(STATUS "Copied ${COPIED_COUNT} FFmpeg .so files and created necessary symlinks")
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()