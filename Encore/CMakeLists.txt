# CMakeList.txt : CMake project for Encore, include source and define
# project specific logic here.
#



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES x86_64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
  FetchContent_MakeAvailable(zlib)
endif()

FetchContent_Declare(
        ffmpeg
        GIT_REPOSITORY https://github.com/FFmpeg/FFmpeg.git
        GIT_TAG n6.0
)

find_program(CCACHE_EXECUTABLE ccache)
if(CCACHE_EXECUTABLE)
  message(STATUS "Found ccache, will use it to speed up FFmpeg rebuilds.")
  set(FFMPEG_ENABLE_CCACHE "--enable-ccache")
else()
  message(STATUS "ccache not found. For faster FFmpeg rebuilds, consider installing it (e.g., 'choco install ccache').")
  set(FFMPEG_ENABLE_CCACHE "")
endif()


set(FFMPEG_CONFIGURE_FLAGS
        "--disable-everything"
        "--disable-programs"
        "--disable-doc"
        "--disable-network"
        "--disable-avdevice"
        "--disable-avfilter"
        "--disable-swresample"
        "--disable-postproc"
        "--disable-encoders"
        "--disable-muxers"
        "--disable-bsfs"
        "--disable-protocols"
        "--disable-indevs"
        "--disable-outdevs"
        "--disable-x86asm"
        "--enable-protocol=file"
        "--enable-decoder=h264,hevc,vp9,mpeg4,wmv3,aac,mp3"
        "--enable-demuxer=mov,mp4,m4a,3gp,3g2,mj2,mkv,webm"
        "--enable-swscale"
        "--enable-avformat"
        "--enable-avcodec"
        ${FFMPEG_ENABLE_CCACHE}
)

set(CMAKE_PROJECT_FFmpeg_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/include)
set(FFMPEG_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/build)
set(FFMPEG_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/install)

FetchContent_GetProperties(ffmpeg)
if(NOT ffmpeg_POPULATED)
  FetchContent_MakeAvailable(ffmpeg)

  if(WIN32)
    find_program(SH_EXECUTABLE sh HINTS "C:/Program Files/Git/bin")
    if(NOT SH_EXECUTABLE)
      message(FATAL_ERROR "sh.exe not found. A POSIX shell (like Git Bash or MSYS2) is required to build FFmpeg on Windows and must be in the system PATH.")
    endif()
    find_program(MAKE_EXECUTABLE mingw32-make)
    if(NOT MAKE_EXECUTABLE)
      find_program(MAKE_EXECUTABLE make)
    endif()

    set(CONFIGURE_COMMAND ${SH_EXECUTABLE} ${ffmpeg_SOURCE_DIR}/configure)
  else()
    find_program(MAKE_EXECUTABLE make)
    set(CONFIGURE_COMMAND ${ffmpeg_SOURCE_DIR}/configure)
  endif()

  if(NOT MAKE_EXECUTABLE)
    message(FATAL_ERROR "'make' executable not found, but it is required to build FFmpeg.")
  endif()

  # --- Run FFmpeg's configure script ---
  execute_process(
          COMMAND ${CONFIGURE_COMMAND} --prefix=${FFMPEG_INSTALL_DIR} ${FFMPEG_CONFIGURE_FLAGS}
          WORKING_DIRECTORY ${ffmpeg_SOURCE_DIR}
          RESULT_VARIABLE FFMPEG_CONFIGURE_RESULT
          OUTPUT_VARIABLE FFMPEG_OUTPUT
          ERROR_VARIABLE FFMPEG_ERROR
  )
  if(NOT FFMPEG_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "FFmpeg configure failed.\nOutput:\n${FFMPEG_OUTPUT}\nError:\n${FFMPEG_ERROR}")
  endif()

  execute_process(
          COMMAND ${MAKE_EXECUTABLE} install -j
          WORKING_DIRECTORY ${ffmpeg_SOURCE_DIR}
          RESULT_VARIABLE FFMPEG_BUILD_RESULT
          OUTPUT_VARIABLE FFMPEG_OUTPUT
          ERROR_VARIABLE FFMPEG_ERROR
  )
  if(NOT FFMPEG_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "FFmpeg build failed.\nOutput:\n${FFMPEG_OUTPUT}\nError:\n${FFMPEG_ERROR}")
  endif()
