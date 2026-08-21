# Environment variables

StelnetTTS exposes a large number of environment variables — tuning knobs, device
placement overrides, benchmark timers, and debug/dump switches. This page is the
single reference for all of them: the naming convention, the cross-cutting
variables, the per-backend variables, and the ones StelnetTTS deliberately does
**not** own.

> This document was introduced for issue #265 ("consistency and documentation").
> The same change standardized every backend-owned variable onto one prefix and
> unified the reference-voice cache location (see below).

## Naming convention

Every StelnetTTS-owned variable is named:

```
STELNETTTS_<BACKEND>_<FEATURE>
```

for example `STELNETTTS_OMNIVOICE_CODEC_GPU`, `STELNETTTS_IRODORI_CODEC_GPU`,
`STELNETTTS_PARAKEET_BENCH`. Process-global variables that are not tied to one
backend drop the `<BACKEND>` segment (`STELNETTTS_MODELS_DIR`, `STELNETTTS_KV_ON_CPU`).

### Legacy aliases and deprecation warnings

Historically many backends used a bare prefix (`OMNIVOICE_CODEC_GPU`,
`CIELVOX2_TTS_BENCH`, `CHATTERBOX_DEBUG`, …). Those bare names **still work** as
legacy aliases so existing scripts, notebooks, and Kaggle A/B kernels keep
running. The alias is derived automatically by stripping the `STELNETTTS_` prefix,
so for any `STELNETTTS_FOO_BAR` documented here the bare `FOO_BAR` is also honored.

The first time a run reads a value from a **legacy** name, StelnetTTS prints a
one-time notice to stderr:

```
[stelnettts] warning: environment variable 'OMNIVOICE_CODEC_GPU' is deprecated and
will be removed in a future release; use 'STELNETTTS_OMNIVOICE_CODEC_GPU' instead
(the old name still works for now). ...
```

Migrate to the canonical `STELNETTTS_`-prefixed name at your convenience. To
silence the warnings (e.g. for a pipeline that scrapes stderr and cannot migrate
yet), set:

```
STELNETTTS_SUPPRESS_ENV_DEPRECATION=1
```

The lookup + aliasing is implemented once in `src/core/stelnettts_env.h`
(`stelnettts_env::get / truthy / present`); new backends should read env vars
through that helper with a canonical `STELNETTTS_`-prefixed name.

### Value/truthiness conventions

Most variables are read with one of two conventions — check the surrounding
suffix legend, but as a rule:

- **Flag (opt-in)** — set to `1` (or any non-empty, non-`0` value) to enable.
  Some legacy flags treat *being set at all* as on, so prefer `=1` / unset it
  entirely rather than `=0` when a flag's exact semantics matter.
- **Path / value** — the variable's value is a filesystem path, integer, or
  float used directly.

### Common suffix legend

The same feature suffixes recur across nearly every backend. Rather than repeat
them per backend below, they mean:

| Suffix | Meaning |
|--------|---------|
| `_BENCH` | Print per-stage wall-clock timings for that engine. |
| `_DEBUG` / `_DIAG` | Verbose debug logging. |
| `_DUMP_DIR` / `_DUMP` / `_DUMP_*` | Write intermediate tensors / stages to disk (diagnostics, diff-harness). |
| `_FORCE_SCALAR` | Disable SIMD kernels (numeric-parity debugging). |
| `_GPU` / `_USE_GPU` / `_FORCE_METAL` | Force that stage/codec onto the GPU. |
| `_CPU` / `_CPU_ONLY` / `_FORCE_CPU` | Force that stage/codec onto the CPU. |
| `_NO_FA` / `_FLASH_ATTN` | Toggle flash-attention. |
| `_FASTCONV` (`_DEBUG`) | Enable the baked-F32 / matmul conv fast path for a codec/vocoder. |
| `_CFG_INTERVAL` (`_DEBUG`) | Interval-CFG cadence for a flow-matching decoder. |
| `_SEED` | RNG seed for reproducible generation. |
| `_*_FILE` / `_*_PATH` / `_*_GGUF` | Override an asset/model/fixture path (mostly dev/test). |

## Global / cross-cutting variables

These are not tied to a single backend.

### Paths, cache, and models

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_MODELS_DIR` | Directory searched for GGUF models (also the auto-download target root). |
| `STELNETTTS_CACHE_DIR` | Base cache directory for auto-downloaded models/assets (default `~/.cache/stelnettts`). |
| `STELNETTTS_SCRATCH_DIR` | Scratch directory for temporary run artifacts. |
| `STELNETTTS_DUMP_DIR` | Global tensor-dump directory (diagnostics). |
| `STELNETTTS_GGUF_MMAP` / `STELNETTTS_GGUF_PRELOAD` | Control GGUF mmap vs. preload-into-RAM loading. |
| `STELNETTTS_MLOCK` | mlock model weights into RAM. |

### GPU / device placement

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_N_GPU_LAYERS` | Number of transformer layers to offload to the GPU. |
| `STELNETTTS_ARG_DEVICE` | Default device selection for the CLI. |
| `STELNETTTS_KV_ON_CPU` | Keep the KV cache on the CPU. |
| `STELNETTTS_KV_QUANT` / `_KV_QUANT_K` / `_KV_QUANT_V` / `_KV_READ_F32` | KV-cache quantization / read format. |

> Device *selection* across compiled backends also honors the standard ggml /
> CUDA variables `CUDA_VISIBLE_DEVICES` and `GGML_VK_VISIBLE_DEVICES` — see
> "Variables StelnetTTS does not own" below.

### Session / long-audio chunking

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_SESSION_AUTOCHUNK` | Enable auto-chunking in the session API for long audio. |
| `STELNETTTS_SESSION_CHUNK_SECONDS` | Chunk length (seconds) for session auto-chunking. |
| `STELNETTTS_SESSION_PERBACKEND_CHUNK` | Use per-backend chunk-window tuning instead of a flat window. |
| `STELNETTTS_SESSION_UNIFIED_DISPATCH` | Route surfaces through the unified library dispatch path. |

### Post-decode hygiene (PLAN.md §W2–W7)

All OFF unless set. Each of these can delete or alter user-visible text, so
none of them switches on by surprise; a wrong deletion is worse than a
surviving artifact. Applied on both the CLI and the session C-ABI.

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_SEG_MAX_CHARS` | Truncate any segment longer than N **code points**, backing up to the last `。．.！!？?、,` but never below 75% of the cap. A line past the cap is almost always a repetition hallucination that survived the n-gram collapse. |
| `STELNETTTS_SEG_DROP_NONVERBAL` | Drop segments that are entirely a non-verbal marker — `[Music]`, `(applause)`, `（喘ぎ声）`, `♪`. Running speech merely *containing* such a word is never dropped. |
| `STELNETTTS_SEG_LOGPROB_THOLD` | Drop segments whose average log-probability is below this. Post-hoc, on top of the decoder's own fallback gate. |
| `STELNETTTS_SEG_LOGPROB_MARGIN` | Loosen that threshold by this much for segments ≤1.6 s — a short segment's mean logprob is noisier, so it gets more room, not less. |
| `STELNETTTS_SEG_MERGE_REPEATS` | Collapse runs of near-identical adjacent segments into one spanning the whole run. Catches a phrase repeating *across* segment boundaries, which per-segment loop fixes cannot see. |
| `STELNETTTS_SEG_MERGE_SIMILARITY` | Similarity bar for the above (default 0.90; LCS over code points). |
| `STELNETTTS_SEG_MERGE_GAP_CS` | Never merge across a gap wider than this many centiseconds (default 200). Two identical lines a minute apart are two real utterances. |
| `STELNETTTS_SEG_MERGE_MIN_RUN` | Minimum consecutive similar segments before merging (default 3), so an ordinary repeated "yes." pair survives. |

### Alignment and VAD sanity checks

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_ALIGN_SENTINEL` | `0` disables the forced-alignment collapse check. On by default, **detect + warn only**. Catches `ctc_forced_align()` returning words at `t0 == t1 == 0` — its two silent-zero paths (characters absent from the CTC vocab, or a word the Viterbi path never visited) produce garbage timestamps inside a *successful* return. |
| `STELNETTTS_ALIGN_SENTINEL_REDISTRIBUTE` | `1` opts into repair: respace the words across the clip in proportion to character count. Off by default — a wrong auto-repair would be just as invisible as the collapse. |
| `STELNETTTS_VAD_FAILOVER` | `0` disables the VAD sanity check. On by default: if a clip over 120 s comes back with under 1% speech coverage (or a couple of segments covering under 10% of a very long clip), the VAD is wrong and the run falls back to fixed full-clip chunks rather than losing the transcript. |
| `STELNETTTS_NGRAM_LOOPFIX_OFF` | `1` disables the repeated-n-gram collapse entirely, exposing the RAW decoded text. Diagnostic: for telling whether a loop originates in the decode itself or is merely being masked. |

### Decoding / beam search (shared)

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_MAES_BETA` / `_MAES_GAMMA` / `_MAES_NUM_STEPS` | MAES beam-search parameters. |
| `STELNETTTS_TDT_BATCH` / `STELNETTTS_RNNT_BATCH` | Batch the TDT / RNNT joint decode. |
| `STELNETTTS_RNNT_GGML_PERSTEP` | Per-step (vs. persistent-graph) ggml RNNT decode. |
| `STELNETTTS_NGRAM_LOOPFIX_OFF` | Disable the n-gram decode-loop breaker. |
| `STELNETTTS_GAP_FILL` / `_GAP_FILL_MIN_CS` | Re-transcribe spans a first pass left empty (long audio); on by default for parakeet, threshold non-JA 300 cs / JA 100 cs. |

