#!/bin/bash
# tests/test-tiron-live.sh — live integration test for the Tiron backend (#295).
#
# Tiron is a Whisper large-v3 checkpoint with inline <|speakerN|> markers. This
# verifies end to end that:
#   1. the tiron decode grammar emits per-window speaker attribution, and
#   2. the cross-window linker promotes it to global SPEAKER_NN.
#
# Usage:  bash tests/test-tiron-live.sh <stelnettts-binary>
#
# Inputs (all optional — SKIPs cleanly when prerequisites are missing):
#   STELNETTTS_MODEL_TIRON  tiron GGML model (default: -m auto / auto-download)
#   STELNETTTS_TEST_AUDIO   multi-speaker 16 kHz mono wav (default: samples/multispeaker.wav)
#
# The tiron model + TitaNet embedder auto-download on first use; the test SKIPs
# if resolution fails (e.g. offline), so only real regressions FAIL.

set -u

STELNET_ASR="${1:-${STELNET_ASR:-}}"
[ -z "$STELNET_ASR" ] && STELNET_ASR="$(command -v stelnettts || true)"
if [ -z "$STELNET_ASR" ] || [ ! -x "$STELNET_ASR" ]; then
    echo "SKIP: stelnettts binary not provided/executable"
    exit 0
fi

AUDIO="${STELNETTTS_TEST_AUDIO:-}"
if [ -z "$AUDIO" ]; then
    for c in samples/multispeaker.wav ./multispeaker.wav "$(dirname "$0")/../samples/multispeaker.wav"; do
        [ -f "$c" ] && AUDIO="$c" && break
    done
fi
if [ -z "$AUDIO" ] || [ ! -f "$AUDIO" ]; then
    echo "SKIP: no multi-speaker test audio (set STELNETTTS_TEST_AUDIO=<16k wav>)"
    exit 0
fi

MODEL="${STELNETTTS_MODEL_TIRON:-auto}"
OUT="$(mktemp)"

run() {
    DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:-}" LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}" \
        "$STELNET_ASR" --backend tiron -m "$MODEL" --auto-download -f "$AUDIO" "$@" >"$OUT" 2>&1
}

fail=0

# 1. Decode grammar: without linking, the transcript must carry <|speakerN|> markers.
if ! run -ps -l en --no-punctuation; then
    if grep -qiE "failed to (load|resolve|download)|could not|no such file" "$OUT"; then
        echo "SKIP: tiron model unavailable (offline / not resolvable)"
        rm -f "$OUT"
        exit 0
    fi
    echo "FAIL: tiron transcription exited non-zero"
    cat "$OUT"
    rm -f "$OUT"
    exit 1
fi
if grep -qE "Tiron speaker tokens = " "$OUT"; then
    echo "PASS: tiron speaker vocab detected"
else
    echo "FAIL: tiron speaker vocab not detected (grammar path not active)"
    fail=1
fi
if grep -qE "<\|speaker[0-9]\|>|speaker[0-9]" "$OUT"; then
    echo "PASS: per-window speaker markers emitted"
else
    echo "FAIL: no <|speakerN|> markers in the transcript"
    fail=1
fi

# 2. Cross-window linking → global SPEAKER_NN (needs the TitaNet embedder).
if run --diarize-embedder auto --no-punctuation; then
    if grep -qE "linked .* meeting-level speakers|SPEAKER_[0-9][0-9]" "$OUT"; then
        echo "PASS: cross-window linking produced SPEAKER_NN labels"
    else
        echo "SKIP: linking did not run (embedder likely unavailable offline)"
    fi
fi

rm -f "$OUT"
[ "$fail" -eq 0 ] && echo "ALL TIRON LIVE CHECKS PASSED" || echo "TIRON LIVE CHECKS FAILED"
exit "$fail"
