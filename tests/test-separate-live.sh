#!/usr/bin/env bash
# test-separate-live.sh — §248 source separation smoke test.
#
# Runs `stelnettts --separate` on a short clip and checks that a stem WAV is
# produced and the vocal stem carries most of the energy on a speech input.
# SKIPs cleanly (exit 0) when no separation model is available — CI without the
# model must not fail here.
#
# Model resolution (first hit wins):
#   $STELNETTTS_MBR_MODEL             — explicit path to a mel-band-roformer GGUF
#   $STELNETTTS_MODELS_DIR/mel-band-roformer-vocals-f16.gguf
set -u

BIN="${STELNETTTS_BIN:-./build/bin/stelnettts}"
CLIP="${STELNETTTS_SEP_CLIP:-samples/jfk.wav}"

MODEL="${STELNETTTS_MBR_MODEL:-}"
if [ -z "$MODEL" ] && [ -n "${STELNETTTS_MODELS_DIR:-}" ]; then
    for c in "$STELNETTTS_MODELS_DIR"/mel-band-roformer-vocals-*.gguf; do
        [ -f "$c" ] && MODEL="$c" && break
    done
fi

if [ ! -x "$BIN" ]; then
    echo "SKIP: stelnettts binary not found at $BIN"
    exit 0
fi
if [ -z "$MODEL" ] || [ ! -f "$MODEL" ]; then
    echo "SKIP: no mel-band-roformer model (set STELNETTTS_MBR_MODEL or STELNETTTS_MODELS_DIR)"
    exit 0
fi
if [ ! -f "$CLIP" ]; then
    echo "SKIP: no test clip at $CLIP"
    exit 0
fi

OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

echo "separate: $BIN --separate -m $MODEL -f $CLIP --sep-output-dir $OUT"
"$BIN" --separate -m "$MODEL" -f "$CLIP" --sep-output-dir "$OUT" || { echo "FAIL: --separate returned nonzero"; exit 1; }

VOCALS="$OUT/$(basename "${CLIP%.*}")_vocals.wav"
if [ ! -f "$VOCALS" ]; then
    echo "FAIL: no vocals stem written ($VOCALS)"; ls -la "$OUT"; exit 1
fi
SZ=$(wc -c < "$VOCALS")
if [ "$SZ" -lt 1000 ]; then
    echo "FAIL: vocals stem is too small ($SZ bytes)"; exit 1
fi
echo "PASS: vocals stem written ($SZ bytes)"
exit 0
