# crisp_audio — shared C++ audio-encoder library.
#
# Used by StelnetTTS (cielvox2_asr.cpp) and StelnetEmbed (BidirLM-Omni audio path)
# to share the Whisper-shape Conv-stem + Transformer-encoder forward pass.
#
# Linked into StelnetEmbed via add_subdirectory("../StelnetTTS/crisp_audio") from
# StelnetEmbed's top-level CMakeLists.txt.

cmake_minimum_required(VERSION 3.14)

# crisp_audio links against the consumer's "core" target — which provides
# core_gguf::* and core_mel::* symbols. StelnetTTS's stelnettts-core and
# StelnetEmbed's crispembed-core both supply these from identical sources;
# we pick whichever the parent build defines so consumers don't end up
# with duplicate symbols.
if(TARGET stelnettts-core)
    set(_crisp_audio_core_target stelnettts-core)
    set(_crisp_audio_core_inc    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
elseif(TARGET crispembed-core)
    set(_crisp_audio_core_target crispembed-core)
    # StelnetEmbed exposes core/* via crispembed-core's PUBLIC include dirs;
    # nothing extra needed here.
    set(_crisp_audio_core_inc "")
else()
    message(STATUS "crisp_audio: no core target (stelnettts-core or crispembed-core) — skipping")
    return()
endif()

add_library(crisp_audio STATIC
    src/audio_tower.cpp
    include/crisp_audio.h
)

target_include_directories(crisp_audio
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(_crisp_audio_core_inc)
    target_include_directories(crisp_audio PRIVATE ${_crisp_audio_core_inc})
endif()

target_link_libraries(crisp_audio
    PUBLIC  ${_crisp_audio_core_target} ggml ggml-base
)

target_compile_features(crisp_audio PUBLIC cxx_std_17)
set_target_properties(crisp_audio PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(NOT MSVC)
    target_compile_options(crisp_audio PRIVATE -O3)
endif()

if(GGML_METAL)
    target_compile_definitions(crisp_audio PRIVATE GGML_USE_METAL)
    target_link_libraries(crisp_audio PUBLIC stelnettts_link_ggml_metal)
endif()
if(GGML_CUDA)
    target_compile_definitions(crisp_audio PRIVATE GGML_USE_CUDA)
    target_link_libraries(crisp_audio PUBLIC stelnettts_link_ggml_cuda)
endif()
