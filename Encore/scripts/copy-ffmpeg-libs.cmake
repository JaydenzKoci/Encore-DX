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
    
    # Copy everything from local lib folder instead of FFmpeg download
    set(LOCAL_FFMPEG_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/linux/lib")
    message(STATUS "Using local FFmpeg libraries from: ${LOCAL_FFMPEG_LIB_DIR}")
    
    if(EXISTS ${LOCAL_FFMPEG_LIB_DIR})
        file(GLOB LOCAL_SO_FILES "${LOCAL_FFMPEG_LIB_DIR}/*.so*")
        message(STATUS "Local FFmpeg files found:")
        foreach(FILE ${LOCAL_SO_FILES})
            get_filename_component(FILENAME ${FILE} NAME)
            message(STATUS "  - ${FILENAME}")
        endforeach()
        
        set(COPIED_COUNT 0)
        foreach(LIB_FILE ${LOCAL_SO_FILES})
            get_filename_component(LIB_NAME ${LIB_FILE} NAME)
            
            # Copy all .so files from local directory
            if(LIB_NAME MATCHES "^lib.*\\.so.*$")
                message(STATUS "  Copying local file: ${LIB_NAME}")
                configure_file(${LIB_FILE} ${TARGET_DIR}/${LIB_NAME} COPYONLY)
                
                if(EXISTS "${TARGET_DIR}/${LIB_NAME}")
                    message(STATUS "    ✓ Successfully copied to ${TARGET_DIR}/${LIB_NAME}")
                    math(EXPR COPIED_COUNT "${COPIED_COUNT} + 1")
                else()
                    message(STATUS "    ✗ Failed to copy ${LIB_NAME}")
                endif()
            else()
                message(STATUS "  Skipping: ${LIB_NAME} (not a library file)")
            endif()
        endforeach()
    else()
        message(FATAL_ERROR "Local FFmpeg lib directory not found: ${LOCAL_FFMPEG_LIB_DIR}")
    endif()
    
    # Create base symlinks for major version libraries
    # This ensures libname.so -> libname.so.X for proper dynamic linking
    set(FFMPEG_LIBS_TO_LINK
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
    
    # Verify critical library types exist (any version)
    message(STATUS "Verifying critical library types exist:")
    set(CRITICAL_LIB_TYPES "libavutil" "libswresample" "libavcodec" "libavformat" "libswscale")
    foreach(LIB_TYPE ${CRITICAL_LIB_TYPES})
        file(GLOB LIB_FILES "${TARGET_DIR}/${LIB_TYPE}.so*")
        if(LIB_FILES)
            list(GET LIB_FILES 0 FIRST_FILE)
            get_filename_component(FILENAME ${FIRST_FILE} NAME)
            message(STATUS "  ✓ ${LIB_TYPE} found: ${FILENAME}")
        else()
            message(STATUS "  ✗ ${LIB_TYPE} missing!")
        endif()
    endforeach()
    

    
    # Fix RPATH and dependencies on all copied libraries
    find_program(PATCHELF_PROGRAM patchelf)
    find_program(CHRPATH_PROGRAM chrpath)
    
    if(PATCHELF_PROGRAM)
        message(STATUS "Fixing RPATH and dependencies on copied libraries...")
        file(GLOB COPIED_LIBS "${TARGET_DIR}/lib*.so*")
        foreach(LIB_FILE ${COPIED_LIBS})
            get_filename_component(LIB_NAME ${LIB_FILE} NAME)
            
            # Skip symlinks for RPATH modification
            if(NOT IS_SYMLINK ${LIB_FILE})
                message(STATUS "  Processing ${LIB_NAME}")
                
                # Set RPATH to look in current directory first
                execute_process(
                    COMMAND ${PATCHELF_PROGRAM} --set-rpath '$ORIGIN' ${LIB_FILE}
                    RESULT_VARIABLE RPATH_RESULT
                    OUTPUT_QUIET
                    ERROR_QUIET
                )
                
                if(RPATH_RESULT EQUAL 0)
                    message(STATUS "    ✓ RPATH set successfully")
                else()
                    message(STATUS "    Warning: Failed to set RPATH")
                endif()
                


            endif()
        endforeach()
    elseif(CHRPATH_PROGRAM)
        message(STATUS "Using chrpath to fix RPATH...")
        file(GLOB COPIED_LIBS "${TARGET_DIR}/lib*.so.[0-9]*")
        foreach(LIB_FILE ${COPIED_LIBS})
            get_filename_component(LIB_NAME ${LIB_FILE} NAME)
            message(STATUS "  Setting RPATH on ${LIB_NAME}")
            
            execute_process(
                COMMAND ${CHRPATH_PROGRAM} -r '$ORIGIN' ${LIB_FILE}
                RESULT_VARIABLE RPATH_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            
            if(RPATH_RESULT EQUAL 0)
                message(STATUS "    ✓ RPATH set successfully")
            else()
                message(STATUS "    Warning: Failed to set RPATH")
            endif()
        endforeach()
    else()
        message(STATUS "Neither patchelf nor chrpath found")
        message(STATUS "Install patchelf for better library compatibility")
        
        # As a last resort, create an LD_LIBRARY_PATH wrapper script
        message(STATUS "Creating LD_LIBRARY_PATH wrapper script...")
        set(WRAPPER_SCRIPT "${TARGET_DIR}/run_encore.sh")
        file(WRITE ${WRAPPER_SCRIPT} "#!/bin/bash\n")
        file(APPEND ${WRAPPER_SCRIPT} "export LD_LIBRARY_PATH=\"$(dirname \"$0\"):$LD_LIBRARY_PATH\"\n")
        file(APPEND ${WRAPPER_SCRIPT} "exec \"$(dirname \"$0\")/Encore\" \"$@\"\n")
        
        execute_process(
            COMMAND chmod +x ${WRAPPER_SCRIPT}
            RESULT_VARIABLE CHMOD_RESULT
        )
        
        if(CHMOD_RESULT EQUAL 0)
            message(STATUS "✓ Created wrapper script: run_encore.sh")
            message(STATUS "  Use ./run_encore.sh instead of ./Encore to run the application")
        endif()
    endif()
    
    # Create comprehensive symlinks to handle all possible dependency naming
    message(STATUS "Creating comprehensive symlinks for dependency resolution...")
    
    # Create additional symlinks that might be needed (excluding swresample)
    # Define as pairs: symlink_name -> target_name
    set(SYMLINK_NAMES "libavutil.so" "libswresample.so" "libavcodec.so" "libavformat.so" "libswscale.so" "libavfilter.so" "libavdevice.so")
    set(TARGET_NAMES "libavutil.so.60" "libswresample.so.4" "libavcodec.so.62" "libavformat.so.62" "libswscale.so.9" "libavfilter.so.11" "libavdevice.so.62")
    
    list(LENGTH SYMLINK_NAMES NUM_MAPPINGS)
    math(EXPR LAST_INDEX "${NUM_MAPPINGS} - 1")
    
    foreach(INDEX RANGE ${LAST_INDEX})
        list(GET SYMLINK_NAMES ${INDEX} SYMLINK_NAME)
        list(GET TARGET_NAMES ${INDEX} TARGET_NAME)
        
        set(SYMLINK_PATH "${TARGET_DIR}/${SYMLINK_NAME}")
        set(TARGET_PATH "${TARGET_DIR}/${TARGET_NAME}")
        
        if(EXISTS ${TARGET_PATH})
            if(NOT EXISTS ${SYMLINK_PATH})
                message(STATUS "  Creating symlink: ${SYMLINK_NAME} -> ${TARGET_NAME}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E create_symlink ${TARGET_NAME} ${SYMLINK_NAME}
                    WORKING_DIRECTORY ${TARGET_DIR}
                    RESULT_VARIABLE SYMLINK_RESULT
                )
                if(SYMLINK_RESULT EQUAL 0)
                    message(STATUS "    ✓ Symlink created successfully")
                else()
                    message(STATUS "    ✗ Failed to create symlink")
                endif()
            else()
                message(STATUS "  Symlink already exists: ${SYMLINK_NAME}")
            endif()
        else()
            message(STATUS "  Target ${TARGET_NAME} not found, skipping symlink")
        endif()
    endforeach()
    
    # Final test - check that core libraries are properly linked
    find_program(LDD_PROGRAM ldd)
    if(LDD_PROGRAM AND EXISTS "${TARGET_DIR}/libavcodec.so.62")
        message(STATUS "Final dependency check for libavcodec.so.62:")
        execute_process(
            COMMAND env LD_LIBRARY_PATH=${TARGET_DIR} ${LDD_PROGRAM} ${TARGET_DIR}/libavcodec.so.62
            OUTPUT_VARIABLE FINAL_LDD_OUTPUT
            ERROR_VARIABLE FINAL_LDD_ERROR
            RESULT_VARIABLE FINAL_LDD_RESULT
        )
        if(FINAL_LDD_RESULT EQUAL 0)
            message(STATUS "Dependencies with LD_LIBRARY_PATH set:")
            string(REPLACE "\n" "\n  " FINAL_LDD_FORMATTED "  ${FINAL_LDD_OUTPUT}")
            message(STATUS "${FINAL_LDD_FORMATTED}")
        else()
            message(STATUS "ldd failed: ${FINAL_LDD_ERROR}")
        endif()
    endif()
    

    
    message(STATUS "Copied ${COPIED_COUNT} FFmpeg major version .so files and created symlinks")
else()
    message(STATUS "FFMPEG_LIB_DIR or TARGET_DIR not set")
endif()