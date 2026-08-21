// test-cielvox2asr-params.cpp — unit tests for cielvox2_asr_context_params defaults
// and null-guard coverage. No GGUF required.

#include <catch2/catch_test_macros.hpp>
#include "cielvox2_asr.h"

TEST_CASE("cielvox2_asr_params: default values are sensible", "[unit][cielvox2_asr]") {
    struct cielvox2_asr_context_params p = cielvox2_asr_context_default_params();

    REQUIRE(p.n_threads >= 1);
    REQUIRE(p.verbosity >= 0);
}

// Defaults-audit / config-parity guard (motivated by #192/#197 + PLAN #89).
// flash_attn must stay on by default; a silent removal (the #89 regression class,
// which chatterbox guards the same way) fails CI here.
TEST_CASE("cielvox2_asr_params: gpu/flash defaults are pinned", "[unit][cielvox2_asr]") {
    struct cielvox2_asr_context_params p = cielvox2_asr_context_default_params();
    REQUIRE(p.use_gpu == true);
    REQUIRE(p.flash_attn == true);
}

TEST_CASE("cielvox2_asr_init_from_file: null path returns nullptr", "[unit][cielvox2_asr]") {
    struct cielvox2_asr_context_params p = cielvox2_asr_context_default_params();
    struct cielvox2_asr_context* ctx = cielvox2_asr_init_from_file(nullptr, p);
    REQUIRE(ctx == nullptr);
}

TEST_CASE("cielvox2_asr_init_from_file: empty path returns nullptr", "[unit][cielvox2_asr]") {
    struct cielvox2_asr_context_params p = cielvox2_asr_context_default_params();
    struct cielvox2_asr_context* ctx = cielvox2_asr_init_from_file("", p);
    REQUIRE(ctx == nullptr);
}

TEST_CASE("cielvox2_asr_free: NULL context is a no-op", "[unit][cielvox2_asr]") {
    cielvox2_asr_free(nullptr);
    SUCCEED("cielvox2_asr_free tolerated a NULL ctx.");
}
