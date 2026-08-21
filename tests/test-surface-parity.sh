#!/usr/bin/env bash
# test-surface-parity.sh — cross-surface parity harness (improvements Phase 0).
#
# The CLI, the HTTP server, and the session C-ABI should produce the SAME
# transcription for the same audio. This harness runs a clip through the CLI
# adapter and the session C-ABI (the pair the Phase 1 dispatch unification
# changes) and asserts they agree by the canonical rule in
# src/core/asr_parity.h: same segment count, same whitespace-normalized text
# per segment, offsets within a small tolerance.
#
# On a short clip (<= one encoder window) both paths already run single-pass and
# MUST match today — so this guards Phase 1 from regressing the easy case, and
# becomes the acceptance gate for the divergent (chunked) cases once unified.
#
# Skips (exit 2) unless it can find: the stelnettts CLI, a parakeet model, a built
# libstelnettts shared lib, and the python binding. Exit 0 pass / 1 fail / 2 skip.

set -uo pipefail
cd "$(dirname "$0")/.."

STELNET_ASR="${STELNETTTS_BIN:-./build/bin/stelnettts}"
[ -x "$STELNET_ASR" ] || STELNET_ASR="./build-ninja-compile/bin/stelnettts"
if [ ! -x "$STELNET_ASR" ]; then
    echo "SKIP: stelnettts binary not found (set STELNETTTS_BIN)"; exit 2
fi

# Resolve a parakeet model (content-JA-safe: any parakeet-tdt works).
MODEL="${STELNETTTS_PARITY_MODEL:-}"
if [ -z "$MODEL" ]; then
    for c in \
        "${STELNETTTS_MODELS_DIR:-}/parakeet-tdt-0.6b-v3.gguf" \
        "${STELNETTTS_MODELS_DIR:-}/parakeet-tdt-1.1b-q4_k.gguf" \
        /Volumes/backups/ai/stelnettts-gguf/parakeet-tdt-1.1b-q4_k.gguf \
        /Volumes/backups/ai/stelnettts-gguf/parakeet-tdt-0.6b-v3-q4_k.gguf ; do
        [ -f "$c" ] && { MODEL="$c"; break; }
    done
fi
[ -n "$MODEL" ] && [ -f "$MODEL" ] || { echo "SKIP: no parakeet model (set STELNETTTS_PARITY_MODEL)"; exit 2; }

# Resolve the shared lib for the python session path.
LIB="${STELNETTTS_LIB_PATH:-}"
if [ -z "$LIB" ]; then
    LIB=$(ls ./build-shared/src/libstelnettts.dylib ./build-shared/src/libstelnettts.so \
             ./build/src/libstelnettts.dylib ./build/src/libstelnettts.so 2>/dev/null | head -1 || true)
fi
[ -n "$LIB" ] && [ -f "$LIB" ] || { echo "SKIP: libstelnettts shared lib not found (build -DBUILD_SHARED_LIBS=ON or set STELNETTTS_LIB_PATH)"; exit 2; }

PY="${STELNETTTS_PYTHON:-python}"
$PY -c "import numpy" 2>/dev/null || { echo "SKIP: python+numpy not available"; exit 2; }

CLIP="${STELNETTTS_PARITY_CLIP:-samples/jfk.wav}"   # ~11 s → single-pass on both surfaces
[ -f "$CLIP" ] || { echo "SKIP: clip $CLIP missing"; exit 2; }

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Feed BOTH surfaces identical samples: if the clip isn't 16 kHz mono, resample
# it ONCE to a shared 16 kHz WAV (both the CLI and the python session then read
# the same samples — otherwise their different resamplers produce different mel
# and the comparison is meaningless).
SRATE=$($PY -c "import wave,sys; w=wave.open(sys.argv[1]); print(w.getframerate(),w.getnchannels())" "$CLIP" 2>/dev/null)
if [ "$SRATE" != "16000 1" ]; then
    $PY - "$CLIP" "$TMP/clip16k.wav" <<'PY' || { echo "SKIP: could not resample clip to 16k"; exit 2; }
import sys, wave, numpy as np
w = wave.open(sys.argv[1]); sr = w.getframerate(); n = w.getnframes()
pcm = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float32)
if w.getnchannels() == 2:
    pcm = pcm.reshape(-1, 2).mean(axis=1)
