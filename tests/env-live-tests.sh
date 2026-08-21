#!/bin/bash
# env-live-tests.sh — set env vars for running integration / live tests.
#
# Usage: source tests/env-live-tests.sh && ctest --test-dir build --rerun-failed
#
# Override STELNETTTS_MODELS_DIR to point at your local model cache:
#   STELNETTTS_MODELS_DIR=/my/models source tests/env-live-tests.sh
#
# Models are looked up via STELNETTTS_MODELS_DIR (defaults to ~/.cache/stelnettts).
# The auto-download cache and well-known search dirs also probe this path.

STELNETTTS_MODELS_DIR="${STELNETTTS_MODELS_DIR:-$HOME/.cache/stelnettts}"
export STELNETTTS_MODELS_DIR

# ── Whisper (beam search, VAD tests) ──
# Whisper models use the ggml-*.bin naming convention and are typically in
# the auto-download cache (~/.cache/stelnettts), not the GGUF model dir.
_whisper_cache="${HOME}/.cache/stelnettts"
if [ -f "$STELNETTTS_MODELS_DIR/ggml-tiny.bin" ]; then
    _whisper_default="$STELNETTTS_MODELS_DIR/ggml-tiny.bin"
elif [ -f "$_whisper_cache/ggml-tiny.bin" ]; then
    _whisper_default="$_whisper_cache/ggml-tiny.bin"
else
    _whisper_default="$STELNETTTS_MODELS_DIR/ggml-tiny.bin"
fi
export STELNETTTS_MODEL_WHISPER="${STELNETTTS_MODEL_WHISPER:-$_whisper_default}"
unset _whisper_cache _whisper_default

# Tiron (#295): Whisper large-v3 + inline <|speakerN|> markers (legacy ggml bin).
export STELNETTTS_MODEL_TIRON="${STELNETTTS_MODEL_TIRON:-$STELNETTTS_MODELS_DIR/tiron-q4_k.bin}"

# ── Beam search backends ──
export STELNETTTS_MODEL_GLM_ASR="${STELNETTTS_MODEL_GLM_ASR:-$STELNETTTS_MODELS_DIR/glm-asr-nano.gguf}"
export STELNETTTS_MODEL_QWEN3_ASR="${STELNETTTS_MODEL_QWEN3_ASR:-$STELNETTTS_MODELS_DIR/cielvox2-asr-0.6b.gguf}"
export STELNETTTS_MODEL_HIGGS_STT="${STELNETTTS_MODEL_HIGGS_STT:-$STELNETTTS_MODELS_DIR/higgs-stt-q8_0.gguf}"
export STELNETTTS_MODEL_VOXTRAL_TTS="${STELNETTTS_MODEL_VOXTRAL_TTS:-$STELNETTTS_MODELS_DIR/voxtral-4b-tts-q4_k.gguf}"
export STELNETTTS_MODEL_CANARY="${STELNETTTS_MODEL_CANARY:-$STELNETTTS_MODELS_DIR/canary-1b-v2.gguf}"
# canary-qwen SALM (nvidia/canary-qwen-2.5b). #247 short-window echo regression.
export STELNETTTS_MODEL_CANARY_QWEN="${STELNETTTS_MODEL_CANARY_QWEN:-$STELNETTTS_MODELS_DIR/canary-qwen-2.5b-q8_0.gguf}"
export STELNETTTS_MODEL_LFM2_EN="${STELNETTTS_MODEL_LFM2_EN:-$STELNETTTS_MODELS_DIR/lfm2-audio-1.5b-q5_k.gguf}"
export STELNETTTS_MODEL_LFM2_JP="${STELNETTTS_MODEL_LFM2_JP:-$STELNETTTS_MODELS_DIR/lfm2-audio-1.5b-jp-q5_k.gguf}"
# dots.tts: F16 core (the CFG flow-match derails on full-q8) + vocoder companion.
export STELNETTTS_MODEL_DOTS_TTS="${STELNETTTS_MODEL_DOTS_TTS:-$STELNETTTS_MODELS_DIR/dots-tts-soar-f16.gguf}"
export STELNETTTS_MODEL_DOTS_TTS_VOCODER="${STELNETTTS_MODEL_DOTS_TTS_VOCODER:-$STELNETTTS_MODELS_DIR/dots-tts-soar-vocoder-f16.gguf}"
export STELNETTTS_MODEL_COHERE="${STELNETTTS_MODEL_COHERE:-$STELNETTTS_MODELS_DIR/cohere-transcribe.gguf}"

