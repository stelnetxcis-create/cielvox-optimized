#!/bin/bash
# tests/test-speaker-id-live.sh — live integration tests for speaker labeling.
#
# Covers both paths:
#   * session-scoped clustering: --diarize-speakers (no DB, no names), and
#   * the opt-in biometric named-profile path's CONSENT GATE
#     (--enroll-speaker / --speaker-db require --speaker-db-consent).
#
# Usage:
#   bash tests/test-speaker-id-live.sh <stelnettts-binary>
#
# Inputs (all optional — the test SKIPs cleanly when prerequisites are
# missing so it is safe to run in any environment):
#   STELNETTTS_TEST_AUDIO   path to a 16 kHz mono wav (else common paths tried)
#   STELNETTTS_TEST_MODEL   ASR model spec for -m (default: auto / auto-download)
#
# Models (TitaNet for enrollment, ASR for transcription) auto-download on
# first use; the suite SKIPs the model-dependent cases if resolution fails
# (e.g. offline), so only real regressions FAIL.

set -u

STELNET_ASR="${1:-${STELNET_ASR:-}}"
[ -z "$STELNET_ASR" ] && STELNET_ASR="$(command -v stelnettts || true)"
if [ -z "$STELNET_ASR" ] || [ ! -x "$STELNET_ASR" ]; then
    echo "SKIP: stelnettts binary not provided/executable"
    exit 0
fi

# Locate test audio.
AUDIO="${STELNETTTS_TEST_AUDIO:-}"
if [ -z "$AUDIO" ]; then
    for c in samples/jfk.wav ./jfk.wav \
             /Volumes/backups/code/audio_samples/en/*.wav \
             "$HOME/stelnettts-live-cache"/*.wav; do
        [ -f "$c" ] && AUDIO="$c" && break
    done
fi
if [ -z "$AUDIO" ] || [ ! -f "$AUDIO" ]; then
    echo "SKIP: no test audio (set STELNETTTS_TEST_AUDIO=<16k wav>)"
    exit 0
fi

MODEL="${STELNETTTS_TEST_MODEL:-auto}"
# Keep scratch off /tmp (project rule); CWD is the repo root under ctest.
WORK="$(mktemp -d "${STELNETTTS_SCRATCH_DIR:-.}/spkid.XXXXXX")" || { echo "SKIP: cannot make scratch dir"; exit 0; }
DB="$WORK/voiceprints"
trap 'rm -rf "$WORK"' EXIT

PASS=0
FAIL=0
ok() { echo "[PASS] $1"; PASS=$((PASS + 1)); }
no() { echo "[FAIL] $1"; FAIL=$((FAIL + 1)); }
finish() {
    echo "=== $PASS passed, $FAIL failed ==="
    [ "$FAIL" -eq 0 ]
}

echo "=== Speaker-ID live tests ==="
echo "binary: $STELNET_ASR"
echo "audio:  $AUDIO"
echo "model:  $MODEL"
echo ""

# ── Test 1: enrollment is REFUSED without --speaker-db-consent ───────────────
# The biometric gate must fire (exit 25) and must NOT write a voiceprint.
OUT1="$("$STELNET_ASR" -m "$MODEL" -f "$AUDIO" --enroll-speaker TestUser --speaker-db "$DB" 2>&1)"
RC1=$?
if echo "$OUT1" | grep -qiE "download failed|could not resolve|failed to (resolve|download|load)|no such model|curl|wget"; then
    echo "SKIP: ASR model '$MODEL' unavailable (offline / no network) — skipping live cases"
    exit 0
fi
if [ "$RC1" -eq 25 ] && echo "$OUT1" | grep -q "speaker-db-consent"; then
    ok "enroll without --speaker-db-consent is refused (exit 25, consent message)"
else
    no "enroll without consent: expected exit 25 + consent message (got rc=$RC1)"
    echo "$OUT1" | tail -3
fi
if [ ! -e "$DB/TestUser.spkr" ]; then
    ok "refused enrollment wrote no voiceprint file"
else
    no "refused enrollment leaked a .spkr file"
fi

# ── Test 2: enroll WITH consent creates a voiceprint (.spkr) ─────────────────
OUT2="$("$STELNET_ASR" -m "$MODEL" -f "$AUDIO" --enroll-speaker TestUser --speaker-db "$DB" --speaker-db-consent 2>&1)"
if [ -f "$DB/TestUser.spkr" ]; then
    ok "enroll with consent created TestUser.spkr"
elif echo "$OUT2" | grep -qiE "resolve|download|not found|failed to load"; then
    echo "SKIP: TitaNet embedder unavailable (offline?) — skipping remaining DB cases"
    finish; exit $?
else
    no "enroll with consent did not create a .spkr"
    echo "$OUT2" | tail -3
fi

# ── Test 3: --speaker-db matching is IGNORED without consent ─────────────────
# Even with a populated DB, the 1:N match must not run; a notice is printed.
OUT3="$("$STELNET_ASR" -m "$MODEL" -f "$AUDIO" --diarize --speaker-db "$DB" 2>&1)"
if echo "$OUT3" | grep -q "ignored"; then
    ok "--speaker-db without consent is ignored (prints notice)"
else
    no "--speaker-db without consent: expected an 'ignored' notice"
    echo "$OUT3" | tail -3
fi

# ── Test 4: session-scoped --diarize-speakers emits (speaker N) labels ───────
# No DB, no names — just stable per-recording cluster labels.
OUT4="$("$STELNET_ASR" -m "$MODEL" -f "$AUDIO" --diarize-speakers 2>&1)"
if echo "$OUT4" | grep -q "(speaker"; then
    ok "--diarize-speakers emits (speaker N) labels"
else
    no "--diarize-speakers produced no (speaker N) label"
    echo "$OUT4" | tail -3
fi

echo ""
finish