### G2P / phonemizer

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_CMUDICT_PATH` | Path to the CMUdict pronunciation dictionary. |
| `STELNETTTS_DE_DICT_PATH` / `_FR_DICT_PATH` / `_ES_DICT_PATH` | Language-specific pronunciation dictionaries. |
| `STELNETTTS_G2P_DICT_SOURCE` / `_G2P_MODEL_PATH` | G2P dictionary source / neural G2P model path. |
| `STELNETTTS_ESPEAK_DATA_PATH` | eSpeak-NG data directory. |
| `STELNETTTS_KOKORO_G2P` | Kokoro G2P backend selection. |
| `STELNETTTS_KOKORO_MISAKI_IPA` | `0` disables the espeak-IPA → misaki-alphabet conversion Kokoro needs (#316), restoring the raw G2P spelling for A/B. On by default. |
| `STELNETTTS_G2P_DE_UNSTRESS` | `1` reads the German closed class the way espeak reads it in a SENTENCE (`sie` → `ziː`) instead of the citation form our per-word dictionary stores (`zˈiː`). Off by default: it takes phoneme agreement with espeak from 45.9% to 87.1%, but the ASR round-trip could not resolve a difference, and that metric measures intelligibility rather than naturalness (#316). |
| `STELNETTTS_KOKORO_DE_MISAKI_ALPHABET` | `1` applies misaki's tied-sequence collapse for German (`tsvˈaɪ` → `ʦvˈI`), which is what the published training recipe does. Off by default: it made the ASR round-trip worse on the `kokoro-de-hui-base` model we ship, which appears to predate that part of the recipe (#316). The `ʏ`→`y` vocabulary fixup is applied either way. |
| `STELNETTTS_T5_REPEAT_BREAK` | `0` disables the decode-loop break for madlad/T5 translation, restoring exact PyTorch-blueprint behaviour. On by default: MADLAD greedy-decodes into a repeated token cycle on some short inputs and burns the whole token budget on it. The blueprint does the same — this is a deliberate improvement on it, not a parity fix (#333). |
| `STELNETTTS_T5_NO_KV_REUSE` | `1` re-forwards the whole decoder prefix each step instead of appending to the KV cache. Same output, much slower; an A/B lever for isolating cache bugs (#333). |
| `STELNETTTS_T5_KEEP_EMBED` | `1` keeps `shared.embed.*` and `lm_head.*` at source precision when quantizing a T5 model. **Off by default because it was measured and loses**: on madlad400 it makes q8_0 3.38→3.62 GB and q4_k 2.04→2.41 GB for a worst-stage cosine that does not improve (0.999922→0.999920, 0.992929→0.992606). The Q4_K loss accumulates through the 32 encoder blocks, not in the embedding lookup (#333). |
| `STELNETTTS_KOKORO_PUNCT` | `0` drops punctuation from the phoneme string for the German/French/Spanish built-in G2Ps, restoring pre-0.8.26 behaviour for A/B. On by default: Kokoro's vocabulary contains `,.;:!?` and they are how it pauses (#316). English is not gated — it is settled against misaki. |

### Watermark / provenance

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_NO_WATERMARK` | Disable the audio watermark. |
| `STELNETTTS_WATERMARK_LEGACY` | Use the legacy watermark path. |
| `STELNETTTS_NO_C2PA_REMUX` | Skip the C2PA MP4 remux step. |

### Quantization / diff-harness / misc

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_QUANT_LMHEAD` | Quantize the LM head during `stelnettts-quantize`. |
| `STELNETTTS_IMATRIX_OUT` | Importance-matrix output path. |
| `STELNETTTS_ACTDUMP_OUT` / `_ACTDUMP_TENSOR` | Activation dump output / target tensor. |
| `STELNETTTS_DIFF_NO_GPU` / `_DIFF_USE_GPU` / `_DIFF_SLICES` / `_DIFF_STAGES` | `stelnettts-diff` harness controls. |
| `STELNETTTS_MEL_PARALLEL` / `_MEL_TIMING` | Parallelize / time mel-spectrogram computation. |
| `STELNETTTS_VERBOSE` | Global verbose output. |
| `STELNETTTS_NO_WARMUP` / `STELNETTTS_WARMUP` | Skip / force the model warmup pass. |
| `STELNETTTS_SERVER_WORKERS` / `STELNETTTS_API_KEYS` | HTTP server worker count / API keys. |

## Reference-voice cache (voice cloning)

Encoding a reference clip for voice cloning (a codec encoder, a Conformer /
Perceiver, or an ASR pass) is slow and produces a small, reusable blob. Every
voice-cloning TTS backend caches that blob through **one shared mechanism**
(`src/core/tts_ref_cache.h`), so the location and disable switch are identical
across backends (issue #265 unified OmniVoice — which previously used a bespoke
`~/.cache/stelnettts` cache — onto this path):

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_TTS_REF_CACHE` | Set to `0` to disable reference-voice caching everywhere. |
| `STELNETTTS_TTS_REF_CACHE_DIR` | Override the cache directory (default `<TMPDIR>/stelnettts-tts-refcache`). |

Content-addressed entries are keyed by a hash of the raw reference (plus, for
some backends, an encoder-weight fingerprint) and tagged per backend
(`irodori-latent`, `openvoice2-se`, `omnivoice-voice`, `f5-reftext`, …) so no two
backends read each other's blob. OmniVoice additionally honors the legacy
`STELNETTTS_OMNIVOICE_VOICE_CACHE=0` as an alias for the shared disable switch.

## Variables StelnetTTS does not own

These are read by StelnetTTS but are **OS / third-party conventions** and are
intentionally *not* renamed to the `STELNETTTS_` prefix:

| Variable | Origin |
|----------|--------|
| `HOME`, `USERPROFILE`, `LOCALAPPDATA`, `XDG_CACHE_HOME` | OS home / cache dirs. |
| `TMPDIR`, `TEMP`, `TMP` | OS temp dir (also the default ref-cache root). |
| `HF_TOKEN`, `HUGGING_FACE_HUB_TOKEN` | HuggingFace auth (Hub convention). |
| `CUDA_VISIBLE_DEVICES` | CUDA device selection. |
| `GGML_VK_VISIBLE_DEVICES` | ggml Vulkan device selection. |
| `LLAMA_*` | Vendored `llama.cpp` (talk-llama example). |

### Sibling modular libraries (own conventions)

The in-tree modular libraries keep their own established prefixes and are synced
from their sibling repos, so their variables are **not** part of the `STELNETTTS_`
scheme:

- **`crisp_audio/`** — audio tower: `CRISP_AUDIO_DUMP_STAGES`, `CRISP_AUDIO_KEEP_PAD_FRAMES`, `CRISP_AUDIO_WINDOWED_ATTN`.
- **`glint/`** — clean-room MP3/AAC codec: `GLINT_*`, `AACDBG`.
- **`crisp_lid/` · `crisp_punc/` · `crisp_truecase/`** — standalone LID / punctuation / truecasing libraries; they and their `src/` counterparts follow each library's own naming (`LID_*`, `FIREREDPUNC_*`, `PCS_*`, `TRUECASER_*`). `FIREREDPUNC_DEBUG=1` prints each restore pass as `[PUNCDBG] in=<…>` / `out=<…>`, which is the quickest way to see a backend's *true* model output — `--no-punctuation` is not, because it strips punctuation after the fact and so hides text the model punctuated itself.
  > ⚠ **These libraries are built by the main target, and each has a second copy under `src/`.** `src/CMakeLists.txt` prefers `crisp_punc/` (etc.) and falls back to the `src/` copy only when the sibling directory is missing from a checkout — so the `crisp_punc/` copy is what normally links, and **a change must be applied to both**. #308's capitalisation fix went into `src/fireredpunc.cpp` alone and was dead code for months while the shipping copy kept the bug. `tests/test-punc-copies-in-sync.cpp` now fails if they diverge.

## Test fixtures

The live/integration test suite reads a few `STELNETTTS_`-prefixed fixture
variables (model/audio paths); `tests/env-live-tests.sh` sets sensible defaults.
Like every other variable here, the pre-standardization bare names are still
honored as deprecated aliases:

