cmake_minimum_required(VERSION 3.20)
include(FetchContent)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# --- xaudio2 library ---
project(XboxAudio2 LANGUAGES CXX ASM_MASM)

# --- ffmpeg (built from the FFmpeg-Cmake submodule) ---
include(ExternalProject)

set(FFMPEG_SUBMODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/FFmpeg-Cmake")
set(FFMPEG_BUILD_DIR     "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")

# Multi-config generators (Visual Studio) emit per-config subdirectories;
# single-config generators (Ninja) do not.
get_property(FFMPEG_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(FFMPEG_MULTI_CONFIG)
    set(FFMPEG_CONFIG_SUBDIR "/$<CONFIG>")
else()
    set(FFMPEG_CONFIG_SUBDIR "")
endif()
set(FFMPEG_BIN_DIR "${FFMPEG_BUILD_DIR}/bin${FFMPEG_CONFIG_SUBDIR}")
set(FFMPEG_LIB_DIR "${FFMPEG_BUILD_DIR}/lib${FFMPEG_CONFIG_SUBDIR}")

# Public FFmpeg headers live in the source tree; avconfig.h / ffversion.h are
# generated into the build tree, so both directories are needed at compile time.
set(FFMPEG_INCLUDE_DIRS
    "${FFMPEG_SUBMODULE_DIR}/FFmpeg"
    "${FFMPEG_BUILD_DIR}"
)

# Libraries we consume (avcodec also needs avutil + swresample at runtime).
set(FFMPEG_COMPONENTS avcodec avutil swresample)
set(FFMPEG_LIBRARIES "")
set(FFMPEG_RUNTIME_DLLS "")
foreach(_lib IN LISTS FFMPEG_COMPONENTS)
    list(APPEND FFMPEG_LIBRARIES    "${FFMPEG_LIB_DIR}/${_lib}.lib")
    list(APPEND FFMPEG_RUNTIME_DLLS "${FFMPEG_BIN_DIR}/${_lib}.dll")
endforeach()

# Forward this build's generator (and VS platform/toolset) to the sub-build.
set(FFMPEG_GEN_ARGS -G "${CMAKE_GENERATOR}")
if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND FFMPEG_GEN_ARGS -A "${CMAKE_GENERATOR_PLATFORM}")
endif()
if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND FFMPEG_GEN_ARGS -T "${CMAKE_GENERATOR_TOOLSET}")
endif()
if(NOT FFMPEG_MULTI_CONFIG)
    list(APPEND FFMPEG_GEN_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

# Build only the decoders this project needs: XMA, XMA2 and the WMA family.
# Start from a blank slate (everything off) and opt in to just these.
list(APPEND FFMPEG_GEN_ARGS
    -DFFMPEG_USE_DEFAULT_CONFIG=OFF
    -DFFMPEG_CONFIG_XMA1_DECODER=ON
    -DFFMPEG_CONFIG_XMA2_DECODER=ON
    -DFFMPEG_CONFIG_WMAV1_DECODER=ON
    -DFFMPEG_CONFIG_WMAV2_DECODER=ON
    -DFFMPEG_CONFIG_WMAPRO_DECODER=ON
    -DFFMPEG_CONFIG_WMALOSSLESS_DECODER=ON
    -DFFMPEG_CONFIG_WMAVOICE_DECODER=ON
)

# The submodule is a standalone top-level CMake project (it relies on
# CMAKE_SOURCE_DIR), so build it via ExternalProject rather than add_subdirectory.
ExternalProject_Add(ffmpeg
    SOURCE_DIR "${FFMPEG_SUBMODULE_DIR}"
    BINARY_DIR "${FFMPEG_BUILD_DIR}"
    CONFIGURE_COMMAND
        "${CMAKE_COMMAND}" -S <SOURCE_DIR> -B <BINARY_DIR>
        #"${CMAKE_COMMAND}" -S <SOURCE_DIR> -B <BINARY_DIR> ${FFMPEG_GEN_ARGS}
    BUILD_COMMAND
        "${CMAKE_COMMAND}" --build <BINARY_DIR> --config $<CONFIG>
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${FFMPEG_LIBRARIES} ${FFMPEG_RUNTIME_DLLS}
)

# --- xaudio2 project ---
set(XAUDIO2_VARIANT "1" CACHE STRING "XAudio2 Variant")

file(GLOB_RECURSE XAUDIO_SOURCES CONFIGURE_DEPENDS "include/*.h" "source/*.cpp" "source/*.h" "source/*.asm")
add_library(XAudio2_9 SHARED ${XAUDIO_SOURCES})
target_compile_definitions(XAudio2_9 PRIVATE "XAUDIO2_VARIANT=${XAUDIO2_VARIANT}")
target_include_directories(XAudio2_9 PRIVATE include)

target_include_directories(XAudio2_9 PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(XAudio2_9 PRIVATE ${FFMPEG_LIBRARIES})
add_dependencies(XAudio2_9 ffmpeg)

# --- copy ffmpeg runtime dlls next to the built library for local runs ---
add_custom_command(TARGET XAudio2_9 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FFMPEG_RUNTIME_DLLS} "$<TARGET_FILE_DIR:XAudio2_9>"
    COMMAND_EXPAND_LISTS
)

# --- install target ---
install(TARGETS XAudio2_9 EXPORT XboxAudio2Targets)

# --- install ffmpeg runtime dlls ---
install(FILES ${FFMPEG_RUNTIME_DLLS} DESTINATION ${CMAKE_INSTALL_PREFIX})
