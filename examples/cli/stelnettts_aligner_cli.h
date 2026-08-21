// stelnettts_aligner_cli.h — CLI-side aligner shim.
//
// The dispatch + inference for both canary-ctc and cielvox2-forced-aligner
// now lives in `src/stelnettts_aligner.{h,cpp}`. This header keeps the
// thin CLI adapter that returns results as `stelnettts_word` (the CLI
// type) instead of the library's `CrispasrAlignedWord`.

#pragma once

#include "stelnettts_backend.h"

#include <string>
#include <vector>

// `out_load_failed` (optional): set true iff the aligner MODEL failed to load
// (vs. loaded but produced no words). Issue #311 uses it to fail an explicitly
// required aligner even when native word timestamps would mask the failure.
std::vector<stelnettts_word> stelnettts_ctc_align(const std::string& aligner_model, const std::string& transcript,
                                              const float* samples, int n_samples, int64_t t_offset_cs, int n_threads,
                                              bool* out_load_failed = nullptr);