| Variable | Purpose |
|----------|---------|
| `STELNETTTS_PARAFORMER_MODEL` / `_PARAFORMER_MODEL_Q4K` / `_PARAFORMER_AUDIO_ZH` / `_PARAFORMER_AUDIO_EN` | Paraformer test model / audio paths. |
| `STELNETTTS_AUDIOSEAL_GGUF` | AudioSeal test model path. |
| `STELNETTTS_PIPER_TEST_MODEL` | Piper phonemize test model path. |
| `STELNETTTS_OV2_DUMP_DIR` | OpenVoice2 HiFi test dump directory. |
| `STELNETTTS_MODEL_BTC_CHORDS` | BTC chord-recognition test model path. Defaults to `$STELNETTTS_MODELS_DIR/btc-chords-large-f32.gguf`. |

---

# Per-backend variables

Every variable below is the canonical `STELNETTTS_`-prefixed name; the bare form
(without `STELNETTTS_`) is accepted as a deprecated alias. See the **suffix
legend** above for the meaning of the common `_BENCH` / `_DEBUG` / `_DUMP_*` /
`_GPU` / `_CPU` / `_FORCE_SCALAR` / `_FASTCONV` / `_CFG_INTERVAL` / `_SEED`
suffixes.

### AAC codec

- `STELNETTTS_AAC_DEBUG`
- `STELNETTTS_AAC_DECODER`

### ARK-ASR

- `STELNETTTS_ARKASR_CPU`
- `STELNETTTS_ARKASR_DEBUG_GEN`
- `STELNETTTS_ARKASR_GPU`
- `STELNETTTS_ARKASR_MAX_SINGLE_PASS_S`
- `STELNETTTS_ARKASR_NO_CHUNK_CONTEXT`
- `STELNETTTS_ARKASR_NO_EOS_SUPPRESS`
- `STELNETTTS_ARKASR_TIMING`

### AudioSeal watermark

- `STELNETTTS_AUDIOSEAL_BENCH`
- `STELNETTTS_AUDIOSEAL_DEBUG`
- `STELNETTTS_AUDIOSEAL_DUMP_STAGES`

### BananaMind TTS

- `STELNETTTS_BANANAMIND_DEBUG`
- `STELNETTTS_BANANAMIND_TTS_BENCH`

### Bark

- `STELNETTTS_BARK_BENCH`
- `STELNETTTS_BARK_DECODE_CODES`
- `STELNETTTS_BARK_DUMP_DIR`

### BERT encoder

- `STELNETTTS_BERT_ENCODER_BENCH`

### BTC chord recognition

- `STELNETTTS_BTC_MAJ_MIN` — collapse the 170-class chord output to the 25-class
  maj/min vocabulary. Default off (full 170-class output): 170 reduces to
  maj/min at runtime, but a 25-class model can never be expanded, so the
  170-class checkpoint is the shipping default.

> The shipped BTC weights are **CC-BY-NC-SA** (trained on Isophonics / Robbie
> Williams / UsPop2002 chord annotations) even though the upstream BTC code and
> StelnetTTS itself are MIT. The registry refuses to download them without
> `--accept-license cc-by-nc-sa-4.0` (or `STELNETTTS_ACCEPT_LICENSE`). A
> commercial product must supply its own weights.

### Canary / Canary-CTC / Canary-Qwen

- `STELNETTTS_CANARY_BENCH`
- `STELNETTTS_CANARY_CTC_BENCH`
- `STELNETTTS_CANARY_QWEN_BENCH`
- `STELNETTTS_CANARY_QWEN_DEBUG`
- `STELNETTTS_CANARY_QWEN_MIN_ENC_FRAMES`
- `STELNETTTS_CANARY_QWEN_NO_ECHO_STRIP`
- `STELNETTTS_CANARY_STREAM_THRESHOLD_S`

### Chatterbox

- `STELNETTTS_CHATTERBOX_BENCH`
- `STELNETTTS_CHATTERBOX_BENCH_B2`
- `STELNETTTS_CHATTERBOX_DEBUG`
- `STELNETTTS_CHATTERBOX_DUMP_ATTN_AT`
- `STELNETTTS_CHATTERBOX_DUMP_FFN_AT`
- `STELNETTTS_CHATTERBOX_DUMP_GPT2_LAYERS`
- `STELNETTTS_CHATTERBOX_DUMP_KPROJ_AT`
- `STELNETTTS_CHATTERBOX_DUMP_KROPE_AT`
- `STELNETTTS_CHATTERBOX_DUMP_KV_AT`
- `STELNETTTS_CHATTERBOX_DUMP_KV_LAYER`
- `STELNETTTS_CHATTERBOX_DUMP_LAYER`
- `STELNETTTS_CHATTERBOX_DUMP_LOGITS_AT`
- `STELNETTTS_CHATTERBOX_DUMP_NORM_AT`
- `STELNETTTS_CHATTERBOX_DUMP_QPROJ_AT`
- `STELNETTTS_CHATTERBOX_DUMP_VPROJ_AT`
- `STELNETTTS_CHATTERBOX_DUMP_WK`
- `STELNETTTS_CHATTERBOX_LANG`
- `STELNETTTS_CHATTERBOX_NAIVE_ATTN`
- `STELNETTTS_CHATTERBOX_SEED`
- `STELNETTTS_CHATTERBOX_SYN_TEXT`
- `STELNETTTS_CHATTERBOX_T3_BUCKET_REUSE`
- `STELNETTTS_CHATTERBOX_T3_CFG_B2`
- `STELNETTTS_CHATTERBOX_T3_CFG_BUCKET`
- `STELNETTTS_CHATTERBOX_T3_SEED`
- `STELNETTTS_CHATTERBOX_TEMP`
- `STELNETTTS_CHATTERBOX_THREADS`

### Chatterbox S3Gen

- `STELNETTTS_S3GEN_CFG_INTERVAL`
- `STELNETTTS_S3GEN_DUMP`
- `STELNETTTS_S3GEN_DUMP_UNET`
- `STELNETTTS_S3GEN_DUMP_UNET_NO_AUTO_MARK`
- `STELNETTTS_S3GEN_FASTCONV`
- `STELNETTTS_S3GEN_FASTCONV_DEBUG`
- `STELNETTTS_S3GEN_RC_AS_MUL_MAT`
- `STELNETTTS_S3GEN_UNET_CFG_SINGLE`
- `STELNETTTS_S3GEN_UNET_CPU`
- `STELNETTTS_S3GEN_UNET_GALLOCR`
- `STELNETTTS_S3GEN_UNET_KEEP_GPU_OP`
- `STELNETTTS_S3GEN_UNET_MARK_DB_OUT`
- `STELNETTTS_S3GEN_UNET_MARK_DB_RESNET`
- `STELNETTTS_S3GEN_UNET_MARK_DB_TB`
- `STELNETTTS_S3GEN_UNET_MARK_MB_OUT`
- `STELNETTTS_S3GEN_UNET_MARK_MB_OUT_INDEX`
- `STELNETTTS_S3GEN_UNET_MARK_MB_OUT_MAX`
- `STELNETTTS_S3GEN_UNET_MARK_MB_RESNET`
- `STELNETTTS_S3GEN_UNET_PIN_CPU_OP`
- `STELNETTTS_S3GEN_UNET_PRESERVE_INTERMEDIATES`
- `STELNETTTS_S3GEN_UNET_PROBE_BLOCK1`
- `STELNETTTS_S3GEN_UNET_PROBE_DENOISER_OUT`
- `STELNETTTS_S3GEN_UNET_PROBE_INPUT_SNAPSHOT`
- `STELNETTTS_S3GEN_UNET_PROBE_RC_OUT`

### Chatterbox sub-modules

- `STELNETTTS_CB_CAMPPLUS_BENCH`
- `STELNETTTS_CB_S3GEN_BENCH`
- `STELNETTTS_CB_S3TOK_BENCH`
- `STELNETTTS_CB_VE_BENCH`

### Cohere

- `STELNETTTS_COHERE_BENCH`
- `STELNETTTS_COHERE_DEBUG`
- `STELNETTTS_COHERE_DEVICE`
- `STELNETTTS_COHERE_DUMP_ATTN`
- `STELNETTTS_COHERE_DUMP_ENCOUT`
- `STELNETTTS_COHERE_DUMP_MEL`
- `STELNETTTS_COHERE_DUMP_STAGES`
- `STELNETTTS_COHERE_FLASH`
- `STELNETTTS_COHERE_GAPS`
- `STELNETTTS_COHERE_LEGACY_SA`
- `STELNETTTS_COHERE_PROF`
- `STELNETTTS_COHERE_THREADS`

### CosyVoice3