# ── Kokoro TTS + its G2P live check (#316) ──
# tests/test-kokoro-g2p-live.sh reads these three and silently skips without
# them, so the G2P path — the one that used to drop numbers entirely — was
# never exercised by a `source tests/env-live-tests.sh` run. The defaults match
# the names the script already falls back to.
export STELNETTTS_KOKORO_MODEL="${STELNETTTS_KOKORO_MODEL:-$STELNETTTS_MODELS_DIR/kokoro-82m-q8_0.gguf}"
export STELNETTTS_KOKORO_VOICE="${STELNETTTS_KOKORO_VOICE:-$STELNETTTS_MODELS_DIR/kokoro-voice-af_heart.gguf}"

# ── Parakeet JA long-form regression guard (issue #89) ──
# Fixture: hf download Xenna/stelnettts-regression-fixtures \
#     parakeet-tdt-0.6b-ja/reazon_baseball_14s/audio.wav --local-dir <dir>
export STELNETTTS_MODEL_PARAKEET_JA="${STELNETTTS_MODEL_PARAKEET_JA:-$STELNETTTS_MODELS_DIR/parakeet-tdt-0.6b-ja.gguf}"
export STELNETTTS_FIXTURE_PARAKEET_JA="${STELNETTTS_FIXTURE_PARAKEET_JA:-$STELNETTTS_MODELS_DIR/fixtures/reazon_baseball_14s.wav}"

# ── Paraformer ── (canonical STELNETTTS_ names; old bare names honored if pre-set)
export STELNETTTS_PARAFORMER_MODEL="${STELNETTTS_PARAFORMER_MODEL:-${PARAFORMER_MODEL:-$STELNETTTS_MODELS_DIR/paraformer-zh-f16.gguf}}"
export STELNETTTS_PARAFORMER_MODEL_Q4K="${STELNETTTS_PARAFORMER_MODEL_Q4K:-${PARAFORMER_MODEL_Q4K:-$STELNETTTS_MODELS_DIR/paraformer-zh-q4_k.gguf}}"
export STELNETTTS_PARAFORMER_AUDIO_ZH="${STELNETTTS_PARAFORMER_AUDIO_ZH:-${PARAFORMER_AUDIO_ZH:-samples/paraformer_zh.wav}}"

# ── Aligner (issue #217) ──
export STELNETTTS_MODEL_ALIGNER="${STELNETTTS_MODEL_ALIGNER:-$STELNETTTS_MODELS_DIR/canary-ctc-aligner-q4_k.gguf}"

# ── Diarization ──
export STELNETTTS_TEST_DIARIZE_MODEL="${STELNETTTS_TEST_DIARIZE_MODEL:-$STELNETTTS_MODELS_DIR/pyannote-seg-3.0.gguf}"
export STELNETTTS_TEST_TITANET_MODEL="${STELNETTTS_TEST_TITANET_MODEL:-$STELNETTTS_MODELS_DIR/titanet-large.gguf}"
export STELNETTTS_TEST_DIARIZE_WAV="${STELNETTTS_TEST_DIARIZE_WAV:-samples/multispeaker.wav}"

# ── Chat (LLM) — requires a llama.cpp-compatible chat model with a chat
# template (e.g. smollm2-360m-instruct, qwen2.5-0.5b-instruct). Harrier
# is an embedding model and won't work.
_chat_default="$STELNETTTS_MODELS_DIR/smollm2-360m-instruct-q4_k.gguf"
if [ -n "${STELNETTTS_CHAT_TEST_MODEL:-}" ]; then
    export STELNETTTS_CHAT_TEST_MODEL
