# StelnetTTS — Architecture

One-page dependency map for contributors. Start here if you need to
know which file to edit for a given change, or which models would be
affected by a given `src/core/` refactor.

For user-facing docs see `README.md`. For pending work see `PLAN.md`.
For the reasoning behind design choices see `LEARNINGS.md`.

---

## Layer overview

```
┌───────────────────────────────────────────────────────────────────┐
│ examples/cli/  — the stelnettts binary                              │
│ ──────────────────────────────────────────────────────────────── │
│   cli.cpp                   stelnettts entry + --backend branch  │
│   whisper_params.h          shared params struct                  │
│                                                                    │
│   stelnettts_backend.{h,cpp}  interface + factory + GGUF detect     │
│   stelnettts_backend_*.cpp    8 per-model adapter files             │
│     whisper (adapter is cli.cpp's historical path)                │
│     parakeet  canary  cohere  granite                             │
│     voxtral   voxtral4b  qwen3                                    │
│                                                                    │
│   stelnettts_run.cpp          top-level pipeline dispatch           │
│   stelnettts_vad.{h,cpp}      Silero VAD slicing                    │
│   stelnettts_output.{h,cpp}   TXT/SRT/VTT/CSV/JSON/LRC writers      │
│   stelnettts_model_mgr.{h,cpp}  -m auto via curl/wget               │
│   stelnettts_aligner.{h,cpp}  canary_ctc forced alignment wrapper   │
│   stelnettts_llm_pipeline.h   shared LLM decode loop (CLI-side)     │
├───────────────────────────────────────────────────────────────────┤
│ src/  — per-model C runtimes (public headers in include/)         │
│ ──────────────────────────────────────────────────────────────── │
│   whisper.{h,cpp}          OpenAI Whisper (reference impl)        │
│   parakeet.{h,cpp}         NVIDIA Parakeet TDT                    │
│   canary.{h,cpp}           NVIDIA Canary 1B v2                    │
│   canary_ctc.{h,cpp}       Canary auxiliary CTC aligner           │
│   cohere.{h,cpp}            Cohere Transcribe 2B                  │
│   cielvox2_asr.{h,cpp}        Qwen3-ASR 0.6B (speech-LLM)            │
│   voxtral.{h,cpp}           Voxtral-Mini-3B (speech-LLM)          │
│   voxtral4b.{h,cpp}         Voxtral-Mini-4B-Realtime              │
│   granite_speech.{h,cpp}    Granite 4.0-1B Speech                 │
│   wav2vec2-ggml.{h,cpp}     Wav2vec2 CTC (cohere-align)           │
├───────────────────────────────────────────────────────────────────┤
│ src/core/  — shared primitives (static library stelnettts-core)    │
│ ──────────────────────────────────────────────────────────────── │
│   mel.{h,cpp}          log-mel spectrogram (NeMo + HF clusters)   │
│   ffn.h                SwiGLU / plain-SiLU FFN (header-only)      │
│   attention.h          Llama-style MHA + flash-attn (header-only) │
│   gguf_loader.{h,cpp}  GGUF open + weight mmap + name lookup      │
├───────────────────────────────────────────────────────────────────┤
│ ggml/  — tensor library + backend dispatch + quantisation         │
└───────────────────────────────────────────────────────────────────┘
```

---

## `src/core/` consumption map

Which model uses which shared helper, today:

|               | `mel` | `ffn` | `attention` | `gguf_loader` |
|---|:---:|:---:|:---:|:---:|
| parakeet      |  ✔  |     |     |  ✔  |
| canary        |  ✔  |     |     |  ✔  |
| canary_ctc    |  ✔  |     |     |  ✔  |
| cohere        |  ✔  |     |     |  ✔  |
| voxtral       |  ✔  |  ✔  |  ✔ (LLM block) |  ✔  |
| voxtral4b     |  ✔  |  ✔  |  ✔ (encoder + LLM) |  ✔  |
| cielvox2_asr     |  ✔  |  ✔  |     |  ✔  |
| granite_speech|     |  ✔  |     |  ✔  |
| wav2vec2-ggml |     |     |     |     |
| **whisper**   |     |     |     |     |

**Whisper is intentionally not migrated** — it's the battle-tested
reference and the test gate for every other refactor.

**Granite mel** is the last holdout on `core_mel` coverage: granite's
mel output is stacked `(160, T/2)` = two 80-mel frames zipped along
channels. Needs a `core_mel::Params::stacked_frames` knob. Tracked in
`TODO.md`.

**`wav2vec2-ggml`** is called from the legacy `cohere-align` path; it
has a minimal model structure and hasn't been worth migrating.

---

## Dependency graph (edge direction: "depends on")

