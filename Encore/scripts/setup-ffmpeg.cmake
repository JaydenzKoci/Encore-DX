# FFmpeg Setup Script for Cross-Platform Support
# This script handles downloading and setting up FFmpeg prebuilt binaries
# for Windows (x86/x64), Linux (x86/x64), and macOS (x64)

function(setup_ffmpeg)
    set(FFMPEG_VERSION "6.0")

    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # Windows x64: Download from BtbN
            set(FFMPEG_BASE_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest")
            set(FFMPEG_PACKAGE "ffmpeg-master-latest-win64-gpl-shared")
            set(PLATFORM_NAME "Windows x64")
            set(ARCHIVE_EXT "zip")
            set(USE_DOWNLOAD TRUE)
        else()
            # Windows x86: Download from yt-dlp (revert back to downloads)
            set(FFMPEG_BASE_URL "https://github.com/yt-dlp/FFmpeg-Builds/releases/download/latest")
            set(FFMPEG_PACKAGE "ffmpeg-master-latest-win32-gpl-shared")
            set(PLATFORM_NAME "Windows x86")
            set(ARCHIVE_EXT "zip")
            set(USE_DOWNLOAD TRUE)
        endif()
        set(LIB_PREFIX "")
        set(LIB_SUFFIX ".lib")
        set(SHARED_SUFFIX ".dll")
    elseif(APPLE)
        # macOS: Build FFmpeg from source with shared libraries
        set(PLATFORM_NAME "macOS")
        set(LIB_PREFIX "lib")
        set(LIB_SUFFIX ".dylib")
        set(SHARED_SUFFIX ".dylib")
        set(USE_DOWNLOAD FALSE)
        set(BUILD_FROM_SOURCE TRUE)
    elseif(UNIX)
        # Linux: Download from BtbN
        set(FFMPEG_BASE_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(FFMPEG_PACKAGE "ffmpeg-master-latest-linux64-gpl-shared")
            set(PLATFORM_NAME "Linux x64")
        else()
            set(FFMPEG_PACKAGE "ffmpeg-master-latest-linux32-gpl-shared")
            set(PLATFORM_NAME "Linux x86")
        endif()
        set(ARCHIVE_EXT "tar.xz")
        set(LIB_PREFIX "lib")
        set(LIB_SUFFIX ".so")
        set(SHARED_SUFFIX ".so")
        set(USE_DOWNLOAD TRUE)
    endif()
    
    message(STATUS "Setting up FFmpeg for ${PLATFORM_NAME}")

    if(USE_DOWNLOAD)
        # Download FFmpeg prebuilt binaries
        set(FFMPEG_URL "${FFMPEG_BASE_URL}/${FFMPEG_PACKAGE}.${ARCHIVE_EXT}")
        set(FFMPEG_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg" PARENT_SCOPE)

        include(FetchContent)
        FetchContent_Declare(
            ffmpeg_prebuilt
            URL ${FFMPEG_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
        )
        
        FetchContent_GetProperties(ffmpeg_prebuilt)
        if(NOT ffmpeg_prebuilt_POPULATED)
            message(STATUS "Downloading FFmpeg prebuilt binaries...")
            FetchContent_MakeAvailable(ffmpeg_prebuilt)

            file(COPY ${ffmpeg_prebuilt_SOURCE_DIR}/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg)
            message(STATUS "FFmpeg setup complete for ${PLATFORM_NAME}")
        endif()

        set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/include" PARENT_SCOPE)
        set(FFMPEG_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib" PARENT_SCOPE)
        set(FFMPEG_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/bin" PARENT_SCOPE)
    elseif(BUILD_FROM_SOURCE)
        # Build FFmpeg from source for macOS
        message(STATUS "Building FFmpeg from source for ${PLATFORM_NAME}")
        build_ffmpeg_macos_from_source()
    else()
        # Use local FFmpeg from lib folder
        set(FFMPEG_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${FFMPEG_FOLDER}")
        
        # Check if local FFmpeg exists
        if(NOT EXISTS "${FFMPEG_LOCAL_DIR}")
            message(FATAL_ERROR "FFmpeg not found at ${FFMPEG_LOCAL_DIR}. Please ensure FFmpeg is installed in the lib folder with the expected structure.")
        endif()
        
        message(STATUS "Using local FFmpeg from: ${FFMPEG_LOCAL_DIR}")
        
        # Set the output variables to point to local FFmpeg
        set(FFMPEG_INCLUDE_DIR "${FFMPEG_LOCAL_DIR}/include" PARENT_SCOPE)
        set(FFMPEG_LIB_DIR "${FFMPEG_LOCAL_DIR}/lib" PARENT_SCOPE)
        set(FFMPEG_BIN_DIR "${FFMPEG_LOCAL_DIR}/bin" PARENT_SCOPE)
    endif()
    set(FFMPEG_LIB_PREFIX "${LIB_PREFIX}" PARENT_SCOPE)
    set(FFMPEG_LIB_SUFFIX "${LIB_SUFFIX}" PARENT_SCOPE)
    set(FFMPEG_SHARED_SUFFIX "${SHARED_SUFFIX}" PARENT_SCOPE)
    
endfunction()

function(find_ffmpeg_libraries)
    if(APPLE)
        # For macOS, search in the specified paths first, then allow default paths
        find_library(AVFORMAT_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avformat${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(AVCODEC_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avcodec${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(AVUTIL_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avutil${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(SWSCALE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}swscale${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(SWRESAMPLE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}swresample${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(AVFILTER_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avfilter${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(AVDEVICE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avdevice${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
        find_library(POSTPROC_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}postproc${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
            PATH_SUFFIXES lib
        )
    else()
        find_library(AVFORMAT_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avformat${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(AVCODEC_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avcodec${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(AVUTIL_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avutil${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(SWSCALE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}swscale${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(SWRESAMPLE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}swresample${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(AVFILTER_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avfilter${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(AVDEVICE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avdevice${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
        find_library(POSTPROC_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}postproc${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR} 
            NO_DEFAULT_PATH
        )
    endif()

    # Debug output for macOS
    if(APPLE)
        message(STATUS "FFmpeg library search results:")
        message(STATUS "  AVFORMAT_LIB: ${AVFORMAT_LIB}")
        message(STATUS "  AVCODEC_LIB: ${AVCODEC_LIB}")
        message(STATUS "  AVUTIL_LIB: ${AVUTIL_LIB}")
        message(STATUS "  SWSCALE_LIB: ${SWSCALE_LIB}")
        message(STATUS "  FFMPEG_LIB_DIR: ${FFMPEG_LIB_DIR}")
        message(STATUS "  FFMPEG_LIB_PREFIX: ${FFMPEG_LIB_PREFIX}")
        message(STATUS "  FFMPEG_LIB_SUFFIX: ${FFMPEG_LIB_SUFFIX}")
        
        # Debug: List what's actually in the lib directory
        if(EXISTS ${FFMPEG_LIB_DIR})
            file(GLOB LIB_FILES "${FFMPEG_LIB_DIR}/*")
            message(STATUS "Files in FFMPEG_LIB_DIR:")
            foreach(FILE ${LIB_FILES})
                message(STATUS "    ${FILE}")
            endforeach()
        else()
            message(STATUS "FFMPEG_LIB_DIR does not exist: ${FFMPEG_LIB_DIR}")
        endif()
    endif()

    if(NOT AVFORMAT_LIB OR NOT AVCODEC_LIB OR NOT AVUTIL_LIB OR NOT SWSCALE_LIB)
        if(APPLE)
            # Try using pkg-config as fallback for macOS
            find_package(PkgConfig QUIET)
            if(PKG_CONFIG_FOUND)
                message(STATUS "Trying pkg-config as fallback...")
                pkg_check_modules(PC_AVFORMAT QUIET libavformat)
                pkg_check_modules(PC_AVCODEC QUIET libavcodec)
                pkg_check_modules(PC_AVUTIL QUIET libavutil)
                pkg_check_modules(PC_SWSCALE QUIET libswscale)
                pkg_check_modules(PC_SWRESAMPLE QUIET libswresample)
                pkg_check_modules(PC_AVFILTER QUIET libavfilter)
                pkg_check_modules(PC_AVDEVICE QUIET libavdevice)
                
                if(PC_AVFORMAT_FOUND AND PC_AVCODEC_FOUND AND PC_AVUTIL_FOUND AND PC_SWSCALE_FOUND)
                    set(AVFORMAT_LIB ${PC_AVFORMAT_LIBRARIES})
                    set(AVCODEC_LIB ${PC_AVCODEC_LIBRARIES})
                    set(AVUTIL_LIB ${PC_AVUTIL_LIBRARIES})
                    set(SWSCALE_LIB ${PC_SWSCALE_LIBRARIES})
                    if(PC_SWRESAMPLE_FOUND)
                        set(SWRESAMPLE_LIB ${PC_SWRESAMPLE_LIBRARIES})
                    endif()
                    if(PC_AVFILTER_FOUND)
                        set(AVFILTER_LIB ${PC_AVFILTER_LIBRARIES})
                    endif()
                    if(PC_AVDEVICE_FOUND)
                        set(AVDEVICE_LIB ${PC_AVDEVICE_LIBRARIES})
                    endif()
                    message(STATUS "Found FFmpeg libraries via pkg-config")
                endif()
            endif()
        endif()
        
        if(NOT AVFORMAT_LIB OR NOT AVCODEC_LIB OR NOT AVUTIL_LIB OR NOT SWSCALE_LIB)
            message(STATUS "Final FFmpeg library search results:")
            message(STATUS "  AVFORMAT_LIB: ${AVFORMAT_LIB}")
            message(STATUS "  AVCODEC_LIB: ${AVCODEC_LIB}")
            message(STATUS "  AVUTIL_LIB: ${AVUTIL_LIB}")
            message(STATUS "  SWSCALE_LIB: ${SWSCALE_LIB}")
            message(STATUS "  FFMPEG_LIB_DIR: ${FFMPEG_LIB_DIR}")
            message(STATUS "  FFMPEG_LIB_PREFIX: ${FFMPEG_LIB_PREFIX}")
            message(STATUS "  FFMPEG_LIB_SUFFIX: ${FFMPEG_LIB_SUFFIX}")
            message(FATAL_ERROR "Required FFmpeg libraries not found!")
        endif()
    endif()

    set(FFMPEG_LIBRARIES_LIST)

    if(AVDEVICE_LIB)
        list(APPEND FFMPEG_LIBRARIES_LIST ${AVDEVICE_LIB})
    endif()
    if(AVFILTER_LIB)
        list(APPEND FFMPEG_LIBRARIES_LIST ${AVFILTER_LIB})
    endif()
    list(APPEND FFMPEG_LIBRARIES_LIST ${AVFORMAT_LIB})
    list(APPEND FFMPEG_LIBRARIES_LIST ${AVCODEC_LIB})
    if(POSTPROC_LIB)
        list(APPEND FFMPEG_LIBRARIES_LIST ${POSTPROC_LIB})
    endif()
    if(SWRESAMPLE_LIB)
        list(APPEND FFMPEG_LIBRARIES_LIST ${SWRESAMPLE_LIB})
    endif()
    list(APPEND FFMPEG_LIBRARIES_LIST ${SWSCALE_LIB})
    list(APPEND FFMPEG_LIBRARIES_LIST ${AVUTIL_LIB})

    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES_LIST} PARENT_SCOPE)
    
    message(STATUS "Found FFmpeg libraries:")
    message(STATUS "  AVFORMAT: ${AVFORMAT_LIB}")
    message(STATUS "  AVCODEC: ${AVCODEC_LIB}")
    message(STATUS "  AVUTIL: ${AVUTIL_LIB}")
    message(STATUS "  SWSCALE: ${SWSCALE_LIB}")
    if(SWRESAMPLE_LIB)
        message(STATUS "  SWRESAMPLE: ${SWRESAMPLE_LIB}")
    endif()
    if(AVFILTER_LIB)
        message(STATUS "  AVFILTER: ${AVFILTER_LIB}")
    endif()
    if(AVDEVICE_LIB)
        message(STATUS "  AVDEVICE: ${AVDEVICE_LIB}")
    endif()
    if(POSTPROC_LIB)
        message(STATUS "  POSTPROC: ${POSTPROC_LIB}")
    endif()
endfunction()

function(copy_ffmpeg_binaries TARGET_DIR)
    if(WIN32)
        file(GLOB FFMPEG_BINARIES "${FFMPEG_BIN_DIR}/*${FFMPEG_SHARED_SUFFIX}")
        if(FFMPEG_BINARIES)
            file(COPY ${FFMPEG_BINARIES} DESTINATION ${TARGET_DIR})
            list(LENGTH FFMPEG_BINARIES DLL_COUNT)
            message(STATUS "Copied ${DLL_COUNT} FFmpeg DLLs to ${TARGET_DIR}")

            foreach(DLL ${FFMPEG_BINARIES})
                get_filename_component(DLL_NAME ${DLL} NAME)
                message(STATUS "  - ${DLL_NAME}")
            endforeach()
        else()
            message(WARNING "No FFmpeg DLLs found in ${FFMPEG_BIN_DIR}")
        endif()
    elseif(UNIX)
        file(GLOB FFMPEG_BINARIES "${FFMPEG_LIB_DIR}/*${FFMPEG_SHARED_SUFFIX}*")
        if(FFMPEG_BINARIES)
            file(COPY ${FFMPEG_BINARIES} DESTINATION ${TARGET_DIR})
            message(STATUS "Copied FFmpeg shared libraries to ${TARGET_DIR}")
        endif()
    endif()
endfunction()

function(setup_all_post_build TARGET_NAME TARGET_DIR)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying assets and dependencies..."
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_DIR}"
        
        # Copy assets (Songs and Assets folders)
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Songs"
            "${TARGET_DIR}/Songs"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "${TARGET_DIR}/Assets"
    )
    
    if(WIN32)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            # Copy Discord RPC, BASS, and BASSOPUS DLLs based on architecture
            COMMAND ${CMAKE_COMMAND} -E echo "Copying Windows-specific libraries..."
        )
        
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            # x86 libraries
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord-rpc/windows/x86/discord-rpc.dll"
                    "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/windows/x86/bass.dll"
                    "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/windows/x86/bassopus.dll"
                    "${TARGET_DIR}/"
            )
        else()
            # x64 libraries
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord-rpc/windows/x64/discord-rpc.dll"
                    "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/windows/x64/bass.dll"
                    "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/windows/x64/bassopus.dll"
                    "${TARGET_DIR}/"
            )
        endif()
        
        # Copy FFmpeg DLLs from local lib folder
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${FFMPEG_BIN_DIR}"
                "${TARGET_DIR}"
            
            COMMENT "Copying Windows dependencies to output directory"
        )
    elseif(UNIX AND NOT APPLE)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            # Copy Discord RPC, BASS, and BASSOPUS libraries
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord-rpc/linux/x64/libdiscord-rpc.so"
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/linux/x86_64/libbass.so"
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/linux/x86_64/libbassopus.so"
                "${TARGET_DIR}/"
            
            # Copy only essential FFmpeg .so files (no executables or extra files)
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FFMPEG_LIB_DIR}/libavformat.so"
                "${FFMPEG_LIB_DIR}/libavcodec.so"
                "${FFMPEG_LIB_DIR}/libavutil.so"
                "${FFMPEG_LIB_DIR}/libswscale.so"
                "${TARGET_DIR}/"
            # Copy optional FFmpeg libraries if they exist
            COMMAND ${CMAKE_COMMAND} -E echo "Copying optional FFmpeg libraries..."
            COMMAND bash -c "[ -f '${FFMPEG_LIB_DIR}/libswresample.so' ] && ${CMAKE_COMMAND} -E copy_if_different '${FFMPEG_LIB_DIR}/libswresample.so' '${TARGET_DIR}/' || true"
            COMMAND bash -c "[ -f '${FFMPEG_LIB_DIR}/libavfilter.so' ] && ${CMAKE_COMMAND} -E copy_if_different '${FFMPEG_LIB_DIR}/libavfilter.so' '${TARGET_DIR}/' || true"
            COMMAND bash -c "[ -f '${FFMPEG_LIB_DIR}/libavdevice.so' ] && ${CMAKE_COMMAND} -E copy_if_different '${FFMPEG_LIB_DIR}/libavdevice.so' '${TARGET_DIR}/' || true"
            
            COMMENT "Copying Linux dependencies to output directory"
        )
    elseif(APPLE)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            # Copy Discord RPC, BASS, and BASSOPUS libraries
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord-rpc/macos/libdiscord-rpc.dylib"
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/macos/libbass.dylib"
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/bass/macos/libbassopus.dylib"
                "${TARGET_DIR}/"
            
            COMMENT "Copying macOS dependencies to output directory"
        )
        
        if(FFMPEG_LIB_DIR)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying FFmpeg libraries from ${FFMPEG_LIB_DIR}..."
                COMMAND bash -c "for lib in libavformat libavcodec libavutil libswscale libswresample libavfilter libavdevice; do if [ -f '${FFMPEG_LIB_DIR}/$lib${FFMPEG_SHARED_SUFFIX}' ]; then ${CMAKE_COMMAND} -E copy_if_different '${FFMPEG_LIB_DIR}/$lib${FFMPEG_SHARED_SUFFIX}' '${TARGET_DIR}/'; fi; done"
                COMMENT "Copying FFmpeg dylibs to output directory"
            )
        endif()
    endif()
endfunction()

# Function to download custom prebuilt FFmpeg packages
function(download_custom_ffmpeg_prebuilt PLATFORM)
    message(STATUS "Downloading custom prebuilt FFmpeg for ${PLATFORM}...")
    
    # Set URLs for prebuilt packages
    if(PLATFORM STREQUAL "windows-x86")
        # Use Zeranoe FFmpeg builds or similar
        set(FFMPEG_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win32-gpl-shared.zip")
        set(ARCHIVE_EXT "zip")
    elseif(PLATFORM STREQUAL "macos")
        # Use your own prebuilt package or build from Homebrew
        set(FFMPEG_URL "https://github.com/YOUR_USERNAME/YOUR_REPO/releases/download/ffmpeg-prebuilt/ffmpeg-macos.tar.gz")
        set(ARCHIVE_EXT "tar.gz")
    else()
        message(FATAL_ERROR "Unknown platform for custom prebuilt FFmpeg: ${PLATFORM}")
    endif()
    
    set(FFMPEG_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")
    
    include(FetchContent)
    FetchContent_Declare(
        ffmpeg_custom_prebuilt
        URL ${FFMPEG_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
    )
    
    FetchContent_GetProperties(ffmpeg_custom_prebuilt)
    if(NOT ffmpeg_custom_prebuilt_POPULATED)
        message(STATUS "Downloading custom prebuilt FFmpeg...")
        FetchContent_MakeAvailable(ffmpeg_custom_prebuilt)
        
        # Copy to standard location
        file(COPY ${ffmpeg_custom_prebuilt_SOURCE_DIR}/ DESTINATION ${FFMPEG_INSTALL_DIR})
        message(STATUS "Custom prebuilt FFmpeg setup complete for ${PLATFORM}")
    endif()
    
    # Set the output variables
    set(FFMPEG_INCLUDE_DIR "${FFMPEG_INSTALL_DIR}/include" PARENT_SCOPE)
    set(FFMPEG_LIB_DIR "${FFMPEG_INSTALL_DIR}/lib" PARENT_SCOPE)
    set(FFMPEG_BIN_DIR "${FFMPEG_INSTALL_DIR}/bin" PARENT_SCOPE)
endfunction()

# Function to build FFmpeg using vcpkg for Windows x86 (fallback)
function(build_ffmpeg_from_source)
    message(STATUS "Setting up FFmpeg build via vcpkg for Windows x86...")
    
    set(FFMPEG_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")
    
    # Check if vcpkg is available
    find_program(VCPKG_EXECUTABLE vcpkg HINTS 
        "C:/vcpkg" 
        "C:/tools/vcpkg" 
        "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg"
        ENV VCPKG_ROOT
    )
    
    if(NOT VCPKG_EXECUTABLE)
        message(STATUS "vcpkg not found, downloading and setting up...")
        
        # Download and setup vcpkg
        include(FetchContent)
        FetchContent_Declare(
            vcpkg
            GIT_REPOSITORY https://github.com/Microsoft/vcpkg.git
            GIT_TAG master
        )
        
        FetchContent_GetProperties(vcpkg)
        if(NOT vcpkg_POPULATED)
            FetchContent_MakeAvailable(vcpkg)
            
            # Bootstrap vcpkg
            execute_process(
                COMMAND ${vcpkg_SOURCE_DIR}/bootstrap-vcpkg.bat
                WORKING_DIRECTORY ${vcpkg_SOURCE_DIR}
                RESULT_VARIABLE BOOTSTRAP_RESULT
            )
            
            if(NOT BOOTSTRAP_RESULT EQUAL 0)
                message(FATAL_ERROR "vcpkg bootstrap failed")
            endif()
            
            set(VCPKG_EXECUTABLE "${vcpkg_SOURCE_DIR}/vcpkg.exe")
        endif()
    endif()
    
    message(STATUS "Using vcpkg at: ${VCPKG_EXECUTABLE}")
    
    # First, check what FFmpeg features are available
    message(STATUS "Checking available FFmpeg features...")
    execute_process(
        COMMAND ${VCPKG_EXECUTABLE} search ffmpeg
        RESULT_VARIABLE SEARCH_RESULT
        OUTPUT_VARIABLE SEARCH_OUTPUT
        ERROR_VARIABLE SEARCH_ERROR
    )
    
    message(STATUS "Available FFmpeg packages: ${SEARCH_OUTPUT}")
    
    # Install FFmpeg for x86-windows
    message(STATUS "Installing FFmpeg via vcpkg (this may take a while)...")
    execute_process(
        COMMAND ${VCPKG_EXECUTABLE} install ffmpeg:x86-windows
        RESULT_VARIABLE VCPKG_RESULT
        OUTPUT_VARIABLE VCPKG_OUTPUT
        ERROR_VARIABLE VCPKG_ERROR
    )
    
    if(NOT VCPKG_RESULT EQUAL 0)
        message(STATUS "vcpkg output: ${VCPKG_OUTPUT}")
        message(STATUS "vcpkg error: ${VCPKG_ERROR}")
        
        # Try to update vcpkg and retry
        message(STATUS "Trying to update vcpkg and retry...")
        execute_process(
            COMMAND ${VCPKG_EXECUTABLE} update
            RESULT_VARIABLE UPDATE_RESULT
        )
        
        # Retry installation
        execute_process(
            COMMAND ${VCPKG_EXECUTABLE} install ffmpeg:x86-windows
            RESULT_VARIABLE RETRY_RESULT
            OUTPUT_VARIABLE RETRY_OUTPUT
            ERROR_VARIABLE RETRY_ERROR
        )
        
        if(NOT RETRY_RESULT EQUAL 0)
            message(STATUS "Retry output: ${RETRY_OUTPUT}")
            message(STATUS "Retry error: ${RETRY_ERROR}")
            message(FATAL_ERROR "vcpkg FFmpeg installation failed after retry")
        endif()
    endif()
    
    # Find vcpkg installation directory
    get_filename_component(VCPKG_ROOT ${VCPKG_EXECUTABLE} DIRECTORY)
    set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x86-windows")
    
    # Set the output variables
    set(FFMPEG_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include" PARENT_SCOPE)
    set(FFMPEG_LIB_DIR "${VCPKG_INSTALLED_DIR}/lib" PARENT_SCOPE)
    set(FFMPEG_BIN_DIR "${VCPKG_INSTALLED_DIR}/bin" PARENT_SCOPE)
    
    message(STATUS "FFmpeg build complete for Windows x86 via vcpkg")
    message(STATUS "  Include dir: ${VCPKG_INSTALLED_DIR}/include")
    message(STATUS "  Library dir: ${VCPKG_INSTALLED_DIR}/lib")
    message(STATUS "  Binary dir: ${VCPKG_INSTALLED_DIR}/bin")
endfunction()

# Function to build FFmpeg from source for macOS with shared libraries
function(build_ffmpeg_macos_from_source)
    message(STATUS "Setting up FFmpeg source build for macOS...")
    
    set(FFMPEG_SOURCE_URL "https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n6.1.1.tar.gz")
    set(FFMPEG_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")
    
    include(FetchContent)
    FetchContent_Declare(
        ffmpeg_source_macos
        URL ${FFMPEG_SOURCE_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
    )
    
    FetchContent_GetProperties(ffmpeg_source_macos)
    if(NOT ffmpeg_source_macos_POPULATED)
        message(STATUS "Downloading FFmpeg source code...")
        FetchContent_MakeAvailable(ffmpeg_source_macos)
        
        # Configure FFmpeg build for macOS with shared libraries
        message(STATUS "Configuring FFmpeg build for macOS...")
        
        # Configure FFmpeg with minimal features and shared libraries
        execute_process(
            COMMAND ./configure
                --prefix=${FFMPEG_INSTALL_PREFIX}
                --enable-shared
                --disable-static
                --disable-programs
                --disable-doc
                --disable-network
                --disable-devices
                --disable-filters
                --enable-filter=scale
                --disable-encoders
                --disable-muxers
                --disable-protocols
                --enable-protocol=file
                --disable-bsfs
                --disable-indevs
                --disable-outdevs
                --disable-debug
                --enable-optimizations
            WORKING_DIRECTORY ${ffmpeg_source_macos_SOURCE_DIR}
            RESULT_VARIABLE CONFIGURE_RESULT
            OUTPUT_VARIABLE CONFIGURE_OUTPUT
            ERROR_VARIABLE CONFIGURE_ERROR
        )
        
        if(NOT CONFIGURE_RESULT EQUAL 0)
            message(STATUS "FFmpeg configure output: ${CONFIGURE_OUTPUT}")
            message(STATUS "FFmpeg configure error: ${CONFIGURE_ERROR}")
            message(FATAL_ERROR "FFmpeg configure failed")
        endif()
        
        # Build FFmpeg
        message(STATUS "Building FFmpeg (this may take a while)...")
        execute_process(
            COMMAND make -j4
            WORKING_DIRECTORY ${ffmpeg_source_macos_SOURCE_DIR}
            RESULT_VARIABLE BUILD_RESULT
            OUTPUT_VARIABLE BUILD_OUTPUT
            ERROR_VARIABLE BUILD_ERROR
        )
        
        if(NOT BUILD_RESULT EQUAL 0)
            message(STATUS "FFmpeg build output: ${BUILD_OUTPUT}")
            message(STATUS "FFmpeg build error: ${BUILD_ERROR}")
            message(FATAL_ERROR "FFmpeg build failed")
        endif()
        
        # Install FFmpeg
        message(STATUS "Installing FFmpeg...")
        execute_process(
            COMMAND make install
            WORKING_DIRECTORY ${ffmpeg_source_macos_SOURCE_DIR}
            RESULT_VARIABLE INSTALL_RESULT
        )
        
        if(NOT INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "FFmpeg install failed")
        endif()
        
        message(STATUS "FFmpeg build complete for macOS")
    endif()
    
    # Set the output variables
    set(FFMPEG_INCLUDE_DIR "${FFMPEG_INSTALL_PREFIX}/include" PARENT_SCOPE)
    set(FFMPEG_LIB_DIR "${FFMPEG_INSTALL_PREFIX}/lib" PARENT_SCOPE)
    set(FFMPEG_BIN_DIR "${FFMPEG_INSTALL_PREFIX}/bin" PARENT_SCOPE)
endfunction()

# Legacy function for backward compatibility
function(setup_ffmpeg_post_build TARGET_NAME TARGET_DIR)
    setup_all_post_build(${TARGET_NAME} ${TARGET_DIR})
endfunction()