elif [ -f "$_chat_default" ]; then
    export STELNETTTS_CHAT_TEST_MODEL="$_chat_default"
fi
unset _chat_default

# MOSS-Audio (OpenMOSS-Team/MOSS-Audio-4B-Instruct): audio understanding + ASR
export STELNETTTS_MODEL_MOSS_AUDIO="${STELNETTTS_MODEL_MOSS_AUDIO:-$STELNETTTS_MODELS_DIR/moss-audio-4b-instruct-q4_k.gguf}"

# MOSS-Transcribe (OpenMOSS-Team/MOSS-Transcribe-preview-2B): ASR
export STELNETTTS_MODEL_MOSS_TRANSCRIBE="${STELNETTTS_MODEL_MOSS_TRANSCRIBE:-$STELNETTTS_MODELS_DIR/moss-transcribe-preview-2b-q4_k.gguf}"

# MOSS-Transcribe-Diarize (OpenMOSS-Team/MOSS-Transcribe-Diarize-0.9B): ASR + diarization + timestamps
export STELNETTTS_MODEL_MOSS_DIARIZE="${STELNETTTS_MODEL_MOSS_DIARIZE:-$STELNETTTS_MODELS_DIR/moss-transcribe-diarize-0.9b-q4_k.gguf}"

# MOSS-TTS-v1.5 (OpenMOSS-Team/MOSS-TTS-v1.5): TTS — Qwen3-8B backbone + 32 RVQ
# codebooks + transformer codec companion (validated by ASR round-trip, #249).
# MioTTS-0.6B (Qwen3 + MioCodec, Apache-2.0)
export STELNETTTS_MODEL_MIOTTS="${STELNETTTS_MODEL_MIOTTS:-$STELNETTTS_MODELS_DIR/miotts-0.6b-q8_0.gguf}"
export STELNETTTS_MODEL_PIANO_TRANSCRIPTION="${STELNETTTS_MODEL_PIANO_TRANSCRIPTION:-$STELNETTTS_MODELS_DIR/piano-transcription-f16.gguf}"
export STELNETTTS_MODEL_MOSS_TTS="${STELNETTTS_MODEL_MOSS_TTS:-$STELNETTTS_MODELS_DIR/moss-tts-v1.5-q4_k.gguf}"
export STELNETTTS_MODEL_MOSS_TTS_CODEC="${STELNETTTS_MODEL_MOSS_TTS_CODEC:-$STELNETTTS_MODELS_DIR/moss-tts-v1.5-codec.gguf}"
export STELNETTTS_MODEL_MOSS_TTS_LOCAL="${STELNETTTS_MODEL_MOSS_TTS_LOCAL:-$STELNETTTS_MODELS_DIR/moss-tts-local-v1.5-q4_k.gguf}"
export STELNETTTS_MODEL_MOSS_TTS_LOCAL_CODEC="${STELNETTTS_MODEL_MOSS_TTS_LOCAL_CODEC:-$STELNETTTS_MODELS_DIR/moss-tts-local-v1.5-codec.gguf}"

# ARK-ASR-3B (AutoArk-AI/ARK-ASR-3B): Whisper-large-v3 enc (partial RoPE) + Qwen2.5-3B LM.
# ⚠️ experimental/WIP — CPU only. See PLAN.md §ARK.
export STELNETTTS_MODEL_ARK_ASR="${STELNETTTS_MODEL_ARK_ASR:-$STELNETTTS_MODELS_DIR/ark-asr-3b-q8_0.gguf}"

# Mini-Omni2 (gpt-omni/mini-omni2): Whisper-small + Qwen2-0.5B
export STELNETTTS_MODEL_MINI_OMNI2="${STELNETTTS_MODEL_MINI_OMNI2:-$STELNETTTS_MODELS_DIR/mini-omni2-q4_k.gguf}"
export STELNETTTS_MODEL_SNAC="${STELNETTTS_MODEL_SNAC:-$STELNETTTS_MODELS_DIR/snac-24khz.gguf}"

