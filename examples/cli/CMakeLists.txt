#
# stelnettts — unified ASR CLI (formerly stelnettts).
#
# Drives all model backends in src/ (stelnettts, parakeet, canary, cohere,
# qwen3, voxtral, voxtral4b, granite) from a single binary via the
# --backend flag. When --backend is omitted or set to "whisper" the
# historical stelnettts code path is used unchanged.
#
# The CMake target is named `stelnettts-cli`, but the shipped binary stays
# `stelnettts` via OUTPUT_NAME for user-facing compatibility.

set(TARGET stelnettts-cli)

# The CLI records consent hashes even when C2PA signing is disabled.  Its
# header therefore uses the shared SHA-256 shim independently of
# stelnettts_enable_c2pa(), which only configures the optional signing library.
set(_stelnettts_c2pa_audio_include "${CMAKE_SOURCE_DIR}/third_party/c2pa-audio/src")
if (NOT EXISTS "${_stelnettts_c2pa_audio_include}/sha256.h")
    message(FATAL_ERROR
        "The c2pa-audio submodule is required by the CLI consent record "
        "(missing ${_stelnettts_c2pa_audio_include}/sha256.h). Initialize submodules with: "
        "git submodule update --init --recursive")
endif()

add_executable(${TARGET}
    cli.cpp
    stelnettts_backend.cpp
    stelnettts_backend_stelnettts.cpp
    stelnettts_backend_nemotron.cpp
    stelnettts_backend_gigaam.cpp
    stelnettts_backend_parakeet.cpp
    stelnettts_backend_canary.cpp
    stelnettts_backend_canary_qwen.cpp
    stelnettts_backend_lfm2_audio.cpp
    stelnettts_backend_mini_omni2.cpp
    stelnettts_backend_cohere.cpp
    stelnettts_backend_granite.cpp
    stelnettts_backend_granite_nle.cpp
    stelnettts_backend_voxtral.cpp
    stelnettts_backend_voxtral4b.cpp
    stelnettts_backend_voxtral_tts.cpp
    stelnettts_backend_higgs_stt.cpp
    stelnettts_backend_cielvox2.cpp
    stelnettts_backend_fastconformer_ctc.cpp
    stelnettts_backend_wav2vec2.cpp
    stelnettts_backend_vibevoice.cpp
    stelnettts_backend_kugelaudio.cpp
    stelnettts_backend_cielvox2_tts.cpp
    stelnettts_backend_omnivoice.cpp
    stelnettts_backend_orpheus.cpp
    stelnettts_backend_chatterbox.cpp
    stelnettts_backend_tada.cpp
    stelnettts_backend_indextts.cpp
    stelnettts_backend_f5_tts.cpp
    stelnettts_backend_irodori_tts.cpp
    stelnettts_backend_pocket_tts.cpp
    stelnettts_backend_csm.cpp
    stelnettts_backend_parler.cpp
    stelnettts_backend_fastpitch.cpp
    stelnettts_backend_bananamind_tts.cpp
    stelnettts_backend_bark.cpp
    stelnettts_backend_dia.cpp
    stelnettts_backend_dots_tts.cpp
    stelnettts_backend_voxcpm2_tts.cpp
    stelnettts_backend_voxcpm2_vae.cpp
    stelnettts_backend_cosyvoice3.cpp
    stelnettts_backend_kokoro.cpp
    stelnettts_backend_piper.cpp
    stelnettts_backend_melotts.cpp
    stelnettts_backend_m2m100.cpp
    stelnettts_backend_t5.cpp
    stelnettts_backend_glm_asr.cpp
    stelnettts_backend_htdemucs.cpp
    stelnettts_backend_crepe.cpp
    stelnettts_backend_btc.cpp
    stelnettts_backend_rvc.cpp
    stelnettts_piano_cli.cpp
    stelnettts_backend_beat_this.cpp
    stelnettts_backend_mel_band_roformer.cpp
    stelnettts_backend_kyutai_stt.cpp
    stelnettts_backend_firered_asr.cpp
    stelnettts_backend_moonshine.cpp
    stelnettts_backend_moonshine_streaming.cpp
    stelnettts_backend_gemma4_e2b.cpp
    stelnettts_backend_omniasr.cpp
    stelnettts_backend_mimo_asr.cpp
    stelnettts_backend_ark_asr.cpp
    stelnettts_backend_miotts.cpp
    stelnettts_backend_piano_transcription.cpp
    stelnettts_backend_moss_audio.cpp
    stelnettts_backend_moss_tts.cpp
    stelnettts_backend_moss_tts_local.cpp
    stelnettts_backend_moss_transcribe.cpp
    stelnettts_backend_moss_transcribe_diarize.cpp
    stelnettts_backend_funasr.cpp
    stelnettts_backend_paraformer.cpp
    stelnettts_backend_sensevoice.cpp
    stelnettts_backend_sidon.cpp
    stelnettts_backend_speecht5.cpp
    stelnettts_backend_zonos.cpp
    stelnettts_aligner_cli.cpp
    stelnettts_vad_cli.cpp
    stelnettts_separate_cli.cpp
    stelnettts_beats_cli.cpp
    stelnettts_chords_cli.cpp
    stelnettts_tab_cli.cpp
    stelnettts_backend_tabcnn.cpp
    stelnettts_pitch_cli.cpp
    stelnettts_lid_cli.cpp
    stelnettts_diarize_cli.cpp
    stelnettts_output.cpp
    stelnettts_model_mgr_cli.cpp
    stelnettts_diagnostics.cpp
    stelnettts_run.cpp
    stelnettts_server.cpp
    stelnettts_tts_chunking.cpp
    # Real-time WebSocket ASR streaming, shared with the standalone server
    # (examples/server). Wired into `stelnettts --server` via --ws-port.
    ${CMAKE_CURRENT_SOURCE_DIR}/../server/ws_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../server/realtime_server.cpp
    # Wyoming protocol TCP server (issue #172) — Home Assistant Assist STT+TTS.
    wyoming.cpp
)

