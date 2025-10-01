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
    
    # Copy both major version files AND base symlinks from source
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
        
        # Copy major version files (ends with .so.X where X is a single number)
        if(LIB_NAME MATCHES "^lib.*\\.so\\.[0-9]+$")
            message(STATUS "  Copying major version: ${LIB_NAME}")
            configure_file(${LIB_FILE} ${TARGET_DIR}/${LIB_NAME} COPYONLY)
            
            if(EXISTS "${TARGET_DIR}/${LIB_NAME}")
                message(STATUS "    ✓ Successfully copied to ${TARGET_DIR}/${LIB_NAME}")
                math(EXPR COPIED_COUNT "${COPIED_COUNT} + 1")
            else()
                message(STATUS "    ✗ Failed to copy ${LIB_NAME}")
            endif()
        # Also copy base .so files (symlinks) from source
        elseif(LIB_NAME MATCHES "^lib.*\\.so$" AND IS_SYMLINK ${LIB_FILE})
            message(STATUS "  Copying base symlink: ${LIB_NAME}")
            configure_file(${LIB_FILE} ${TARGET_DIR}/${LIB_NAME} COPYONLY)
            
            if(EXISTS "${TARGET_DIR}/${LIB_NAME}")
                message(STATUS "    ✓ Successfully copied symlink ${LIB_NAME}")
                math(EXPR COPIED_COUNT "${COPIED_COUNT} + 1")
            else()
                message(STATUS "    ✗ Failed to copy symlink ${LIB_NAME}")
            endif()
        else()
            message(STATUS "  Skipping: ${LIB_NAME} (not needed)")
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
    
    # Verify specific files exist
    message(STATUS "Verifying critical files:")
    set(CRITICAL_FILES "libavutil.so.60" "libswresample.so.6")
    foreach(CRITICAL_FILE ${CRITICAL_FILES})
        if(EXISTS "${TARGET_DIR}/${CRITICAL_FILE}")
            message(STATUS "  ✓ ${CRITICAL_FILE} exists")
            # Check file permissions
            execute_process(
                COMMAND ls -la ${TARGET_DIR}/${CRITICAL_FILE}
                OUTPUT_VARIABLE LS_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            message(STATUS "    Permissions: ${LS_OUTPUT}")
        else()
            message(STATUS "  ✗ ${CRITICAL_FILE} missing!")
        endif()
    endforeach()
    
    # Check dependencies of libswresample specifically
    find_program(LDD_PROGRAM ldd)
    if(LDD_PROGRAM AND EXISTS "${TARGET_DIR}/libswresample.so.6")
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
        
        # Test if we can load libavutil directly
        message(STATUS "Testing direct access to libavutil.so.60:")
        execute_process(
            COMMAND ${LDD_PROGRAM} ${TARGET_DIR}/libavutil.so.60
            OUTPUT_VARIABLE LDD_AVUTIL_OUTPUT
            ERROR_VARIABLE LDD_AVUTIL_ERROR
            RESULT_VARIABLE LDD_AVUTIL_RESULT
        )
        if(LDD_AVUTIL_RESULT EQUAL 0)
            message(STATUS "libavutil.so.60 dependencies:")
            string(REPLACE "\n" "\n  " LDD_AVUTIL_FORMATTED "  ${LDD_AVUTIL_OUTPUT}")
            message(STATUS "${LDD_AVUTIL_FORMATTED}")
        else()
            message(STATUS "ldd on libavutil.so.60 failed: ${LDD_AVUTIL_ERROR}")
        endif()
    else()
        message(STATUS "ldd not found or libswresample.so.6 missing, cannot check dependencies")
    endif()
    
    # Fix RPATH on all copied libraries to ensure they can find each other
    find_program(PATCHELF_PROGRAM patchelf)
    find_program(CHRPATH_PROGRAM chrpath)
    
    if(PATCHELF_PROGRAM OR CHRPATH_PROGRAM)
        message(STATUS "Fixing RPATH on copied libraries...")
        file(GLOB COPIED_LIBS "${TARGET_DIR}/lib*.so*")
        foreach(LIB_FILE ${COPIED_LIBS})
            get_filename_component(LIB_NAME ${LIB_FILE} NAME)
            
            # Skip symlinks for RPATH modification
            if(NOT IS_SYMLINK ${LIB_FILE})
                message(STATUS "  Setting RPATH on ${LIB_NAME}")
                
                if(PATCHELF_PROGRAM)
                    execute_process(
                        COMMAND ${PATCHELF_PROGRAM} --set-rpath '$ORIGIN:$ORIGIN/..' ${LIB_FILE}
                        RESULT_VARIABLE RPATH_RESULT
                        OUTPUT_QUIET
                        ERROR_QUIET
                    )
                elseif(CHRPATH_PROGRAM)
                    execute_process(
                        COMMAND ${CHRPATH_PROGRAM} -r '$ORIGIN:$ORIGIN/..' ${LIB_FILE}
                        RESULT_VARIABLE RPATH_RESULT
                        OUTPUT_QUIET
                        ERROR_QUIET
                    )
                endif()
                
                if(NOT RPATH_RESULT EQUAL 0)
                    message(STATUS "    Warning: Failed to set RPATH on ${LIB_NAME}")
                else()
                    message(STATUS "    ✓ RPATH set successfully on ${LIB_NAME}")
                endif()
            endif()
        endforeach()
    else()
        message(STATUS "Neither patchelf nor chrpath found")
        message(STATUS "Install patchelf or chrpath for better library compatibility")
    endif()
    
    message(STATUS "Copied ${COPIED_COUNT} FFmpeg major version .so files and created symlinks")
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()