# ── WeSpeaker ResNet34-LM (#324 foxnose diarization speaker embedder) ──
export STELNETTTS_MODEL_WESPEAKER="${STELNETTTS_MODEL_WESPEAKER:-$STELNETTTS_MODELS_DIR/wespeaker-resnet34-lm.gguf}"

# ── GigaAM-v3 (ai-sage/GigaAM-v3, Russian ASR) ──
# Default to the e2e_rnnt revision — best WER, and the only one that emits
# punctuation + casing. The fixture is GigaAM's own example.wav:
#   curl -o example.wav https://cdn.chatwm.opensmodel.sberdevices.ru/GigaAM/example.wav
export STELNETTTS_MODEL_GIGAAM="${STELNETTTS_MODEL_GIGAAM:-$STELNETTTS_MODELS_DIR/gigaam-v3-e2e-rnnt-q4_k.gguf}"
export STELNETTTS_MODEL_GIGAAM_F16="${STELNETTTS_MODEL_GIGAAM_F16:-$STELNETTTS_MODELS_DIR/gigaam-v3-e2e-rnnt-f16.gguf}"
export STELNETTTS_FIXTURE_GIGAAM="${STELNETTTS_FIXTURE_GIGAAM:-$STELNETTTS_MODELS_DIR/fixtures/gigaam-example.wav}"

# ── Nemotron (streaming ASR) ──
export STELNETTTS_MODEL_NEMOTRON="${STELNETTTS_MODEL_NEMOTRON:-$STELNETTTS_MODELS_DIR/nemotron-3.5-asr-streaming-0.6b-q4_k.gguf}"
export STELNETTTS_MODEL_NEMOTRON_F16="${STELNETTTS_MODEL_NEMOTRON_F16:-$STELNETTTS_MODELS_DIR/nemotron-3.5-asr-streaming-0.6b-f16.gguf}"

# ── LFM2-Audio ──
export STELNETTTS_MODEL_LFM2="${STELNETTTS_MODEL_LFM2:-$STELNETTTS_MODELS_DIR/lfm2-audio-1.5b-q5_k.gguf}"

# ── TADA TTS (talker + TADA codec companion) ──
export STELNETTTS_MODEL_TADA="${STELNETTTS_MODEL_TADA:-$STELNETTTS_MODELS_DIR/tada-tts-1b-q4_k.gguf}"
export STELNETTTS_MODEL_TADA_CODEC="${STELNETTTS_MODEL_TADA_CODEC:-$STELNETTTS_MODELS_DIR/tada-codec-f16.gguf}"

# ── KugelAudio (7B audio understanding) ──
export STELNETTTS_MODEL_KUGELAUDIO="${STELNETTTS_MODEL_KUGELAUDIO:-$STELNETTTS_MODELS_DIR/kugelaudio-0-open-f16.gguf}"

# ── MeloTTS (VITS2) ──
export STELNETTTS_MODEL_MELOTTS="${STELNETTTS_MODEL_MELOTTS:-$STELNETTTS_MODELS_DIR/melotts-en-v2-f16.gguf}"

# ── Dia TTS ──
export STELNETTTS_MODEL_DIA="${STELNETTTS_MODEL_DIA:-$STELNETTTS_MODELS_DIR/dia-1.6b-q4_k.gguf}"

# ── OuteTTS + WavTokenizer ──
export STELNETTTS_MODEL_OUTETTS="${STELNETTTS_MODEL_OUTETTS:-$STELNETTTS_MODELS_DIR/outetts-0.3-1b-q4k-final.gguf}"
export STELNETTTS_MODEL_WAVTOK="${STELNETTTS_MODEL_WAVTOK:-$STELNETTTS_MODELS_DIR/wavtokenizer-decoder-f16.gguf}"

# ── Sidon speech restoration ──
export STELNETTTS_MODEL_SIDON="${STELNETTTS_MODEL_SIDON:-$STELNETTTS_MODELS_DIR/sidon-v0.1-f16.gguf}"