# ---------------------------------------------------------------------------
# Build-info macros surfaced to stelnettts_diagnostics.cpp. Together they
# answer "what exact bits am I running?" — important for bug reports
# from prebuilt Docker tags where the user has no source tree handy.
#
# `git-vars.cmake` populates GIT_SHA1 / GIT_DATE / GIT_COMMIT_SUBJECT.
# Re-include defensively in case a downstream consumer of this file
# imports it without the top-level CMakeLists having run yet.
# ---------------------------------------------------------------------------
if (NOT DEFINED GIT_SHA1)
    include(${CMAKE_SOURCE_DIR}/cmake/git-vars.cmake)
endif()
string(TIMESTAMP STELNETTTS_BUILD_DATE_STR "%Y-%m-%dT%H:%M:%SZ" UTC)
target_compile_definitions(${TARGET} PRIVATE
    STELNETTTS_VERSION_STR="${PROJECT_VERSION}"
    STELNETTTS_GIT_SHA1="${GIT_SHA1}"
    STELNETTTS_GIT_DATE="${GIT_DATE}"
    STELNETTTS_GIT_SUBJECT="${GIT_COMMIT_SUBJECT}"
    STELNETTTS_BUILD_DATE="${STELNETTTS_BUILD_DATE_STR}"
    STELNETTTS_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)
