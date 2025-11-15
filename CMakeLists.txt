cmake_minimum_required(VERSION 3.20)
project(XboxAudio2 LANGUAGES CXX ASM_MASM)

file(GLOB_RECURSE XAUDIO_SOURCES CONFIGURE_DEPENDS "include/*.h" "source/*.cpp" "source/*.h" "source/*.asm")
add_library(XAudio2_9 SHARED ${XAUDIO_SOURCES})
target_include_directories(XAudio2_9 PRIVATE include)

find_package(FFMPEG REQUIRED)
target_include_directories(XAudio2_9 PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(XAudio2_9 PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(XAudio2_9 PRIVATE ${FFMPEG_LIBRARIES})

find_package(wil CONFIG REQUIRED)
target_link_libraries(XAudio2_9 PRIVATE WIL::WIL)
