# Issue #265 — Consistency and documentation

Reporter (@bakamomi) asked for three things: (1) unify the reference-voice cache
location across backends, (2) standardize env-var naming, (3) a single doc that
lists/explains all env vars.

## NOW — active work

Branch `feat/issue-265-consistency-docs` (worktree
`.claude/worktrees/issue-265-consistency-docs`).

- **Item 1 — cache location: DONE.** OmniVoice's bespoke `~/.cache/stelnettts`
  code cache migrated onto the shared `stelnettts_ref_cache`
  (`src/core/tts_ref_cache.h`) — same `<TMPDIR>/stelnettts-tts-refcache` dir and
  same `STELNETTTS_TTS_REF_CACHE=0` disable switch as irodori/f5/openvoice2/
  indextts. Added content-addressed `get_bytes`/`put_bytes` helpers (OmniVoice
  caches int32 RVQ codes, not floats). Legacy `STELNETTTS_OMNIVOICE_VOICE_CACHE=0`
  kept as an alias. Same content-address key (preprocessed pcm + encoder
  fingerprint). `src/omnivoice.cpp`, `src/core/tts_ref_cache.h`.
- **Item 2 — env-var naming: CODE DONE, build pending.** Chosen scope = FULL
  rename (user decision). New helper `src/core/stelnettts_env.h`
  (`stelnettts_env::get/truthy/present`): looks up the canonical `STELNETTTS_`
  name, auto-falls-back to the prefix-stripped legacy name, and prints a
  one-time stderr deprecation warning (silence with
  `STELNETTTS_SUPPRESS_ENV_DEPRECATION=1`). Mechanically renamed every bare
  app-owned var in `src/` + `examples/` (main build) to
  `STELNETTTS_<BACKEND>_<FEATURE>` via `scratchpad/rename_env.py` (492 subs across
  94 files); routed the 5 wrapper helpers (env_bool/env_str/… in kokoro,
  cielvox2_tts, omnivoice, voxtral_tts, voxcpm2) through the helper. Fixed the
  pre-existing double-prefix bug `STELNETTTS_STELNETTTS_NEMOTRON_NO_WINDOW_MASK`.
  EXCLUDED (own conventions / synced siblings / vendored): external OS vars
  (HOME/TMPDIR/HF_TOKEN/CUDA_VISIBLE_DEVICES/GGML_VK_VISIBLE_DEVICES/LLAMA_*),
  `crisp_audio` (`CRISP_AUDIO_*`), `glint` (`GLINT_*`), the
  `crisp_lid/punc/truecase` mirror files, `ggml/`, `third_party/`,
  `examples/talk-llama/`, and Kaggle refsrc snapshots.
- **Item 3 — docs: DONE.** `docs/environment-variables.md` (convention, legacy
  aliases + deprecation, suffix legend, global tables, unified ref-voice cache,
  non-owned/sibling vars, full per-backend appendix). Linked from README.

All three items COMPLETE and verified. Committed on the branch (`feat/issue-265-consistency-docs`).

### Next
1. ff `feat/issue-265-consistency-docs` into `main` and push (pending user OK —
   96-file outward-facing change).

## Verification done
- **Build:** clean `cmake --build build -j6` — exit 0, 601 targets linked
  (fixed 12 ASR-family files whose mid-file `core/` include block placed the new
  include after first use).
- **Unit tests:** `ctest -L unit` → 975/975 passed, 0 failed.
- **Helper runtime behavior:** legacy bare name resolves via alias; one-time
  stderr deprecation warning; canonical name takes precedence; unset → nullptr.
- Static audit: no leftover bare app-owned getenv/wrapper calls in scope; all
  touched files include the helper; no non-const `char*` assignments from
  `get()`; external vars preserved; only intended lines changed (no format noise).
