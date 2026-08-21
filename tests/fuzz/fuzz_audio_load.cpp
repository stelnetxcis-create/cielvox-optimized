// fuzz_audio_load.cpp — libFuzzer harness over stelnettts_audio_load.
//
// stelnettts_audio_load() runs an attacker-controllable file through the full
// decoder dispatch (miniaudio WAV/MP3/FLAC/OGG, then the hand-rolled Sun-AU,
// AMR, WebM/EBML and MP4 fallbacks). Those hand-rolled parsers are the audio
// attack surface, so we fuzz the one entry point that reaches all of them.
//
// Build + run (requires clang):
//   cmake -B build-fuzz -DSTELNETTTS_FUZZ=ON -DSTELNETTTS_SANITIZE_ADDRESS=ON \
//         -DCMAKE_BUILD_TYPE=Debug
//   cmake --build build-fuzz --target stelnettts-fuzz-audio
//   ./build-fuzz/bin/stelnettts-fuzz-audio -max_len=65536 tests/fuzz/corpus
//
// The API is path-based, so each input is written to a fixed scratch file
// (libFuzzer drives one input per process iteration, single-threaded).

#include <cstddef>
#include <cstdint>
#include <cstdio>

extern "C" int stelnettts_audio_load(const char* path, float** out_pcm, int* out_samples, int* out_sample_rate);
extern "C" void stelnettts_audio_free(float* pcm);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Cap size so a pathological length field can't turn the fuzzer itself into
    // the DoS we are testing for (the parsers have their own 500 MB caps).
    if (size > 8u * 1024u * 1024u)
        return 0;

    const char* path = "stelnettts_fuzz_input.bin";
    FILE* f = std::fopen(path, "wb");
    if (!f)
        return 0;
    if (size)
        std::fwrite(data, 1, size, f);
    std::fclose(f);

    float* pcm = nullptr;
    int n_samples = 0, sample_rate = 0;
    if (stelnettts_audio_load(path, &pcm, &n_samples, &sample_rate) == 0)
        stelnettts_audio_free(pcm);

    return 0;
}
