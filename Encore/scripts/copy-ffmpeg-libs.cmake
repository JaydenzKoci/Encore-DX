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
    
    # Copy only major version .so files (like libname.so.6, not libname.so.6.0.100 or libname.so)
    file(GLOB ALL_SO_FILES "${FFMPEG_LIB_DIR}/*.so*")
    
    # Special check for swresample files
    file(GLOB SWRESAMPLE_FILES "${FFMPEG_LIB_DIR}/libswresample*")
    message(STATUS "Swresample files found:")
    foreach(FILE ${SWRESAMPLE_FILES})
        get_filename_component(FILENAME ${FILE} NAME)
        message(STATUS "  - ${FILENAME}")
    endforeach()
    
    set(COPIED_COUNT 0)
    foreach(LIB_FILE ${ALL_SO_FILES})
        get_filename_component(LIB_NAME ${LIB_FILE} NAME)
        
        # Check if this is a major version file (ends with .so.X where X is a single number)
        if(LIB_NAME MATCHES "^lib.*\\.so\\.[0-9]+$")
            message(STATUS "  Copying major version: ${LIB_NAME}")
            
            # Use configure_file instead of file(COPY) for better error handling
            configure_file(${LIB_FILE} ${TARGET_DIR}/${LIB_NAME} COPYONLY)
            
            # Verify the file was copied successfully
            if(EXISTS "${TARGET_DIR}/${LIB_NAME}")
                message(STATUS "    ✓ Successfully copied to ${TARGET_DIR}/${LIB_NAME}")
                math(EXPR COPIED_COUNT "${COPIED_COUNT} + 1")
            else()
                message(STATUS "    ✗ Failed to copy ${LIB_NAME}")
            endif()
        else()
            message(STATUS "  Skipping: ${LIB_NAME} (not a major version)")
        endif()
    endforeach()
    
    # Create base symlinks for major version libraries
    # This ensures libname.so -> libname.so.X for proper dynamic linking
    set(FFMPEG_LIBS_TO_LINK
        "swresample"
        "avcodec"
        "avformat"
        "avutil"
        "swscale"
        "avfilter"
        "avdevice"
        "postproc"
    )
    
    foreach(LIB_BASE ${FFMPEG_LIBS_TO_LINK})
        # Find the major version file for this library
        file(GLOB MAJOR_VERSION_FILE "${TARGET_DIR}/lib${LIB_BASE}.so.[0-9]*")
        if(MAJOR_VERSION_FILE)
            list(GET MAJOR_VERSION_FILE 0 VERSION_FILE)
            get_filename_component(VERSION_FILENAME ${VERSION_FILE} NAME)
            
            set(BASE_SYMLINK "${TARGET_DIR}/lib${LIB_BASE}.so")
            
            # Create symlink if it doesn't exist
            if(NOT EXISTS ${BASE_SYMLINK})
                message(STATUS "  Creating symlink: lib${LIB_BASE}.so -> ${VERSION_FILENAME}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E create_symlink ${VERSION_FILENAME} lib${LIB_BASE}.so
                    WORKING_DIRECTORY ${TARGET_DIR}
                    RESULT_VARIABLE SYMLINK_RESULT
                )
                if(SYMLINK_RESULT EQUAL 0)
                    message(STATUS "    ✓ Symlink created successfully")
                else()
                    message(STATUS "    ✗ Failed to create symlink (result: ${SYMLINK_RESULT})")
                endif()
            else()
                message(STATUS "  Symlink already exists: lib${LIB_BASE}.so")
            endif()
        else()
            message(STATUS "  No major version file found for lib${LIB_BASE}")
        endif()
    endforeach()
    
    # Final verification - check what's actually in the target directory
    message(STATUS "Final files in target directory:")
    file(GLOB TARGET_FILES "${TARGET_DIR}/lib*.so*")
    foreach(FILE ${TARGET_FILES})
        get_filename_component(FILENAME ${FILE} NAME)
        if(IS_SYMLINK ${FILE})
            message(STATUS "  - ${FILENAME} (symlink)")
        else()
            message(STATUS "  - ${FILENAME}")
        endif()
    endforeach()
    
    # Check dependencies of libswresample specifically
    find_program(LDD_PROGRAM ldd)
    if(LDD_PROGRAM)
        message(STATUS "Checking dependencies of libswresample.so.6:")
        execute_process(
            COMMAND ${LDD_PROGRAM} ${TARGET_DIR}/libswresample.so.6
            OUTPUT_VARIABLE LDD_OUTPUT
            ERROR_VARIABLE LDD_ERROR
            RESULT_VARIABLE LDD_RESULT
        )
        if(LDD_RESULT EQUAL 0)
            message(STATUS "Dependencies:")
            string(REPLACE "\n" "\n  " LDD_FORMATTED "  ${LDD_OUTPUT}")
            message(STATUS "${LDD_FORMATTED}")
        else()
            message(STATUS "ldd failed: ${LDD_ERROR}")
        endif()
        
        # Also check libavcodec for comparison
        message(STATUS "Checking dependencies of libavcodec.so.62 for comparison:")
        execute_process(
            COMMAND ${LDD_PROGRAM} ${TARGET_DIR}/libavcodec.so.62
            OUTPUT_VARIABLE LDD_OUTPUT2
            ERROR_VARIABLE LDD_ERROR2
            RESULT_VARIABLE LDD_RESULT2
        )
        if(LDD_RESULT2 EQUAL 0)
            message(STATUS "Dependencies:")
            string(REPLACE "\n" "\n  " LDD_FORMATTED2 "  ${LDD_OUTPUT2}")
            message(STATUS "${LDD_FORMATTED2}")
        endif()
    else()
        message(STATUS "ldd not found, cannot check dependencies")
    endif()
    
    # Fix RPATH on all copied libraries to ensure they can find each other
    find_program(PATCHELF_PROGRAM patchelf)
    if(PATCHELF_PROGRAM)
        message(STATUS "Fixing RPATH on copied libraries...")
        file(GLOB COPIED_LIBS "${TARGET_DIR}/lib*.so.[0-9]*")
        foreach(LIB_FILE ${COPIED_LIBS})
            get_filename_component(LIB_NAME ${LIB_FILE} NAME)
            message(STATUS "  Setting RPATH on ${LIB_NAME}")
            execute_process(
                COMMAND ${PATCHELF_PROGRAM} --set-rpath '$ORIGIN' ${LIB_FILE}
                RESULT_VARIABLE PATCHELF_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            if(NOT PATCHELF_RESULT EQUAL 0)
                message(STATUS "    Warning: Failed to set RPATH on ${LIB_NAME}")
            endif()
        endforeach()
    else()
        message(STATUS "patchelf not found, cannot fix library RPATH")
        message(STATUS "Consider installing patchelf for better library compatibility")
    endif()
    
    message(STATUS "Copied ${COPIED_COUNT} FFmpeg major version .so files and created symlinks")
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()