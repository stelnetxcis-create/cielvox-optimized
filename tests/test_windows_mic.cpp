// test_windows_mic.cpp — regression tests for Windows microphone command construction.

#include "stelnettts_mic_cli.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Windows dshow mic arg uses the provided device name", "[windows][mic]") {
#if defined(_WIN32)
    REQUIRE(stelnettts_windows_dshow_audio_arg_from_name("Microphone") == "audio=\"Microphone\"");
    REQUIRE(stelnettts_windows_dshow_audio_arg_from_name(nullptr) == "audio=\"default\"");
    REQUIRE(stelnettts_windows_dshow_audio_arg_from_name("Stereo \"Mix\"") == "audio=\"Stereo 'Mix'\"");
#else
    SUCCEED("Windows-only helper");
#endif
}