# ── VoxCPM2 AudioVAE speech upscaler ──
export STELNETTTS_MODEL_VOXCPM2_VAE="${STELNETTTS_MODEL_VOXCPM2_VAE:-$STELNETTTS_MODELS_DIR/voxcpm2-vae-f32.gguf}"
# Optional full-model path for the simultaneous TTS + upscaler lifecycle test.
export STELNETTTS_MODEL_VOXCPM2_FULL="${STELNETTTS_MODEL_VOXCPM2_FULL:-}"

# ── CREPE monophonic F0 / pitch (Xenna/crepe-GGUF) ──
# `tiny` is the shipping default: `full` is 38x the compute for the same
# geometry (see docs/music-transcription/PLAN.md). STELNETTTS_MODEL_CREPE_FULL is
# only read by manual runs of test-crepe-parity, not by ctest.
export STELNETTTS_MODEL_CREPE="${STELNETTTS_MODEL_CREPE:-$STELNETTTS_MODELS_DIR/crepe-tiny-f16.gguf}"
export STELNETTTS_MODEL_CREPE_FULL="${STELNETTTS_MODEL_CREPE_FULL:-$STELNETTTS_MODELS_DIR/crepe-full-f16.gguf}"

# ── BTC chord recognition (Xenna/btc-chords-GGUF) ──
# NON-COMMERCIAL WEIGHTS. The BTC checkpoints are CC-BY-NC-SA (trained on
# Isophonics / Robbie Williams / UsPop2002 annotations) even though the
# upstream code and this library are MIT. Downloading them requires
# --accept-license cc-by-nc-sa-4.0 (or STELNETTTS_ACCEPT_LICENSE).
# The 170-class model is the default: it collapses to maj/min with
# STELNETTTS_BTC_MAJ_MIN=1, whereas the 25-class one can never be expanded.
# ── RVC voice conversion (§CB1) ──
# LICENCE VARIES PER CHECKPOINT. RVC's code is MIT but circulating voice models
# do not share one licence; the GGUF carries its own tag and the registry gate
# matches on it. The pretrained base (lj1995/VoiceConversionWebUI
# pretrained_v2/f0G40k.pth) is what the parity work used.
# No CLI verb: the input is ContentVec features, so the session C ABI is the
# only surface. See docs/music-transcription/SVC_RECORD_SHAPES.md.
export STELNETTTS_MODEL_RVC="${STELNETTTS_MODEL_RVC:-$STELNETTTS_MODELS_DIR/rvc-40k-f32.gguf}"

export STELNETTTS_MODEL_BTC_CHORDS="${STELNETTTS_MODEL_BTC_CHORDS:-$STELNETTTS_MODELS_DIR/btc-chords-large-f32.gguf}"
# TabCNN guitar tablature (--tab). CC BY 4.0 weights, Xenna/tabcnn-GGUF.
export STELNETTTS_MODEL_TABCNN="${STELNETTTS_MODEL_TABCNN:-$STELNETTTS_MODELS_DIR/tabcnn-f16.gguf}"

echo "Live test env configured (STELNETTTS_MODELS_DIR=$STELNETTTS_MODELS_DIR)"

# cielvox2-tts live tests
export STELNETTTS_MODEL_QWEN3_TTS="${STELNETTTS_MODEL_QWEN3_TTS:-$STELNETTTS_MODELS_DIR/cielvox2-tts-0.6b-q4_k.gguf}"

# omnivoice live tests (#13273): style-token parity + three-surface parity.
# Both SKIP cleanly when these are unset, and the tokenizer must be the F16
# build — the q8_0 one loads and synthesizes but cannot be read back as f32, so
# a reference encode yields garbage codes (and poisons the content-addressed
# voice cache for later F16 runs).
export STELNETTTS_TEST_OMNIVOICE_MODEL="${STELNETTTS_TEST_OMNIVOICE_MODEL:-$STELNETTTS_MODELS_DIR/omnivoice-q4_k.gguf}"
export STELNETTTS_TEST_OMNIVOICE_TOKENIZER="${STELNETTTS_TEST_OMNIVOICE_TOKENIZER:-$STELNETTTS_MODELS_DIR/omnivoice-tokenizer-f16.gguf}"