- `STELNETTTS_COSYVOICE3_BENCH`
- `STELNETTTS_COSYVOICE3_CAMPPLUS_PATH`
- `STELNETTTS_COSYVOICE3_CFG_BATCH`
- `STELNETTTS_COSYVOICE3_CFG_INTERVAL`
- `STELNETTTS_COSYVOICE3_CFG_INTERVAL_DEBUG`
- `STELNETTTS_COSYVOICE3_DUMP_TOKENS`
- `STELNETTTS_COSYVOICE3_FASTCONV`
- `STELNETTTS_COSYVOICE3_FASTCONV_DEBUG`
- `STELNETTTS_COSYVOICE3_FLOW_STEPS`
- `STELNETTTS_COSYVOICE3_HIFT_PATH`
- `STELNETTTS_COSYVOICE3_KV_BUCKET`
- `STELNETTTS_COSYVOICE3_NO_CLONE_CACHE` — re-extract the `--voice ref.wav`
  speaker (s3tokenizer + CAMPPlus + prompt mel) on every synthesis instead of
  once per reference. Output-identical; the cached path is ~30% faster on a
  multi-sentence `--tts` (#334).
- `STELNETTTS_COSYVOICE3_NO_MIN_LEN` — drop the decode's minimum-length floor
  (2 speech tokens per target text token, upstream's `min_token_text_ratio`).
  Without the floor a single unlucky sample at step 0 ends the decode with no
  audio at all (#334).
- `STELNETTTS_COSYVOICE3_VOICES_PATH`

### CosyVoice3 (diff-harness assets)

- `STELNETTTS_CV3_FLOW_GGUF`
- `STELNETTTS_CV3_HIFT_GGUF`
- `STELNETTTS_CV3_S3TOK_GGUF`

### CSM TTS

- `STELNETTTS_CSM_BENCH`
- `STELNETTTS_CSM_WAV_FRAMES`
- `STELNETTTS_CSM_WAV_OUT`
- `STELNETTTS_CSM_WAV_TEMP`
- `STELNETTTS_CSM_WAV_TEXT`

### Dia TTS

- `STELNETTTS_DIA_BENCH`
- `STELNETTTS_DIA_DECODE_CODES`
- `STELNETTTS_DIA_DUMP_DIR`
- `STELNETTTS_DIA_DUMP_STEPLOGITS`
- `STELNETTTS_DIA_DUMP_TOKENS`
- `STELNETTTS_DIA_FORCE_TOKENS`
- `STELNETTTS_DIA_GREEDY`
- `STELNETTTS_DIA_MAX_STEPS`
- `STELNETTTS_DIA_TTS_GPU`

### dots.tts

- `STELNETTTS_DOTS_CFG_INTERVAL`
- `STELNETTTS_DOTS_CFG_INTERVAL_DEBUG`
- `STELNETTTS_DOTS_DIFF_GPU`
- `STELNETTTS_DOTS_DIT_DEBUG`
- `STELNETTTS_DOTS_EOS_THRESHOLD`
- `STELNETTTS_DOTS_FM_AB`
- `STELNETTTS_DOTS_FM_DUMP`
- `STELNETTTS_DOTS_FUSED_STEP`
- `STELNETTTS_DOTS_MAX_PATCHES`
- `STELNETTTS_DOTS_ODE_STEPS`
- `STELNETTTS_DOTS_PENC_VERIFY`
- `STELNETTTS_DOTS_TTS_BENCH`
- `STELNETTTS_DOTS_TTS_CPU`
- `STELNETTTS_DOTS_TTS_DEBUG`

### ECAPA (LID / speaker)

- `STELNETTTS_ECAPA_ASP_CPU`
- `STELNETTTS_ECAPA_ASP_GGML`
- `STELNETTTS_ECAPA_FORCE_SCALAR`
- `STELNETTTS_ECAPA_LID_BENCH`
- `STELNETTTS_ECAPA_REF_FBANK`
- `STELNETTTS_ECAPA_TIMING`

### F5-TTS

- `STELNETTTS_F5_BATCH_CFG`
- `STELNETTTS_F5_BENCH`
- `STELNETTTS_F5_CFG_INTERVAL`
- `STELNETTTS_F5_DURATION_CLAMP` — clamp the per-char speech rate into a sane English band so a reference whose audio/transcript lengths are mismatched can't truncate (or balloon) the output (#294). Default on; set `0` to restore the exact upstream `ref_T / ref_text_len * gen_text_len / speed` estimate.
- `STELNETTTS_F5_FORCE_SCALAR`
- `STELNETTTS_F5_REF_MAX_SEC` — clip the reference audio to this many seconds before it drives the duration estimate (upstream parity: 12 s). Default `12`; set `0` to disable the clip.
- `STELNETTTS_F5_REF_TRIM_SILENCE` — strip leading/trailing silence and collapse internal silences >~1 s in the reference audio (upstream parity). Default on; set `0` to disable.

### FastConformer (shared encoder)

- `STELNETTTS_FC_ATTN_CONT`
- `STELNETTTS_FC_BUCKET`
- `STELNETTTS_FC_FUSED_QKV`
- `STELNETTTS_FC_GPU_MANUAL_ATTN`
- `STELNETTTS_FC_MAX_LAYERS`
- `STELNETTTS_FC_MEM_DEBUG`
- `STELNETTTS_FC_NO_FLASH`
- `STELNETTTS_FC_PROFILE`
- `STELNETTTS_FC_PROF_FP`
- `STELNETTTS_FC_PROF_FP_COLS`
- `STELNETTTS_FC_PW_Q8`
- `STELNETTTS_FC_TILED_ATTN`
- `STELNETTTS_FC_TILED_BLOCK`
- `STELNETTTS_FC_WINDOWED_ATTN`
- `STELNETTTS_FC_WINDOW_BLOCK`

### FastPitch

- `STELNETTTS_FASTPITCH_BENCH`
- `STELNETTTS_FASTPITCH_DUMP_DIR`
- `STELNETTTS_FASTPITCH_FORCE_TOKENS`

### Ffmpeg

- `STELNETTTS_FFMPEG_LOG`

### FireRed ASR / VAD

- `STELNETTTS_FIRERED_BEAM_F32`
- `STELNETTTS_FIRERED_BENCH`
- `STELNETTTS_FIRERED_ENC_CPU`
- `STELNETTTS_FIRERED_GGML_ATTN`
- `STELNETTTS_FIRERED_LID_BENCH`
- `STELNETTTS_FIRERED_MATVEC_CACHE`
- `STELNETTTS_FIRERED_NO_REPEAT_BREAK`
- `STELNETTTS_FIRERED_VAD_BENCH`
- `STELNETTTS_FIRERED_VAD_DEBUG`
- `STELNETTTS_FIRERED_VAD_FORCE_SCALAR`

### FunASR / SenseVoice

- `STELNETTTS_FUNASR_BENCH`
- `STELNETTTS_FUNASR_DUMP_STAGES`
- `STELNETTTS_FUNASR_EMBED_FAST`
- `STELNETTTS_FUNASR_LLM_CPU`
- `STELNETTTS_FUNASR_LLM_LAYERS`
- `STELNETTTS_FUNASR_NAN_CHECK`
- `STELNETTTS_FUNASR_NO_FA`
- `STELNETTTS_FUNASR_STEP_CACHE`

### Gemma-4 E2B

- `STELNETTTS_GEMMA4_AUTO_CHUNK`
- `STELNETTTS_GEMMA4_E2B_BENCH`
- `STELNETTTS_GEMMA4_E2B_EMBED_FAST`

### GLM-ASR

- `STELNETTTS_GLM_ASR_BENCH`
- `STELNETTTS_GLM_ASR_DEBUG`
- `STELNETTTS_GLM_ASR_EMBED_FAST`
- `STELNETTTS_GLM_ASR_LEGACY_PROMPT`
- `STELNETTTS_GLM_ASR_SINGLE_WINDOW`

### Granite speech / NLE

- `STELNETTTS_GRANITE_BENCH`
- `STELNETTTS_GRANITE_DEC_GALLOCR`
- `STELNETTTS_GRANITE_DEC_PROFILE`
- `STELNETTTS_GRANITE_DISABLE_ENCODER_GRAPH`
- `STELNETTTS_GRANITE_ENC_F16`
- `STELNETTTS_GRANITE_FORCE_SCALAR`
- `STELNETTTS_GRANITE_NLE_BENCH`
- `STELNETTTS_GRANITE_NLE_EDIT_DUMP`
- `STELNETTTS_GRANITE_QUANT_ALL`

### HiFT vocoder

- `STELNETTTS_HIFT_FULL_IDFT`

### HTDemucs (source separation)

- `STELNETTTS_HTDEMUCS_BLAS` — route the CrossTransformer matmuls through
  `cblas_sgemm` (default **ON** where Accelerate is available). `=0` selects the
  scalar path. The transformer is ~86% of an unoptimised forward pass, so this is
  the dominant knob (measured 44x on the transformer, 4.6x overall).
- `STELNETTTS_HTDEMUCS_FASTCONV` — batched im2col + one GEMM for the CPU convs
  (default **ON**). `=0` selects the original per-time-frame scalar path.
  Measured `enc.conv2d` 10.0 s -> 0.17 s and `enc.rewrite` 12.2 s -> 0.30 s.
- `STELNETTTS_HTDEMUCS_WCACHE` — cache F32 copies of weight tensors by pointer
  (default **ON**). `=0` re-reads and re-converts on every access, which the
  DConv stacks do ~6k times per encoder layer.
- `STELNETTTS_HTDEMUCS_GGML` — run the CrossTransformer as a ggml graph instead of
  the CPU/BLAS path (default **OFF**). Verified correct on CPU and Metal (45/45
  stages, every layer cos 1.000000) but not yet proven faster overall, so it
  stays opt-in per the inverse-default rule.
- `STELNETTTS_HTDEMUCS_GPU` — request a GPU backend (CUDA > Metal > Vulkan, CPU
  fallback). Only meaningful together with `_GGML=1`: under the CPU/BLAS path
  the weights would sit on the device and every kernel would pay a read back.
- `STELNETTTS_HTDEMUCS_PROFILE` — print a per-phase wall-time breakdown of one
  forward pass (stft / enc / transformer / dec / istft).
- `STELNETTTS_HTDEMUCS_DEBUG` — verbose per-layer shape and NaN diagnostics.
- `STELNETTTS_HTDEMUCS_SKIP_TIME` — skip the time branch (bisection aid).

All three optimisation gates are output-equivalent: the per-stage diff reports
45/45 stages passing with them ON or OFF.

### Higgs STT

- `STELNETTTS_HIGGS_DEBUG`
- `STELNETTTS_HIGGS_STT_BENCH`
- `STELNETTTS_HIGGS_STT_EMBED_FAST`
- `STELNETTTS_HIGGS_STT_FUSED_QKV`

### IndexTTS

- `STELNETTTS_INDEXTTS_AA_BACKEND`
- `STELNETTTS_INDEXTTS_AA_SCALAR`
- `STELNETTTS_INDEXTTS_AUDIO24K_FILE`
- `STELNETTTS_INDEXTTS_BEAM_SIZE`
- `STELNETTTS_INDEXTTS_BENCH`
- `STELNETTTS_INDEXTTS_COND_FILE`
- `STELNETTTS_INDEXTTS_DEBUG`
- `STELNETTTS_INDEXTTS_KV_DEVICE_COPY`
- `STELNETTTS_INDEXTTS_LATENT_FILE`
- `STELNETTTS_INDEXTTS_MEL_CODES_FILE`
- `STELNETTTS_INDEXTTS_MEL_FILE`
- `STELNETTTS_INDEXTTS_SPK_NORM`
- `STELNETTTS_INDEXTTS_TEXT_NORMALIZER`
- `STELNETTTS_INDEXTTS_VOCODER_AA`
- `STELNETTTS_INDEXTTS_VOCODER_RAW`
- `STELNETTTS_INDEXTTS_VOC_BENCH`
- `STELNETTTS_INDEXTTS_VOC_FORCE_GPU`

### Irodori TTS

- `STELNETTTS_IRODORI_CAPTION`
- `STELNETTTS_IRODORI_CAPTION_TOKEN_IDS`
- `STELNETTTS_IRODORI_CFG_CAPTION`
- `STELNETTTS_IRODORI_CFG_INTERVAL`
- `STELNETTTS_IRODORI_CFG_INTERVAL_DEBUG`
- `STELNETTTS_IRODORI_CFG_SPEAKER`
- `STELNETTTS_IRODORI_CFG_TEXT`
- `STELNETTTS_IRODORI_CODEC_CPU`
- `STELNETTTS_IRODORI_CODEC_GPU`
- `STELNETTTS_IRODORI_CPU`
- `STELNETTTS_IRODORI_DEBUG`
- `STELNETTTS_IRODORI_DECODE_CHUNK`
- `STELNETTTS_IRODORI_DECODE_CTX`
- `STELNETTTS_IRODORI_DIT_TIMING`
- `STELNETTTS_IRODORI_DUMP_LATENT`
- `STELNETTTS_IRODORI_DUMP_TEXT_STATE`
- `STELNETTTS_IRODORI_DUMP_TOKENS`
- `STELNETTTS_IRODORI_DUMP_V_PRED0`
- `STELNETTTS_IRODORI_ENC_DUMP`
- `STELNETTTS_IRODORI_ENC_PRENORM`
- `STELNETTTS_IRODORI_FASTCONV`
- `STELNETTTS_IRODORI_LAYERS`
- `STELNETTTS_IRODORI_ODE_STEPS`
- `STELNETTTS_IRODORI_PERSIST_GRAPH`
- `STELNETTTS_IRODORI_REF_NOISE`
- `STELNETTTS_IRODORI_TOKEN_IDS`
- `STELNETTTS_IRODORI_T_LATENT`

### Kokoro

- `STELNETTTS_KOKORO_BENCH`
- `STELNETTTS_KOKORO_DEBUG`
- `STELNETTTS_KOKORO_DEBUG_INTERMEDIATES`
- `STELNETTTS_KOKORO_DUMP_STAGES`
- `STELNETTTS_KOKORO_FASTCONV`
- `STELNETTTS_KOKORO_FASTCONV_DEBUG`
- `STELNETTTS_KOKORO_G2P`
- `STELNETTTS_KOKORO_SEED`
- `STELNETTTS_KOKORO_USE_GPU`
- `STELNETTTS_KOKORO_VOICE_GGUF`

### KugelAudio

- `STELNETTTS_KUGELAUDIO_CPU_ONLY`
- `STELNETTTS_KUGELAUDIO_DEBUG`

### Kyutai STT

- `STELNETTTS_KYUTAI_RVQ_FAST`
- `STELNETTTS_KYUTAI_STT_BENCH`

### LFM2-Audio

- `STELNETTTS_LFM2_AUDIO_BENCH`
- `STELNETTTS_LFM2_AUDIO_CPU`
- `STELNETTTS_LFM2_SNAP_LAYERS`

### M2M-100 translate

- `STELNETTTS_M2M100_BENCH`
- `STELNETTTS_M2M100_GPU`

### MarbleNet VAD

- `STELNETTTS_MARBLENET_VAD_BENCH`

### MeloTTS

- `STELNETTTS_MELOTTS_BENCH`
- `STELNETTTS_MELOTTS_BERT`
- `STELNETTTS_MELOTTS_FORCE_SCALAR`
- `STELNETTTS_MELOTTS_WEIGHT_CACHE`

### Mimi codec

- `STELNETTTS_MIMI_NONCAUSAL`

### MiMo-ASR

- `STELNETTTS_MIMO_ASR_BENCH`
- `STELNETTTS_MIMO_ASR_DIAG`
- `STELNETTTS_MIMO_ASR_DUMP_STAGES`
- `STELNETTTS_MIMO_ASR_GPU`
- `STELNETTTS_MIMO_FORCE_CPU`
- `STELNETTTS_MIMO_SMOKE_DUMP`
- `STELNETTTS_MIMO_SMOKE_GPU`
- `STELNETTTS_MIMO_TOKENIZER_GPU`
- `STELNETTTS_MIMO_TOK_CPU`

### mini-omni2

- `STELNETTTS_MINI_OMNI2_BENCH`

### Moonshine

- `STELNETTTS_MOONSHINE_ALL_GPU`
- `STELNETTTS_MOONSHINE_BENCH`
- `STELNETTTS_MOONSHINE_ENC_ATTN`
- `STELNETTTS_MOONSHINE_NO_REPEAT_BREAK`
- `STELNETTTS_MOONSHINE_STREAMING_BENCH`
- `STELNETTTS_MOONSHINE_STREAMING_GPU`
- `STELNETTTS_MOONSHINE_STREAM_BENCH`

### MOSS family

- `STELNETTTS_MOSS_AUDIO_BENCH`
- `STELNETTTS_MOSS_AUDIO_EMBED_FAST`
- `STELNETTTS_MOSS_AUDIO_ENC_FLASH`
- `STELNETTTS_MOSS_AUDIO_ENC_MANUAL`
- `STELNETTTS_MOSS_AUDIO_FORCE_CPU`
- `STELNETTTS_MOSS_AUDIO_MEL_FILE`
- `STELNETTTS_MOSS_DIARIZE_BENCH`
- `STELNETTTS_MOSS_DIARIZE_DEBUG`
- `STELNETTTS_MOSS_DIARIZE_DUMP_CONV`
- `STELNETTTS_MOSS_DIARIZE_ENC_FLASH`
- `STELNETTTS_MOSS_DIARIZE_ENC_MANUAL`
- `STELNETTTS_MOSS_DIARIZE_FORCE_CPU`
- `STELNETTTS_MOSS_DIARIZE_NO_LOOPFIX`
- `STELNETTTS_MOSS_TRANSCRIBE_BENCH`
- `STELNETTTS_MOSS_TRANSCRIBE_ENC_DUMP`
- `STELNETTTS_MOSS_TRANSCRIBE_ENC_FLASH`
- `STELNETTTS_MOSS_TRANSCRIBE_ENC_MANUAL`
- `STELNETTTS_MOSS_TRANSCRIBE_FORCE_CPU`
- `STELNETTTS_MOSS_TRANSCRIBE_L0_DUMP`
- `STELNETTTS_MOSS_TRANSCRIBE_MEL_DUMP`
- `STELNETTTS_MOSS_TRANSCRIBE_NO_LOOPFIX`
- `STELNETTTS_MOSS_TTS_BENCH`
- `STELNETTTS_MOSS_TTS_LOCAL_DEBUG`
- `STELNETTTS_MOSS_TTS_LOCAL_GREEDY_AUDIO`
- `STELNETTTS_MOSS_TTS_LOCAL_NO_GPU`

### MP3 codec

- `STELNETTTS_MP3_ENCODER`

### Diarization — foxnose (#324)

- `STELNETTTS_DIARIZE_COUNT` — speaker-count estimator: `bic` (default, the
  upstream GMM/BIC + silhouette sweep) or `eigengap`. Eigengap is better on
  well-separated synthetic data and cheaper, but under-counts on real speech
  (11.4 % vs 5.3 % DER on VoxConverse) — see `docs/foxnose-diarize/PLAN.md`
- `STELNETTTS_DIARIZE_BIC_WINDOW` — score silhouette only in a `[k-2, k+3]` window
  around the BIC anchor instead of the full `[min, max]` range. The full range is
  the default: the BIC anchor is unreliable in both directions (measured errors
  of +5 / -3 / -3 on 4/5/6 well-separated blobs) and when it over-counts the
  window is stranded above the truth and cannot climb back to it
- `STELNETTTS_WESPEAKER_BENCH` — per-stage embedder timings (fbank / resnet /
  resnet_windows). Counting invocations of these is also how you check WHICH
  embedding path actually ran
- `STELNETTTS_WESPEAKER_DEBUG` — embedder diagnostics
- `STELNETTTS_DIARIZE_DEBUG` — chosen speaker count, the reason it was chosen,
  and the per-k silhouette curve behind it. Worth reading before trusting a
  count: on a borderline file the decision can rest on a <1 % score gap
- `STELNETTTS_DIARIZE_EMBED_WORKERS` — windows embedded concurrently (default:
  `-t`). Each worker gets its own context sharing one copy of the weights
- `STELNETTTS_SPEAKER_EMBED_THREADS` — ggml threads per embedder context
  (default: `-t`). Honoured by the pluggable embedders and by wespeaker
- `STELNETTTS_DIARIZE_SPAN_EMBED=1` — run ONE network pass per span of windows
  instead of one per window. 1.78x less diarization CPU for +0.30 mean DER on
  the VoxConverse dev shard; off by default because accuracy is the better
  default for a diarizer. See `docs/cli.md#diarization`
- `STELNETTTS_DIARIZE_SPAN_WINDOWS` — windows per span (default 32). Measured NOT
  to affect the accuracy cost — identical from N=2 to N=32 — so there is
  nothing to tune here; larger is simply faster

### GigaAM-v3

- `STELNETTTS_GIGAAM_BENCH` — per-stage timings (mel / encoder / decode)
- `STELNETTTS_GIGAAM_DEBUG` — encoder output min/max/mean
- `STELNETTTS_GIGAAM_FLASH` — `ggml_flash_attn_ext` in the encoder (opt-in; the
  manual QK^T path is what the per-stage diff was validated on)
- `STELNETTTS_GIGAAM_FORCE_SCALAR` — scalar LSTM/joint loops instead of cblas
- `STELNETTTS_GIGAAM_QUANT_ALL` — let `stelnettts-quantize` quantize the heads and
  the pre-encode convs too (default keeps them at source precision)

### Nemotron

- `STELNETTTS_NEMOTRON_BENCH`
- `STELNETTTS_NEMOTRON_CONTEXT_PRESET`
- `STELNETTTS_NEMOTRON_DEBUG`
- `STELNETTTS_NEMOTRON_DECODE_TIMING`
- `STELNETTTS_NEMOTRON_FORCE_SCALAR`
- `STELNETTTS_NEMOTRON_GGML_DECODE`
- `STELNETTTS_NEMOTRON_MAES`
- `STELNETTTS_NEMOTRON_NO_WINDOW_MASK`
- `STELNETTTS_NEMOTRON_STREAMING`

### OmniASR

- `STELNETTTS_OMNIASR_BENCH`
- `STELNETTTS_OMNIASR_DEBUG`
- `STELNETTTS_OMNIASR_DUMP_DIR`
- `STELNETTTS_OMNIASR_KEEP_F16_HEAD`
- `STELNETTTS_OMNIASR_KEEP_F16_TAIL`
- `STELNETTTS_OMNIASR_QUANT_ALL`

### OmniVoice

- `STELNETTTS_OMNIVOICE_ACENC_BISECT`
- `STELNETTTS_OMNIVOICE_AUTO_LANG` — **default on.** When no language was requested
  (`-l` / `-tl` / the server's `"language"` / `set_target_language`), guess one
  from the text being spoken and use it if it maps to an id the model knows.
  An explicitly requested language always wins; a low-confidence guess resolves
  to nothing and behaves exactly as before. `=0` restores the old
  always-language-agnostic behaviour. Exists because SubtitleEdit's language
  menu is not yet wired to its request payload (#13273).
- `STELNETTTS_OMNIVOICE_BENCH`
- `STELNETTTS_OMNIVOICE_CFG_INTERVAL`
- `STELNETTTS_OMNIVOICE_CHUNK`
- `STELNETTTS_OMNIVOICE_CLASS_TEMP`
- `STELNETTTS_OMNIVOICE_CODEC_FASTCONV`
- `STELNETTTS_OMNIVOICE_CODEC_GPU` — codec placement override (`1` = GPU, `0` = CPU). Unset defaults to GPU on
  CUDA and CPU on Metal/CPU.
- `STELNETTTS_OMNIVOICE_CPU`
- `STELNETTTS_OMNIVOICE_DEBUG_CODES`
- `STELNETTTS_OMNIVOICE_DEBUG_SUM`
- `STELNETTTS_OMNIVOICE_ENCODE_DIFF`
- `STELNETTTS_OMNIVOICE_FRAMES_PER_CHAR`
- `STELNETTTS_OMNIVOICE_FUSED_STEP`
- `STELNETTTS_OMNIVOICE_GUIDANCE`
- `STELNETTTS_OMNIVOICE_HUBERT_REF`
- `STELNETTTS_OMNIVOICE_NUM_STEPS`
- `STELNETTTS_OMNIVOICE_PERSISTENT_GRAPH`
- `STELNETTTS_OMNIVOICE_POS_TEMP`
- `STELNETTTS_OMNIVOICE_UNIFIED_CFG`
- `STELNETTTS_OMNIVOICE_VOICE_CACHE`

### OpenVoice2

- `STELNETTTS_OPENVOICE2_BENCH`

### OpenVoice2

- `STELNETTTS_OV2_DUMP_DIR`
- `STELNETTTS_OV2_FORCE_SCALAR`
- `STELNETTTS_OV2_NO_NORMALIZE`
- `STELNETTTS_OV2_TAU`

### Opus codec

- `STELNETTTS_OPUS_DEBUG`
- `STELNETTTS_OPUS_DECODER`
- `STELNETTTS_OPUS_ENCODER`

### Orpheus

- `STELNETTTS_ORPHEUS_BENCH`
- `STELNETTTS_ORPHEUS_BUCKET`
- `STELNETTTS_ORPHEUS_DEBUG`
- `STELNETTTS_ORPHEUS_DIFF_GPU`
- `STELNETTTS_ORPHEUS_DIFF_MAXGEN`
- `STELNETTTS_ORPHEUS_PROMPT_IDS`
- `STELNETTTS_ORPHEUS_SNAC_CODE`
- `STELNETTTS_ORPHEUS_SNAC_GPU`
- `STELNETTTS_ORPHEUS_SNAC_T_SUPER`

### OuteTTS

- `STELNETTTS_OUTETTS_BENCH`

### Paraformer

- `STELNETTTS_PARAFORMER_BENCH`
- `STELNETTTS_PARAFORMER_GPU`

### Parakeet

- `STELNETTTS_PARAKEET_ATT_CONTEXT`
- `STELNETTTS_PARAKEET_BENCH`
- `STELNETTTS_PARAKEET_CHUNK_OVERLAP`
- `STELNETTTS_PARAKEET_CHUNK_SECONDS`
- `STELNETTTS_PARAKEET_DEBUG`
- `STELNETTTS_PARAKEET_DECODE_TIMING`
- `STELNETTTS_PARAKEET_ENC_CACHE`
- `STELNETTTS_PARAKEET_ENC_PROBE`
- `STELNETTTS_PARAKEET_FORCE_SCALAR`
- `STELNETTTS_PARAKEET_GGML_DECODE`
- `STELNETTTS_PARAKEET_INTERNAL_CHUNKING`
- `STELNETTTS_PARAKEET_LONGFORM`
- `STELNETTTS_PARAKEET_MAES`
- `STELNETTTS_PARAKEET_MEM_COEFF`
- `STELNETTTS_PARAKEET_MEM_POLICY`
- `STELNETTTS_PARAKEET_QUANT_ALL`
- `STELNETTTS_PARAKEET_SIMULATE_ENCODE_OOM`
- `STELNETTTS_PARAKEET_STREAM_CHUNK`
- `STELNETTTS_PARAKEET_STREAM_OVERLAP`
- `STELNETTTS_PARAKEET_STREAM_THRESHOLD`
- `STELNETTTS_PARAKEET_VAD_SLICE_CAP`
- `STELNETTTS_PARAKEET_VRAM_BUDGET_MB`

### Parler-TTS

- `STELNETTTS_PARLER_BUCKET`
- `STELNETTTS_PARLER_DEBUG`
- `STELNETTTS_PARLER_DESC_IDS`
- `STELNETTTS_PARLER_DIFF_MAXGEN`
- `STELNETTTS_PARLER_DUMP_ENC`
- `STELNETTTS_PARLER_PROMPT_IDS`
- `STELNETTTS_PARLER_TTS_BENCH`

### Piper

- `STELNETTTS_PIPER_FORCE_SCALAR`
- `STELNETTTS_PIPER_TTS_BENCH`
- `STELNETTTS_PIPER_WEIGHT_CACHE`

### Pocket-TTS

- `STELNETTTS_POCKET_DUMP_DIR`
- `STELNETTTS_POCKET_FORCE_LATENTS`
- `STELNETTTS_POCKET_FORCE_NOISE`
- `STELNETTTS_POCKET_MANUAL_BACKBONE`
- `STELNETTTS_POCKET_MANUAL_MIMI`
- `STELNETTTS_POCKET_MAX_FRAMES`
- `STELNETTTS_POCKET_MIMI_DUMP`
- `STELNETTTS_POCKET_MIMI_SCALAR`
- `STELNETTTS_POCKET_TTS_BENCH`
- `STELNETTTS_POCKET_VOICE_CACHE`
- `STELNETTTS_POCKET_VULKAN_MIMI_MAX_FRAMES`

### Pyannote segmentation

- `STELNETTTS_PYANNOTE_LEGACY`
- `STELNETTTS_PYANNOTE_SEG_BENCH`
- `STELNETTTS_PYANNOTE_SEG_DUMP`
- `STELNETTTS_PYANNOTE_CHUNK_S` — audio per chunk of parallel inference, in
  seconds (default 60; `0` restores the pre-#326 single scan over the whole
  file). Chunking is decided by audio LENGTH and never by thread count, so
  posteriors do not change with `-t`
- `STELNETTTS_PYANNOTE_CHUNK_CONTEXT_S` — real audio spliced either side of a
  chunk and then trimmed (default 5), which absorbs the convolutions' zero
  padding and the LSTM's zero initial state

### Qwen3-ASR

- `STELNETTTS_CIELVOX2ASR_QUANT_AUDIO`

### Qwen3-ASR / Qwen3-TTS

- `STELNETTTS_CIELVOX2_ASR_BENCH`
- `STELNETTTS_CIELVOX2_ASR_EMBED_FAST`
- `STELNETTTS_CIELVOX2_ASR_FUSED_QKV`
- `STELNETTTS_QWEN3_SYSPROMPT_LANG`
- `STELNETTTS_CIELVOX2_TTS_BENCH`
- `STELNETTTS_CIELVOX2_TTS_CODEC_ALLOW_FULL`
- `STELNETTTS_CIELVOX2_TTS_CODEC_CHUNK`
- `STELNETTTS_CIELVOX2_TTS_CODEC_CPU`
- `STELNETTTS_CIELVOX2_TTS_CODEC_CTX`
- `STELNETTTS_CIELVOX2_TTS_CODEC_FORCE_METAL`
- `STELNETTTS_CIELVOX2_TTS_CODEC_GGUF`
- `STELNETTTS_CIELVOX2_TTS_CODEC_GPU`
- `STELNETTTS_CIELVOX2_TTS_CODEC_TRACE`
- `STELNETTTS_CIELVOX2_TTS_EMBD_CHECK`
- `STELNETTTS_CIELVOX2_TTS_DUMP_LOGITS=<dir>` — write the raw per-frame talker
  logits (f32, before the repetition penalty and the suppress mask) plus a
  top-5 line to stderr. The instrument for a cross-backend diff: tokens
  alone cannot tell a miscompute from amplified rounding (#337).
- `STELNETTTS_CIELVOX2_TTS_REPLAY_CODES=<file>` — 16 whitespace-separated codec ids
  per frame; the decode uses them instead of sampling. Teacher forcing, and
  the ONLY way to compare two backends step by step: pin the whole frame or
  the 15 residual codebooks (which must be sampled) diverge and the diff
  measures trajectory, not arithmetic (#337).
- `STELNETTTS_CIELVOX2_TTS_REPLAY_TOKENS=<file>` — the weaker form: codebook-0 ids
  only. Useful for forcing a trajectory, NOT sufficient for a logits diff.
- `STELNETTTS_CIELVOX2_TTS_GREEDY` — force the talker's codebook-0 sampler to argmax
  (top_k=1). The frame sequence then depends only on the logits, so two
  backends agree if and only if their logits agree — this is the lever for
  telling a GPU miscompute apart from a sampling difference, and without it a
  CPU-vs-GPU token comparison proves nothing (#337).
- `STELNETTTS_CIELVOX2_TTS_MAX_FRAMES`
- `STELNETTTS_CIELVOX2_TTS_SKIP_REF_DECODE`

### SenseVoice

- `STELNETTTS_SENSEVOICE_BENCH`
- `STELNETTTS_SENSEVOICE_NO_FA`

### Sidon

- `STELNETTTS_SIDON_FASTCONV` — DAC convolution mode (`off`, `k1-f16`, `k1-f32`, or `full`). Unset defaults to
  `k1-f16` on CUDA and `off` on Vulkan/CPU.
- `STELNETTTS_SIDON_RPE` — relative-position-bias formulation: `bucket-direct` (default), `bucket`, or `expand`
  (legacy `[head_dim, T, T]` expansion, ~1 GiB more predictor workspace at `T≈2825`; keeps the Vulkan
  `mul_mat` batching branch). All three are algebraically equivalent.
- `STELNETTTS_SIDON_DECODER_CHUNK_FRAMES` — maximum DAC core size in feature frames (default `512`). `0` decodes
  the whole utterance in one graph (~4.5 GiB at `T≈2825` vs ~0.79 GiB chunked). Chunked output is bit-exact
  against the whole-utterance decode.
- `STELNETTTS_SIDON_LOOKAHEAD` — set to `0` to disable the input padding (one leading predictor frame plus 1.5 s
  of right-side lookahead). Padding is on by default; without it the last ~12 ms of every clip is a full-scale
  transient.
- `STELNETTTS_SIDON_MAX_FRAMES` — predictor input cap in feature frames (default `3000`, ~58.5 s after the
  lookahead). Guards the `O(T^2)` attention.
- `STELNETTTS_SIDON_DEBUG` — print per-stage scheduler workspace sizes (per backend) after graph allocation.

### Sherpa

- `STELNETTTS_SHERPA_LID_BIN`

### Silero LID

- `STELNETTTS_SILERO_FORCE_SCALAR`
- `STELNETTTS_SILERO_LID_BENCH`
- `STELNETTTS_SILERO_LID_DEBUG`
- `STELNETTTS_SILERO_LID_DUMP`
- `STELNETTTS_SILERO_LID_LEGACY`
- `STELNETTTS_SILERO_LID_MAX_S`
- `STELNETTTS_SILERO_LID_TRACE`
- `STELNETTTS_SILERO_LID_TRACE_OFF`
- `STELNETTTS_SILERO_LID_TRUNC`
- `STELNETTTS_SILERO_LID_VULKAN`

### SpeechT5

- `STELNETTTS_SPEECHT5_DUMP_DIR`
- `STELNETTTS_SPEECHT5_FASTCONV`
- `STELNETTTS_SPEECHT5_FASTCONV_DEBUG`
- `STELNETTTS_SPEECHT5_TTS_BENCH`

### T5 translate

- `STELNETTTS_T5_GPU`
- `STELNETTTS_T5_TRANSLATE_BENCH`

### TaDa TTS

- `STELNETTTS_TADA_ACOUSTIC_CFG`
- `STELNETTTS_TADA_ALLOW_VULKAN`
- `STELNETTTS_TADA_BATCH_PREFILL`
- `STELNETTTS_TADA_BENCH`
- `STELNETTTS_TADA_BUCKET_MIN`
- `STELNETTTS_TADA_CFG_INTERVAL`
- `STELNETTTS_TADA_CFG_INTERVAL_DEBUG`
- `STELNETTTS_TADA_CODEC_BENCH`
- `STELNETTTS_TADA_CODEC_DUMP`
- `STELNETTTS_TADA_CODEC_GGUF`
- `STELNETTTS_TADA_CODEC_VULKAN_NATIVE`
- `STELNETTTS_TADA_CTC_ASR`
- `STELNETTTS_TADA_DIFF_TEXT`
- `STELNETTTS_TADA_DO_SAMPLE`
- `STELNETTTS_TADA_DUMP_ACOUSTIC_FEATURES`
- `STELNETTTS_TADA_DUMP_FEATURES`
- `STELNETTTS_TADA_DUMP_FM_STEPS`
- `STELNETTTS_TADA_DUMP_TIME_BEFORE`
- `STELNETTTS_TADA_ENCODER_DEBUG`
- `STELNETTTS_TADA_EXTRA_STEPS`
- `STELNETTTS_TADA_FM_B2`
- `STELNETTTS_TADA_KEEP_F16_HEAD`
- `STELNETTTS_TADA_KEEP_F16_TAIL`
- `STELNETTTS_TADA_MAX_EXPANDED_FRAMES`
- `STELNETTTS_TADA_NOISE_TEMP`
- `STELNETTTS_TADA_NO_BUCKET`
- `STELNETTTS_TADA_NUM_CANDIDATES`
- `STELNETTTS_TADA_NUM_FM_STEPS`
- `STELNETTTS_TADA_PROMPT_CACHE`
- `STELNETTTS_TADA_PROMPT_TEXT`
- `STELNETTTS_TADA_QUANT_ALL`
- `STELNETTTS_TADA_REPETITION_PENALTY`
- `STELNETTTS_TADA_SCORER`
- `STELNETTTS_TADA_TALKER_TIMING`
- `STELNETTTS_TADA_TEMPERATURE`
- `STELNETTTS_TADA_TOP_K`
- `STELNETTTS_TADA_TOP_P`
- `STELNETTTS_TADA_VULKAN_NATIVE`

### TitaNet speaker

- `STELNETTTS_TITANET_BENCH`
- `STELNETTTS_TITANET_DUMP`
- `STELNETTTS_TITANET_DUMP_MEL`
- `STELNETTTS_TITANET_FORCE_SCALAR`
- `STELNETTTS_TITANET_GGML`
- `STELNETTTS_TITANET_GPU`
- `STELNETTTS_TITANET_LEGACY`
- `STELNETTTS_TITANET_REF_MEL`

Three compute paths exist and all are kept working; the default is the fastest
one measured per platform.

| path | selected by | measured, M1, 2 s segment |
| --- | --- | --- |
| legacy (Accelerate / hand-rolled) | default where `HAVE_ACCELERATE` | **71.7 ms** |
| ggml graph, CPU | default elsewhere; `STELNETTTS_TITANET_GGML=1` | 277.3 ms |
| ggml graph, GPU | `STELNETTTS_TITANET_GGML=1 STELNETTTS_TITANET_GPU=1` | 31.9 ms *(but see below)* |

All three agree to cosine **1.000000** with each other and **0.999996** against
NVIDIA's `nemo_en_titanet_large.onnx` export fed the same mel — so the choice is
purely about speed.

⚠ **`STELNETTTS_TITANET_GPU=1` is opt-in because it loses on real workloads
despite winning the micro-benchmark.** Diarization embeds one segment per call
at *variable* lengths, so every call reshapes the graph and the GPU allocator
re-reserves; and `STELNETTTS_SPEAKER_EMBED_WORKERS` runs several embedders at once,
which contend for the one GPU. End-to-end on a 600 s clip, 47 segments:

```
workers=4, legacy   9994 ms    <- default, fastest
workers=1, legacy  12673 ms
workers=1, GPU     15275 ms
workers=4, GPU     49866 ms
```

Keep it for evaluating a discrete GPU (where the balance may differ) or for
`STELNETTTS_SPEAKER_EMBED_WORKERS=1` on a machine with weak CPU cores. Bucketing
segment lengths so the graph shape stops changing is the work that would make
this path win generally.

`STELNETTTS_TITANET_DUMP_MEL=<path>` writes the computed mel as `[T][n_mels]`
float32 — the counterpart to `STELNETTTS_TITANET_REF_MEL`. Feeding that dump to an
upstream ONNX export separates the front-end from the network, which a single
end-to-end cosine cannot do.

### VibeVoice

- `STELNETTTS_VIBEVOICE_BENCH`
- `STELNETTTS_VIBEVOICE_DEBUG`
- `STELNETTTS_VIBEVOICE_DUMP_DIR`
- `STELNETTTS_VIBEVOICE_ENCODER_CHUNK_SECONDS`
- `STELNETTTS_VIBEVOICE_ENCODER_CONTEXT_SECONDS`
- `STELNETTTS_VIBEVOICE_LM_BUCKETS`
- `STELNETTTS_VIBEVOICE_NO_LM_BUCKETS`
- `STELNETTTS_VIBEVOICE_PRED_SCHED`
- `STELNETTTS_VIBEVOICE_QUANT_ALL`
- `STELNETTTS_VIBEVOICE_REF_FEATURES`
- `STELNETTTS_VIBEVOICE_REUSE_PRED_GRAPH`
- `STELNETTTS_VIBEVOICE_TTS_CFG_SCALE`
- `STELNETTTS_VIBEVOICE_TTS_DUMP`
- `STELNETTTS_VIBEVOICE_TTS_DUMP_DECODER`
- `STELNETTTS_VIBEVOICE_TTS_DUMP_PERFRAME`
- `STELNETTTS_VIBEVOICE_TTS_FLASH_ATTN`
- `STELNETTTS_VIBEVOICE_TTS_LATENTS`
- `STELNETTTS_VIBEVOICE_TTS_NOISE`
- `STELNETTTS_VIBEVOICE_TTS_SEED`
- `STELNETTTS_VIBEVOICE_TTS_TRACE`
- `STELNETTTS_VIBEVOICE_TTS_TRACE_FRAME`
- `STELNETTTS_VIBEVOICE_VAE_BACKEND`
- `STELNETTTS_VIBEVOICE_VOICE_AUDIO`

### VoxCPM2

- `STELNETTTS_VOXCPM2_BENCH`
- `STELNETTTS_VOXCPM2_CFG_INTERVAL`
- `STELNETTTS_VOXCPM2_CFG_INTERVAL_DEBUG`
- `STELNETTTS_VOXCPM2_CFG_VALUE`
- `STELNETTTS_VOXCPM2_CPU_ONLY`
- `STELNETTTS_VOXCPM2_INFERENCE_STEPS`
- `STELNETTTS_VOXCPM2_MAX_LEN`
- `STELNETTTS_VOXCPM2_USE_REF`
- `STELNETTTS_VOXCPM2_VAE_MAX_SAMPLES` - maximum 16 kHz input samples accepted by one `voxcpm2-vae` upscaling call
  (default `960000`, or 60 seconds). Split longer audio, or raise this only when enough RAM/VRAM is available.

### Voxtral / Voxtral-TTS

- `STELNETTTS_VOXTRAL_BENCH`
- `STELNETTTS_VOXTRAL_FUSED_QKV`
- `STELNETTTS_VOXTRAL_TTS_CODEC_FROM_FILE`
- `STELNETTTS_VOXTRAL_TTS_DIFF_DUMP`
- `STELNETTTS_VOXTRAL_TTS_SEMANTIC_CB`
- `STELNETTTS_VOXTRAL_TTS_TEXT`
- `STELNETTTS_VOXTRAL_TTS_VOICE`

### Voxtral-4B

- `STELNETTTS_VOXTRAL4B_BENCH`
- `STELNETTTS_VOXTRAL4B_FUSED_QKV`
- `STELNETTTS_VOXTRAL4B_STREAM_BATCH_ENCODER`
- `STELNETTTS_VOXTRAL4B_STREAM_CHUNK_MS`
- `STELNETTTS_VOXTRAL4B_STREAM_DEBUG`
- `STELNETTTS_VOXTRAL4B_STREAM_DECODER_THREAD`
- `STELNETTTS_VOXTRAL4B_STREAM_DIFF`
- `STELNETTTS_VOXTRAL4B_STREAM_LIVE`
- `STELNETTTS_VOXTRAL4B_STREAM_TIMING`

### Wav2Vec2

- `STELNETTTS_WAV2VEC2_BENCH`
- `STELNETTTS_WAV2VEC2_DUMP_DIR`
- `STELNETTTS_WAV2VEC2_VERBOSE`

### WavTokenizer

- `STELNETTTS_WAVTOK_BENCH`
- `STELNETTTS_WAVTOK_DUMP_DIR`
- `STELNETTTS_WAVTOK_FIXED_CODES`

### Zonos

- `STELNETTTS_ZONOS_CPP_DUMP_DIR`
- `STELNETTTS_ZONOS_DECODE_CHUNK`
- `STELNETTTS_ZONOS_DECODE_CTX`
- `STELNETTTS_ZONOS_DIFF_N_STEPS`
- `STELNETTTS_ZONOS_FASTCONV`
- `STELNETTTS_ZONOS_SPEAKER_EMB_PATH`
- `STELNETTTS_ZONOS_TTS_BENCH`
- `STELNETTTS_ZONOS_TTS_TEXT`