if (GGML_CUDA)
    # CMAKE_CUDA_ARCHITECTURES is a CMake list ("75-real;86-real;..."). When
    # fed into target_compile_definitions, CMake's command-line generator
    # treats list items as SEPARATE arguments — the cl.exe line ends up as
    # `-DSTELNETTTS_CUDA_ARCHS="75-real -D86-real -D120-virtual"` which has
    # an unbalanced quote and then leftover `-D120-virtual"` confuses
    # link.exe into trying to open random tokens as .obj files
    # ("LNK1181: cannot open input file 'GIT_DATE.obj'"). Convert the
    # separator to "," before embedding so it remains a single token.
    string(REPLACE ";" "," STELNETTTS_CUDA_ARCHS_STR "${CMAKE_CUDA_ARCHITECTURES}")
    target_compile_definitions(${TARGET} PRIVATE
        STELNETTTS_HAVE_CUDA=1
        STELNETTTS_CUDA_VERSION="${CUDAToolkit_VERSION}"
        STELNETTTS_CUDA_ARCHS="${STELNETTTS_CUDA_ARCHS_STR}"
    )
endif()
if (GGML_METAL)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_METAL=1)
endif()
if (GGML_VULKAN)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_VULKAN=1)
endif()
if (GGML_HIP)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_HIP=1)
endif()
if (GGML_SYCL)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_SYCL=1)
endif()
if (GGML_OPENCL)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_OPENCL=1)
endif()
if (GGML_BLAS)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_BLAS=1)
endif()
if (GGML_MUSA)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_MUSA=1)
endif()