```
             stelnettts binary
                   │
    ┌──────────────┴──────────────┐
    │                              │
 cli.cpp                   stelnettts_backend_*.cpp
    │                              │
    │                              ├─→ whisper.{cpp,h}
    │                              ├─→ parakeet.{cpp,h} ──┐
    │                              ├─→ canary.{cpp,h}    │
    │                              ├─→ canary_ctc.*     ─┤
    │                              ├─→ cohere.{cpp,h}   │
    │                              ├─→ cielvox2_asr.*      ─┤
    │                              ├─→ voxtral.{cpp,h}  ─┼──→ stelnettts-core
    │                              ├─→ voxtral4b.*     ──┤    (src/core/)
    │                              ├─→ granite_speech.*─ ┘       │
    │                              └─→ canary_ctc (aligner)      │
    │                                                             │
    ├─→ common (stelnettts example lib)                          │
    ├─→ whisper (for the whisper-backend path)                    │
    └─→ stelnettts_{vad,output,model_mgr,aligner,run}               │
                                                                   │
                                                                   ▼
                                                                 ggml
```

Every non-whisper model links `stelnettts-core`. The whisper model does
not, by design.

---

## How to find the code for a feature

| Looking for | Look in |
|---|---|
| `--backend` CLI flag parsing | `examples/cli/cli.cpp` (look for `"--backend"`) |
| Backend auto-detection from GGUF | `examples/cli/stelnettts_backend.cpp` → `stelnettts_detect_backend_from_gguf` |
| Feature capability matrix / warnings | `examples/cli/stelnettts_run.cpp` → `warn_unsupported` |
| `-m auto` download | `examples/cli/stelnettts_model_mgr.cpp` |
| VAD slicing | `examples/cli/stelnettts_vad.{h,cpp}` |
| SRT / VTT / JSON writers | `examples/cli/stelnettts_output.{h,cpp}` |
| CTC alignment for LLM backends | `examples/cli/stelnettts_aligner.{h,cpp}` |
| Whisper code path (historical) | `examples/cli/cli.cpp` main(), post-dispatch |
| Model-specific transcribe logic | `examples/cli/stelnettts_backend_<X>.cpp` |
| Model-specific mel / encoder / LLM | `src/<model>.cpp` |
| Shared log-mel spectrogram | `src/core/mel.{h,cpp}` |
| Shared SwiGLU FFN helper | `src/core/ffn.h` |
| Shared Llama self-attention helper | `src/core/attention.h` |
| Shared GGUF loading + weight map | `src/core/gguf_loader.{h,cpp}` |
| Streaming session API (whisper / kyutai-stt / moonshine-streaming) | `src/stelnettts_c_api.cpp` → `stelnettts_session_stream_open` + `stelnettts_stream_*` dispatch |
| Native voxtral4b streaming (PLAN #7: incremental encoder + speculative prefill + live captions + decoder thread) | `src/voxtral4b.cpp` → `voxtral4b_stream_*` |
| Mic capture (cross-platform via miniaudio) | `src/stelnettts_mic.{h,cpp}` |

---

## Adding a new backend

Five files, ~200-300 LOC total (most of it in the src/ model file).
Step-by-step with worked examples is in `README.md` →
"Adding a new backend". Short version:

1. Implement `src/yourmodel.{h,cpp}` with a C API. Prefer
   `core_mel::compute`, `core_ffn::swiglu`, `core_attn::…`, and
   `core_gguf::…` over hand-rolling the equivalents.
2. Wrap it in `examples/cli/stelnettts_backend_yourmodel.cpp` (~120
   LOC, see `stelnettts_backend_parakeet.cpp` as the minimal template).
3. Register in `examples/cli/stelnettts_backend.cpp` factory + list,
   add the architecture string to `stelnettts_detect_backend_from_gguf`.
4. Link in `src/CMakeLists.txt` (new library) and
   `examples/cli/CMakeLists.txt` (add to stelnettts target).
5. Optional: register the default quantised HF repo in
   `stelnettts_model_mgr.cpp` so `-m auto` works.

Regression-test by running `stelnettts --backend yourmodel -m model.gguf
-f samples/jfk.wav` before AND after your change and `diff`-ing the
output. Bit-identical is the gate.

---

## What's intentionally NOT shared

Some code looks duplicated but isn't worth extracting:

- **Each model's custom Cooley-Tukey FFT.** Nine near-identical
  radix-2 implementations. Unifying them would save ~100 LOC at the
  cost of making the FFT function pointer indirection mandatory.
  `core_mel::FftR2C` accepts any of them via a thread-local scratch
  wrapper (see `LEARNINGS.md` → "In-place recursive FFTs are
  const-unsafe"). Not a blocker.

- **Each model's GGUF tensor naming scheme.** These are genuinely
  model-specific (e.g. `encoder.layers.0.attn.q.weight` vs
  `model.audio.encoder.layer.0.self_attn.q_proj.weight`) and have to
  live in the per-model loader. `core_gguf::` handles the scaffolding
  around them; the per-field assignment loop stays.

- **Each model's forward graph structure.** The `src/core/attention.h`
  and `src/core/ffn.h` helpers cover the common building blocks, but
  the overall graph topology (how many encoder layers, where the
  projector sits, how the KV cache is threaded, what the prompt
  template looks like) is model-specific and belongs in `src/<model>.cpp`.
