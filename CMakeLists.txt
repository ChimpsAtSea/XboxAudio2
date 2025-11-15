cmake_minimum_required(VERSION 3.20)
include(FetchContent)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

set(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/vcpkg")
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")
set(VCPKG_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# --- clone vcpkg ---
if(NOT EXISTS "${VCPKG_ROOT}")
    message(STATUS "Cloning vcpkg...")
    execute_process(
        COMMAND git clone https://github.com/microsoft/vcpkg.git --depth 1 "${VCPKG_ROOT}"
        RESULT_VARIABLE GIT_CLONE_RESULT
    )
    if(NOT GIT_CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone vcpkg")
    endif()
endif()
if(NOT EXISTS "${VCPKG_TOOLCHAIN_FILE}")
  message(FATAL_ERROR "vcpkg toolchain file not found '${VCPKG_TOOLCHAIN_FILE}'")
endif()

## --- vcpkg cmake toolchain ---
include(${VCPKG_TOOLCHAIN_FILE})

# --- xaudio2 library ---
project(XboxAudio2 LANGUAGES CXX ASM_MASM)

# --- add ffmpeg package to cmake ---
find_package(FFMPEG REQUIRED)

# --- xaudio2 project ---
set(XAUDIO2_VARIANT "1" CACHE STRING "XAudio2 Variant")

file(GLOB_RECURSE XAUDIO_SOURCES CONFIGURE_DEPENDS "include/*.h" "source/*.cpp" "source/*.h" "source/*.asm")
add_library(XAudio2_9 SHARED ${XAUDIO_SOURCES})
target_compile_definitions(XAudio2_9 PRIVATE "XAUDIO2_VARIANT=${XAUDIO2_VARIANT}")
target_include_directories(XAudio2_9 PRIVATE include)

target_include_directories(XAudio2_9 PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(XAudio2_9 PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(XAudio2_9 PRIVATE ${FFMPEG_LIBRARIES})

find_package(wil CONFIG REQUIRED)
target_link_libraries(XAudio2_9 PRIVATE WIL::WIL)