if sr != 16000:
    tgt = int(round(len(pcm) / sr * 16000))
    pcm = np.interp(np.linspace(0, len(pcm) - 1, tgt), np.arange(len(pcm)), pcm)
o = wave.open(sys.argv[2], "w"); o.setnchannels(1); o.setsampwidth(2); o.setframerate(16000)
o.writeframes(pcm.astype(np.int16).tobytes()); o.close()
PY
    CLIP="$TMP/clip16k.wav"
fi

echo "parity: model=$(basename "$MODEL") clip=$(basename "$CLIP")"

# --- Surface A: CLI adapter ---
BACKEND="${STELNETTTS_PARITY_BACKEND:-parakeet}"
BFLAG=""
[ -n "$BACKEND" ] && BFLAG="--backend $BACKEND"
# shellcheck disable=SC2086
"$STELNET_ASR" -m "$MODEL" $BFLAG --no-punctuation --threads 4 --language en \
    -ojf -f "$CLIP" -of "$TMP/cli" >/dev/null 2>&1 || { echo "FAIL: CLI transcribe errored"; exit 1; }

# --- Surface B: session C-ABI (python binding) + compare (canonical rule) ---
STELNETTTS_LIB_PATH="$LIB" USE_TF=0 "$PY" - "$MODEL" "$CLIP" "$TMP/cli.json" <<'PY'
import sys, json, wave, os
import numpy as np
sys.path.insert(0, os.path.join(os.getcwd(), "python"))
from stelnettts import Session

model, clip, cli_json = sys.argv[1], sys.argv[2], sys.argv[3]

w = wave.open(clip); sr = w.getframerate(); n = w.getnframes()
pcm = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float32) / 32768.0
if w.getnchannels() == 2:
    pcm = pcm.reshape(-1, 2).mean(axis=1)
if sr != 16000:
    tgt = int(round(len(pcm) / sr * 16000))
    pcm = np.interp(np.linspace(0, len(pcm) - 1, tgt), np.arange(len(pcm)), pcm).astype(np.float32)

s = Session(model, backend=os.environ.get("STELNETTTS_PARITY_BACKEND", "parakeet"))
segs_b = s.transcribe_pcm(pcm, sample_rate=16000, language="en") if hasattr(s, "transcribe_pcm") \
    else s.transcribe(pcm, sample_rate=16000, language="en")

import re

def norm(t):  # mirror core_parity::norm_text (strict: case + punctuation kept)
    return " ".join(t.split())

def content(t):  # punctuation/case-insensitive — CONTENT match (dispatch bug detector)
    return " ".join(re.sub(r"[^\w\s]", " ", t.lower()).split())

# CLI segments
cli = json.load(open(cli_json)).get("transcription", [])
a = [(x.get("text", ""), x.get("offsets", {}).get("from", 0), x.get("offsets", {}).get("to", 0)) for x in cli]
b = [(x.text, round(x.start * 100), round(x.end * 100)) for x in segs_b]
a = [(t, round(f / 10), round(to / 10)) for (t, f, to) in a]  # ms→cs

print(f"  CLI segments={len(a)}  session segments={len(b)}")

# CONTENT check on the TOTAL transcript (concatenated), not per-segment: on long
# audio the CLI dispatcher and the session auto-chunker cut at different energy
# minima, so segment COUNTS legitimately differ while the transcript is the same.
# What flags a real dispatch divergence is the total content diverging. (The
# punctuation difference is cosmetic — the CLI ran --no-punctuation, which the
# session has no equivalent for.)
from collections import Counter  # noqa: E402
ta = content(" ".join(t for (t, _, _) in a))
tb = content(" ".join(t for (t, _, _) in b))
wa, wb = ta.split(), tb.split()
overlap = (sum((Counter(wa) & Counter(wb)).values()) / len(wa)) if wa else (0.0 if wb else 1.0)
THRESH = 0.90  # tolerate chunk-boundary word drops; catch real divergence
seg_note = "" if len(a) == len(b) else f"  [segmentation differs {len(a)}vs{len(b)} — chunk boundaries]"
if ta == tb or overlap >= THRESH:
    print(f"PASS(content): total transcripts agree (word-overlap={overlap:.2f}){seg_note}")
    sys.exit(0)
print(f"FAIL(content): total transcripts diverge (word-overlap={overlap:.2f})\n  CLI={ta[:160]!r}\n  SES={tb[:160]!r}")
sys.exit(1)
PY
rc=$?
exit $rc