# ---------------------------------------------------------------------------
# stelnettts-diff-lib — shared ground-truth diff harness (companion to
# tools/dump_reference.py).
#
# Exposed as a small static library so both the `stelnettts-diff` binary
# and the older differential-test drivers under
# examples/{qwen3,voxtral}-test-*/ can link it without re-compiling the
# same sources. The library has no per-model dependencies — it only
# needs core_gguf from stelnettts-core and the ggml CPU backend for its
# tensor loader.
# ---------------------------------------------------------------------------
add_library(stelnettts-diff-lib STATIC
    stelnettts_diff.cpp
    stelnettts_diff.h
)
target_include_directories(stelnettts-diff-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(stelnettts-diff-lib PUBLIC
    stelnettts-core
    ggml
 btc-chords
    tabcnn
    rvc-svc
    beatrice-pitch)
set_target_properties(stelnettts-diff-lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# stelnettts-diff — the CLI wrapper binary. Separate target so downstream
# users who don't care about differential testing don't pay the extra
# link time.
# ---------------------------------------------------------------------------
add_executable(stelnettts-diff
    stelnettts_diff_main.cpp
)
target_link_libraries(stelnettts-diff PRIVATE
    common
    stelnettts-lib
    parakeet
    wespeaker
    gigaam
    canary
    cohere
    granite_speech
    granite_nle
    voxtral
    voxtral4b
    voxtral_tts
    higgs_stt
    cielvox2_asr
    cielvox2_tts
    omnivoice
    orpheus
    kokoro
    chatterbox
    tada-tts
    tada-encoder
    indextts
    m2m100
    gemma4_e2b
    mimo_tokenizer
    mimo_asr
    ark_asr
    moss_audio
    moss_tts
    moss_tts_local
    moss_transcribe
    moss_transcribe_diarize
    voxcpm2_tts
    funasr
    paraformer
    sensevoice
    sidon
    cosyvoice3_tts
    csm-tts
    parler-tts
    mini-omni2
    dots-tts
    miotts
    miocodec
    crepe
    stelnettts-diff-lib
    ${FFMPEG_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
install(TARGETS stelnettts-diff RUNTIME BUNDLE DESTINATION bin)

# ---------------------------------------------------------------------------
# stelnettts-lid — standalone text language identification.
# Auto-routes between fastText (GlotLID-V3, LID-176) and Google CLD3 by
# peeking the GGUF's general.architecture key at load time. One binary
# for any text-LID GGUF.
#
# Link order: text-lid-dispatch BEFORE stelnettts-lib because the dispatch
# static library has unresolved references to stelnettts_registry_lookup
# / stelnettts_cache::ensure_cached_file (in stelnettts_model_registry.cpp
# / stelnettts_cache.cpp inside the main `stelnettts-lib` library), and MinGW's
# GNU ld processes static libraries strictly left-to-right — listing
# the consumer first lets ld pick up the missing definitions when it
# subsequently scans `stelnettts-lib`. Without this, msys2 UCRT64 build.yml
# fails with `undefined reference to stelnettts_registry_lookup` etc.
# (Windows MSVC and Linux gcc are forgiving here; only MinGW UCRT64
# enforces order strictly enough to surface the bug.)
#
# Windows networking libs match stelnettts-cli's set: winhttp for
# auto-download (text-lid GGUFs come from HF over HTTPS), ws2_32 +
# crypt32 because httplib.h's #pragma comment line auto-links them on
# MSVC but MinGW silently ignores those pragmas.
# ---------------------------------------------------------------------------
add_executable(stelnettts-lid stelnettts_lid_main.cpp)
if(TARGET crisp_lid)
    target_link_libraries(stelnettts-lid PRIVATE crisp_lid stelnettts-lib)
else()
    target_link_libraries(stelnettts-lid PRIVATE text-lid-dispatch stelnettts-lib)
endif()
target_link_libraries(stelnettts-lid PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    $<$<BOOL:${WIN32}>:winhttp>
    $<$<BOOL:${WIN32}>:ws2_32>
    $<$<BOOL:${WIN32}>:crypt32>
)
install(TARGETS stelnettts-lid RUNTIME BUNDLE DESTINATION bin)

# ---------------------------------------------------------------------------
# mimo-tokenizer-smoke — finiteness/magnitude smoke test for the MiMo
# tokenizer encoder forward path. Used during PLAN #51 step 2 development
# while the upstream safetensors download is in flight.
# ---------------------------------------------------------------------------
add_executable(mimo-tokenizer-smoke mimo_tokenizer_smoke.cpp)
target_link_libraries(mimo-tokenizer-smoke PRIVATE
    common
    stelnettts-lib
    mimo_tokenizer
    ${FFMPEG_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# 4B MossTTSLocal (#249) — P5-lite runtime smoke test (generate_codes end-to-end).
if (TARGET moss_tts_local)
    add_executable(moss-tts-local-smoke moss_tts_local_smoke.cpp)
    target_link_libraries(moss-tts-local-smoke PRIVATE
        moss_tts_local
        ${CMAKE_THREAD_LIBS_INIT}
    )
endif()

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME stelnettts)

# httplib.h is in the server example directory
target_include_directories(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/examples/server
    ${_stelnettts_c2pa_audio_include}
)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE
    common
    stelnettts-lib
    parakeet
    wespeaker
    gigaam
    canary
    cohere
    granite_speech
    granite_nle
    voxtral
    voxtral4b
    voxtral_tts
    higgs_stt
    cielvox2_asr
    cielvox2_tts
    omnivoice
    orpheus
    chatterbox
    tada-tts
    tada-encoder
    indextts
    kokoro
    piper-tts
    speecht5-tts
    fastpitch-tts
    bananamind-tts
    bark-tts
    dia-tts
    dots-tts
    irodori-tts
    pocket-tts
    csm-tts
    voxcpm2_tts
    m2m100
    t5_translate
    canary_ctc
    wav2vec2-ggml
    vibevoice
    kugelaudio
    silero-lid
    pyannote-seg
    funasr
    paraformer
    sensevoice
    sidon
    moss_audio
    moss_tts
    moss_tts_local
    moss_transcribe
    moss_transcribe_diarize
    miotts
    ${FFMPEG_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    $<$<BOOL:${WIN32}>:winhttp>
    # Winsock + crypto for httplib.h (transitively pulled in by
    # stelnettts_server.cpp). MSVC auto-links these via #pragma comment
    # but MinGW (msys2 UCRT64 / CLANG64) ignores those pragmas, so the
    # link fails with __imp_freeaddrinfo / WSASocketW / etc. undefined.
    # Listing them unconditionally is harmless on MSVC.
    $<$<BOOL:${WIN32}>:ws2_32>
    $<$<BOOL:${WIN32}>:crypt32>
    $<$<BOOL:${WIN32}>:bcrypt>
    $<$<BOOL:${WIN32}>:iphlpapi>
    $<$<BOOL:${WIN32}>:shell32>
    $<$<BOOL:${STELNETTTS_CURL}>:CURL::libcurl>
)

if (STELNETTTS_CURL)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_USE_LIBCURL)
endif()

# Optional outetts backend (source files may not be present in all checkouts)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stelnettts_backend_outetts.cpp")
    target_sources(${TARGET} PRIVATE stelnettts_backend_outetts.cpp)
    target_link_libraries(${TARGET} PRIVATE outetts)
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_OUTETTS=1)
endif()

# Optional parler-tts backend
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stelnettts_backend_parler.cpp")
    target_link_libraries(${TARGET} PRIVATE parler-tts)
endif()

# MP3 output — in-tree glint encoder (glint/), always available.
target_link_libraries(${TARGET} PRIVATE glint)

# Optional libmp3lame fallback (used if glint fails, or forced with
# STELNETTTS_MP3_ENCODER=lame for A/B comparisons)
find_library(LAME_LIBRARY NAMES mp3lame)
find_path(LAME_INCLUDE_DIR lame/lame.h)
if (LAME_LIBRARY AND LAME_INCLUDE_DIR)
    target_link_libraries(${TARGET} PRIVATE ${LAME_LIBRARY})
    target_include_directories(${TARGET} PRIVATE ${LAME_INCLUDE_DIR})
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_LAME=1)
    message(STATUS "MP3 output: glint + libmp3lame fallback")
else()
    message(STATUS "MP3 output: glint (libmp3lame fallback not found)")
endif()

# Optional Opus output (libopus + libogg for OGG container)
find_library(OPUS_LIBRARY NAMES opus)
find_path(OPUS_INCLUDE_DIR opus/opus.h)
if (OPUS_LIBRARY AND OPUS_INCLUDE_DIR)
    target_link_libraries(${TARGET} PRIVATE ${OPUS_LIBRARY})
    target_include_directories(${TARGET} PRIVATE ${OPUS_INCLUDE_DIR})
    target_compile_definitions(${TARGET} PRIVATE STELNETTTS_HAVE_OPUS=1)
    message(STATUS "Opus output enabled (libopus)")
else()
    message(STATUS "Opus output disabled (libopus-dev not found)")
endif()

# C2PA (Content Credentials) signing — shared logic in cmake/StelnetAsrC2pa.cmake.
stelnettts_enable_c2pa(${TARGET})

install(TARGETS ${TARGET} RUNTIME BUNDLE DESTINATION bin)

# ---------------------------------------------------------------------------
# stelnettts-chat — minimal reference CLI for the stelnettts_chat_* C ABI.
#
# Pure stdin → stdout chat loop. Streaming by default (one-shot when
# stdin is a pipe). Demonstrates the smallest downstream-binding shape
# and doubles as a smoke test for the chat surface against any GGUF
# chat model on disk. No SDL2 dep — text-only.
# ---------------------------------------------------------------------------
add_executable(stelnettts-chat
    stelnettts_chat_main.cpp
)
target_include_directories(stelnettts-chat PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(stelnettts-chat PRIVATE
    stelnettts-lib
    ${CMAKE_THREAD_LIBS_INIT}
)
include(DefaultTargetOptions)
install(TARGETS stelnettts-chat RUNTIME BUNDLE DESTINATION bin)

add_executable(stelnettts-f0-eval stelnettts_f0_eval.cpp)
target_link_libraries(stelnettts-f0-eval PRIVATE stelnettts-lib crepe beatrice-pitch)
target_include_directories(stelnettts-f0-eval PRIVATE ${CMAKE_SOURCE_DIR}/src)
