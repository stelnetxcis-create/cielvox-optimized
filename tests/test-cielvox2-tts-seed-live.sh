#!/bin/bash
# Live seed regression for cielvox2-tts /v1/audio/speech.
#
# Requires:
#   STELNETTTS_TEST_CIELVOX2_TTS_MODEL=/path/to/cielvox2-tts-customvoice.gguf
#   STELNETTTS_TEST_CIELVOX2_TTS_CODEC=/path/to/cielvox2-tts-tokenizer-12hz.gguf
# Optional:
#   STELNETTTS_TEST_CIELVOX2_TTS_VOICE=<speaker-name> (default: first model speaker)
#   PORT=11445

set -uo pipefail
cd "$(dirname "$0")/.."

STELNET_ASR=""
for cand in build-ninja-compile/bin/stelnettts build/bin/stelnettts ./bin/stelnettts; do
    if [ -x "$cand" ]; then STELNET_ASR="$cand"; break; fi
done
if [ -z "$STELNET_ASR" ]; then
    echo "ERROR: stelnettts binary not found. Build first."
    exit 2
fi

MODEL="${STELNETTTS_TEST_CIELVOX2_TTS_MODEL:-}"
CODEC="${STELNETTTS_TEST_CIELVOX2_TTS_CODEC:-}"
VOICE="${STELNETTTS_TEST_CIELVOX2_TTS_VOICE:-}"
PORT="${PORT:-11445}"

if [ -z "$MODEL" ] || [ ! -f "$MODEL" ]; then
    echo "SKIP: set STELNETTTS_TEST_CIELVOX2_TTS_MODEL to a cielvox2-tts CustomVoice GGUF"
    exit 0
fi
if [ -z "$CODEC" ] || [ ! -f "$CODEC" ]; then
    echo "SKIP: set STELNETTTS_TEST_CIELVOX2_TTS_CODEC to cielvox2-tts-tokenizer-12hz.gguf"
    exit 0
fi

PASS=0
FAIL=0
SERVER_PID=""
SERVER_LOG=""
OUT_DIR=$(mktemp -d -t stelnettts-cielvox2-tts-seed.XXXXXX)
trap 'if [ -n "$SERVER_PID" ]; then kill "$SERVER_PID" 2>/dev/null || true; fi; if [ -n "$SERVER_LOG" ]; then rm -f "$SERVER_LOG"; fi; rm -rf "$OUT_DIR"' EXIT

hash_file() {
    shasum -a 256 "$1" | awk '{print $1}'
}

assert_equal() {
    local name="$1" a="$2" b="$3"
    if [ "$a" = "$b" ]; then
        echo "  PASS $name"
        PASS=$((PASS + 1))
    else
        echo "  FAIL $name"
        echo "       a: $a"
        echo "       b: $b"
        FAIL=$((FAIL + 1))
    fi
}

assert_not_equal() {
    local name="$1" a="$2" b="$3"
    if [ "$a" != "$b" ]; then
        echo "  PASS $name"
        PASS=$((PASS + 1))
    else
        echo "  FAIL $name: both hashes were $a"
        FAIL=$((FAIL + 1))
    fi
}

post_speech() {
    local seed="$1" out="$2"
    local voice_json=""
    if [ -n "$VOICE" ]; then
        voice_json=", \"voice\": \"$VOICE\""
    fi
    curl -sf -X POST "http://127.0.0.1:$PORT/v1/audio/speech" \
        -H "Content-Type: application/json" \
        -d "{\"model\":\"tts-1\",\"input\":\"Good morning. This is a qwen seed test.\",\"response_format\":\"wav\",\"seed\":$seed$voice_json}" \
        -o "$out"
}

echo "=== cielvox2-tts /v1/audio/speech seed regression ==="
SERVER_LOG=$(mktemp -t stelnettts-cielvox2-tts-seed-server.XXXXXX)
CIELVOX2_TTS_SEED=999 "$STELNET_ASR" --server --backend cielvox2-tts-customvoice \
    -m "$MODEL" --codec-model "$CODEC" --host 127.0.0.1 --port "$PORT" --no-prints \
    > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!

for _ in $(seq 1 120); do
    if curl -sf "http://127.0.0.1:$PORT/health" >/dev/null 2>&1; then
        break
    fi
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "  FAIL server died during startup"
        cat "$SERVER_LOG"
        exit 1
    fi
    sleep 1
done

if ! curl -sf "http://127.0.0.1:$PORT/health" >/dev/null 2>&1; then
    echo "  FAIL server did not become healthy"
    cat "$SERVER_LOG"
    exit 1
fi

post_speech 123 "$OUT_DIR/a.wav" || { echo "  FAIL request seed=123 #1"; cat "$SERVER_LOG"; exit 1; }
post_speech 123 "$OUT_DIR/b.wav" || { echo "  FAIL request seed=123 #2"; cat "$SERVER_LOG"; exit 1; }
post_speech 456 "$OUT_DIR/c.wav" || { echo "  FAIL request seed=456"; cat "$SERVER_LOG"; exit 1; }

HA=$(hash_file "$OUT_DIR/a.wav")
HB=$(hash_file "$OUT_DIR/b.wav")
HC=$(hash_file "$OUT_DIR/c.wav")

assert_equal "same request seed is bit-identical even when CIELVOX2_TTS_SEED is set" "$HA" "$HB"
assert_not_equal "different request seed changes cielvox2-tts WAV hash" "$HA" "$HC"

echo
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
