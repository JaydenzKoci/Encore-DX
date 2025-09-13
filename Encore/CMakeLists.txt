# CMakeList.txt : CMake project for Encore, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.11)
project(Encore)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES x86_64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories to match expected structure
# Use config-specific directories for multi-config generators (like Visual Studio)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Encore/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Encore/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/Encore/RelWithDebInfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/Encore/MinSizeRel)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Encore/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Encore/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/Encore/RelWithDebInfo)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/Encore/MinSizeRel)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Encore/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Encore/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/Encore/RelWithDebInfo)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/Encore/MinSizeRel)

# For single-config generators (like Make), use the build type
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
endif()

# Dependencies
include(FetchContent)

set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  FetchContent_Declare(
          raylib
          DOWNLOAD_EXTRACT_TIMESTAMP OFF
          URL https://github.com/Encore-Developers/raylib-Source/archive/refs/heads/master.zip
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()

find_package(json QUIET)
if (NOT json_FOUND)
  FetchContent_Declare(
          json
          DOWNLOAD_EXTRACT_TIMESTAMP OFF
          URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
  FetchContent_GetProperties(json)
  if (NOT json_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(json)
  endif()
endif()

if(WIN32)
  FetchContent_Declare(
          zlib
          GIT_REPOSITORY https://github.com/madler/zlib.git
          GIT_TAG v1.3.1
  )
  set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Z" FORCE)
  FetchContent_MakeAvailable(zlib)
endif()

# FFmpeg Configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup-ffmpeg.cmake)
setup_ffmpeg()
find_ffmpeg_libraries()


option(SUPPORT_FILEFORMAT_JPG "Support loading JPG as textures" ON)

file(GLOB_RECURSE SRC_FILES "src/*.cpp" "include/*.cpp" "include/*.c")
file(GLOB_RECURSE INC_FILES "include/*.h" "include/*.h*" "src/*.h")
if (WIN32)
  add_executable(Encore ${SRC_FILES} ${INC_FILES} "resources.rc")
else()
  add_executable(Encore ${SRC_FILES} ${INC_FILES})
endif()

# Copy assets to the appropriate output directory based on configuration
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config generators
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        file(COPY "Songs" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
        file(COPY "Assets" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
    endforeach()
else()
    # Single-config generators
    file(COPY "Songs" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    file(COPY "Assets" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
endif()
target_include_directories(Encore PRIVATE "include" "src" ${FFMPEG_INCLUDE_DIR})
if(WIN32)
  if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/windows/x86/")
    find_library(BASS NAMES bass PATHS "lib/bass/windows/x86/")
    find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/windows/x86/")
    
    # Copy DLLs to appropriate output directories
    if(CMAKE_CONFIGURATION_TYPES)
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            file(COPY "lib/discord-rpc/windows/x86/discord-rpc.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
            file(COPY "lib/bass/windows/x86/bass.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
            file(COPY "lib/bass/windows/x86/bassopus.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
        endforeach()
    else()
        file(COPY "lib/discord-rpc/windows/x86/discord-rpc.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
        file(COPY "lib/bass/windows/x86/bass.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
        file(COPY "lib/bass/windows/x86/bassopus.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    endif()
  endif()
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/windows/x64/")
    find_library(BASS NAMES bass PATHS "lib/bass/windows/x64/")
    find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/windows/x64/")
    
    # Copy DLLs to appropriate output directories
    if(CMAKE_CONFIGURATION_TYPES)
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            file(COPY "lib/discord-rpc/windows/x64/discord-rpc.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
            file(COPY "lib/bass/windows/x64/bass.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
            file(COPY "lib/bass/windows/x64/bassopus.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${config})
        endforeach()
    else()
        file(COPY "lib/discord-rpc/windows/x64/discord-rpc.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
        file(COPY "lib/bass/windows/x64/bass.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
        file(COPY "lib/bass/windows/x64/bassopus.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    endif()
  endif()
  
  # Copy FFmpeg binaries for Windows - this will be handled by the post-build step
endif()
if(UNIX AND NOT APPLE)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/linux/x64")
    find_library(BASS NAMES bass PATHS "lib/bass/linux/x86_64")
    find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/linux/x86_64")
    
    # Copy shared libraries to output directory
    file(COPY "lib/discord-rpc/linux/x64/libdiscord-rpc.so" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    file(COPY "lib/bass/linux/x86_64/libbass.so" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    file(COPY "lib/bass/linux/x86_64/libbassopus.so" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
  endif()
  
  # Copy FFmpeg binaries for Linux - this will be handled by the post-build step
endif()
if(APPLE)
  find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/macos")
  find_library(BASS NAMES bass PATHS "lib/bass/macos")
  find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/macos")
  
  # Copy dynamic libraries to output directory
  file(COPY "lib/discord-rpc/macos/libdiscord-rpc.dylib" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
  file(COPY "lib/bass/macos/libbass.dylib" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
  file(COPY "lib/bass/macos/libbassopus.dylib" DESTINATION ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
  
  # Copy FFmpeg binaries for macOS - this will be handled by the post-build step
endif()

execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(Encore PRIVATE
        "-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")

target_compile_definitions(Encore PRIVATE
        "-DENCORE_VERSION=\"v0.2.0\"")

target_compile_definitions(Encore PRIVATE
        "-DGIT_BRANCH=\"DELUXE\"")

set_property(TARGET Encore PROPERTY CXX_STANDARD 20)

# FFmpeg libraries are now found by the helper script

set(ENCORE_LIBRARIES
        raylib
        nlohmann_json::nlohmann_json
        ${BASS}
        ${BASSOPUS}
        ${DISCORD-RPC}
        ${FFMPEG_LIBRARIES}
)
if(UNIX AND NOT APPLE)
  find_package(ZLIB REQUIRED)
  list(APPEND ENCORE_LIBRARIES ZLIB::ZLIB)
endif()
if(UNIX AND APPLE)
  list(APPEND ENCORE_LIBRARIES zlibstatic bcrypt)
endif()
if(WIN32)
  list(APPEND ENCORE_LIBRARIES zlibstatic bcrypt)
endif()
if(UNIX AND NOT APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-fatal-warnings")
endif()

target_link_libraries(Encore PRIVATE ${ENCORE_LIBRARIES})

# Add post-build step to ensure FFmpeg DLLs are available
if(CMAKE_CONFIGURATION_TYPES)
    # For multi-config generators, we need to use generator expressions
    setup_ffmpeg_post_build(Encore ${CMAKE_BINARY_DIR}/Encore/$<CONFIG>)
else()
    # For single-config generators
    setup_ffmpeg_post_build(Encore ${CMAKE_BINARY_DIR}/Encore/${CMAKE_BUILD_TYPE})
endif()