endif()


option(SUPPORT_FILEFORMAT_JPG "Support loading JPG as textures" ON)

file(GLOB_RECURSE SRC_FILES "src/*.cpp" "include/*.cpp" "include/*.c")
file(GLOB_RECURSE INC_FILES "include/*.h" "include/*.h*" "src/*.h")

if (WIN32)
  add_executable(Encore ${SRC_FILES} ${INC_FILES} "resources.rc")
else()
  add_executable(Encore ${SRC_FILES} ${INC_FILES})
endif()

file(COPY "Songs" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
file(COPY "Assets" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
target_include_directories(Encore PRIVATE "include" "src" ${FFMPEG_INSTALL_DIR}/include)
if(WIN32)
  if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/windows/x86/")
    file(COPY "lib/discord-rpc/windows/x86/discord-rpc.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    find_library(BASS NAMES bass PATHS "lib/bass/windows/x86/")
    file(COPY "lib/bass/windows/x86/bass.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/windows/x86/")
    file(COPY "lib/bass/windows/x86/bassopus.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
  endif()
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/windows/x64/")
    file(COPY "lib/discord-rpc/windows/x64/discord-rpc.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    find_library(BASS NAMES bass PATHS "lib/bass/windows/x64/")
    file(COPY "lib/bass/windows/x64/bass.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/windows/x64/")
    file(COPY "lib/bass/windows/x64/bassopus.dll" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
  endif()
endif()
if(UNIX AND NOT APPLE)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/linux/x64")
    file(COPY "lib/discord-rpc/linux/x64/libdiscord-rpc.so" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    find_library(BASS NAMES bass PATHS "lib/bass/linux/x86_64")
    file(COPY "lib/bass/linux/x86_64/libbass.so" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/linux/x86_64")
    file(COPY "lib/bass/linux/x86_64/libbassopus.so" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
  endif()
endif()
if(APPLE)
  find_library(DISCORD-RPC NAMES discord-rpc PATHS "lib/discord-rpc/macos")
  file(COPY "lib/discord-rpc/macos/libdiscord-rpc.dylib" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
  find_library(BASS NAMES bass PATHS "lib/bass/macos")
  file(COPY "lib/bass/macos/libbass.dylib" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
  find_library(BASSOPUS NAMES bassopus PATHS "lib/bass/macos")
  file(COPY "lib/bass/macos/libbassopus.dylib" DESTINATION ${CMAKE_BINARY_DIR}/Encore)
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

target_compile_definitions(${PROJECT_NAME} PRIVATE
        "-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")

target_compile_definitions(${PROJECT_NAME} PRIVATE
        "-DENCORE_VERSION=\"v0.2.0\"")

target_compile_definitions(${PROJECT_NAME} PRIVATE
        "-DGIT_BRANCH=\"DELUXE\"")

set_property(TARGET Encore PROPERTY CXX_STANDARD 20)

find_library(AVFORMAT_LIB avformat PATHS ${FFMPEG_INSTALL_DIR}/lib)
find_library(AVCODEC_LIB avcodec PATHS ${FFMPEG_INSTALL_DIR}/lib)
find_library(AVUTIL_LIB avutil PATHS ${FFMPEG_INSTALL_DIR}/lib)
find_library(SWSCALE_LIB swscale PATHS ${FFMPEG_INSTALL_DIR}/lib)

set(ENCORE_LIBRARIES
        raylib
        nlohmann_json::nlohmann_json
        ${BASS}
        ${BASSOPUS}
        ${DISCORD-RPC}
        ${AVFORMAT_LIB}
        ${AVCODEC_LIB}
        ${AVUTIL_LIB}
        ${SWSCALE_LIB}
)

if(WIN32)
  list(APPEND ENCORE_LIBRARIES zlibstatic bcrypt)
endif()

target_link_libraries(Encore PRIVATE ${ENCORE_LIBRARIES})

