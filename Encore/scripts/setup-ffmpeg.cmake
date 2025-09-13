# FFmpeg Setup Script for Cross-Platform Support
# This script handles downloading and setting up FFmpeg prebuilt binaries
# for Windows (x86/x64), Linux (x86/x64), and macOS (x64)

function(setup_ffmpeg)
    set(FFMPEG_VERSION "6.0")
    set(FFMPEG_BASE_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest")

    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(FFMPEG_PACKAGE "ffmpeg-master-latest-win64-gpl-shared")
            set(PLATFORM_NAME "Windows x64")
        else()
            set(FFMPEG_PACKAGE "ffmpeg-master-latest-win32-gpl-shared")
            set(PLATFORM_NAME "Windows x86")
        endif()
        set(ARCHIVE_EXT "zip")
        set(LIB_PREFIX "")
        set(LIB_SUFFIX ".lib")
        set(SHARED_SUFFIX ".dll")
    elseif(APPLE)
        set(FFMPEG_PACKAGE "")
        set(PLATFORM_NAME "macOS")
        set(ARCHIVE_EXT "")
        set(LIB_PREFIX "lib")
        set(LIB_SUFFIX ".dylib")
        set(SHARED_SUFFIX ".dylib")
        set(USE_SYSTEM_FFMPEG TRUE)
    elseif(UNIX)
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
    endif()
    
    message(STATUS "Setting up FFmpeg for ${PLATFORM_NAME}")

    if(APPLE)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(FFMPEG QUIET libavformat libavcodec libavutil libswscale)
            if(FFMPEG_FOUND)
                message(STATUS "Found system FFmpeg via pkg-config")
                set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIRS} PARENT_SCOPE)
                set(FFMPEG_LIB_DIR ${FFMPEG_LIBRARY_DIRS} PARENT_SCOPE)
                set(FFMPEG_BIN_DIR "" PARENT_SCOPE)
                return()
            endif()
        endif()

        set(HOMEBREW_PREFIXES "/opt/homebrew" "/usr/local")
        foreach(PREFIX ${HOMEBREW_PREFIXES})
            if(EXISTS "${PREFIX}/include/libavformat/avformat.h")
                message(STATUS "Found FFmpeg in Homebrew at ${PREFIX}")
                set(FFMPEG_INCLUDE_DIR "${PREFIX}/include" PARENT_SCOPE)
                set(FFMPEG_LIB_DIR "${PREFIX}/lib" PARENT_SCOPE)
                set(FFMPEG_BIN_DIR "${PREFIX}/bin" PARENT_SCOPE)
                return()
            endif()
        endforeach()
        
        message(FATAL_ERROR "FFmpeg not found on macOS. Please install via Homebrew: brew install ffmpeg")
    else()
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
    endif()
    set(FFMPEG_LIB_PREFIX "${LIB_PREFIX}" PARENT_SCOPE)
    set(FFMPEG_LIB_SUFFIX "${LIB_SUFFIX}" PARENT_SCOPE)
    set(FFMPEG_SHARED_SUFFIX "${SHARED_SUFFIX}" PARENT_SCOPE)
    
endfunction()

function(find_ffmpeg_libraries)
    if(APPLE AND USE_SYSTEM_FFMPEG)
        find_library(AVFORMAT_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avformat${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(AVCODEC_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avcodec${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(AVUTIL_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avutil${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(SWSCALE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}swscale${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(SWRESAMPLE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}swresample${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(AVFILTER_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avfilter${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(AVDEVICE_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}avdevice${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
        )
        find_library(POSTPROC_LIB 
            NAMES ${FFMPEG_LIB_PREFIX}postproc${FFMPEG_LIB_SUFFIX}
            PATHS ${FFMPEG_LIB_DIR}
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

    if(NOT AVFORMAT_LIB OR NOT AVCODEC_LIB OR NOT AVUTIL_LIB OR NOT SWSCALE_LIB)
        message(FATAL_ERROR "Required FFmpeg libraries not found!")
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

function(setup_ffmpeg_post_build TARGET_NAME TARGET_DIR)
    if(WIN32)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Ensuring FFmpeg DLLs are present..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${FFMPEG_BIN_DIR}"
                "${TARGET_DIR}"
            COMMENT "Copying FFmpeg binaries to output directory"
        )
    elseif(UNIX AND NOT APPLE)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Ensuring FFmpeg shared libraries are present..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${FFMPEG_LIB_DIR}/libavformat${FFMPEG_SHARED_SUFFIX}*
                ${FFMPEG_LIB_DIR}/libavcodec${FFMPEG_SHARED_SUFFIX}*
                ${FFMPEG_LIB_DIR}/libavutil${FFMPEG_SHARED_SUFFIX}*
                ${FFMPEG_LIB_DIR}/libswscale${FFMPEG_SHARED_SUFFIX}*
                ${FFMPEG_LIB_DIR}/libswresample${FFMPEG_SHARED_SUFFIX}*
                ${FFMPEG_LIB_DIR}/libavfilter${FFMPEG_SHARED_SUFFIX}*
                ${FFMPEG_LIB_DIR}/libavdevice${FFMPEG_SHARED_SUFFIX}*
                "${TARGET_DIR}/"
            COMMENT "Copying FFmpeg shared libraries to output directory"
        )
    endif()
endfunction()