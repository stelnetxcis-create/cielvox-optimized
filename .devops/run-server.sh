#!/usr/bin/env bash
set -euo pipefail

ts() {
    date -u +"%Y-%m-%dT%H:%M:%SZ"
}

log() {
    echo "[$(ts)] stelnettts-docker: $*" >&2
}

fail_dir_not_writable() {
    local dir="$1"
    log "ERROR: '$dir' is not writable by uid=$(id -u) gid=$(id -g)."
    log "       Current permissions:"
    ls -ld "$dir" >&2 || true
    log "       If this is a bind mount, create it before starting the container and chown it:"
    log "       mkdir -p cache models && sudo chown -R $(id -u):$(id -g) cache models"
    log "       Or set STELNETTTS_UID/STELNETTTS_GID in .env to match the host owner."
    exit 70
}

ensure_writable_dir() {
    local dir="$1"
    local label="$2"

    if [[ ! -d "$dir" ]]; then
        if ! mkdir -p "$dir" 2>/dev/null; then
            log "ERROR: could not create $label directory '$dir'."
            fail_dir_not_writable "$(dirname "$dir")"
        fi
    fi

    if [[ ! -w "$dir" ]]; then
        fail_dir_not_writable "$dir"
    fi
}

SERVER_HOST="${STELNETTTS_SERVER_HOST:-0.0.0.0}"
SERVER_PORT="${STELNETTTS_PORT:-${STELNETTTS_SERVER_PORT:-8080}}"
MODEL_PATH="${STELNETTTS_MODEL:-/models/model.gguf}"
LANGUAGE="${STELNETTTS_LANGUAGE:-auto}"
BACKEND="${STELNETTTS_BACKEND:-}"
AUTO_DOWNLOAD="${STELNETTTS_AUTO_DOWNLOAD:-0}"
CACHE_DIR="${STELNETTTS_CACHE_DIR:-/cache}"
EXTRA_ARGS="${STELNETTTS_EXTRA_ARGS:-}"
API_KEYS="${STELNETTTS_API_KEYS:-}"
USE_CUDA_COMPAT="${STELNETTTS_USE_CUDA_COMPAT:-0}"
VERBOSE="${STELNETTTS_VERBOSE:-0}"

# ---------------------------------------------------------------------------
# Startup diagnostics. Always emit a concise build banner; emit a fuller
# environment + GPU dump when STELNETTTS_VERBOSE=1. Driver-mismatch tickets
# (#31) need a single self-contained log block to triage; this gives it.
# ---------------------------------------------------------------------------
log "=== stelnettts container starting ==="
if [[ -r /app/build-info.txt ]]; then
    while IFS= read -r line; do
        log "build-info: $line"
    done < /app/build-info.txt
else
    log "build-info: (missing /app/build-info.txt — local build?)"
fi

# nvidia-smi only exists when --gpus is passed AND the host has a
# compatible driver. Failure to find / run it does NOT abort startup —
# the container can run on CPU. Output goes to the log so users hitting
# #31 can paste it directly into their issue.
if command -v nvidia-smi >/dev/null 2>&1; then
    log "nvidia-smi:"
    nvidia-smi 2>&1 | sed 's/^/  /' >&2 || log "nvidia-smi failed (continuing on CPU)"
else
    log "nvidia-smi not present in PATH (no --gpus, or non-CUDA image)"
fi

# Show how the dynamic linker resolves libcuda — this is the proximate
# cause of the #31 mismatch. If it points at /usr/local/cuda/compat the
# image is mis-set-up; if it points at /usr/lib/.../nvidia/ the host
# mount is in effect and CUDA should work.
if command -v ldconfig >/dev/null 2>&1; then
    libcuda_resolved=$(ldconfig -p 2>/dev/null | awk '/libcuda\.so\.1/ {print $NF; exit}')
    if [[ -n "$libcuda_resolved" ]]; then
        log "libcuda.so.1 resolves via ldconfig to: $libcuda_resolved"
    else
        log "libcuda.so.1 not in ldconfig cache (no GPU passthrough)"
    fi
fi

# Vulkan / AMD DRI check — separate from CUDA. If /dev/dri/renderD* nodes
# are visible the Vulkan image can use the host GPU without --gpus.
if ls /dev/dri/renderD* >/dev/null 2>&1; then
    log "DRI render nodes found: $(ls /dev/dri/renderD* 2>/dev/null | tr '\n' ' ')"
    if command -v vulkaninfo >/dev/null 2>&1; then
        log "vulkaninfo:"
        vulkaninfo --summary 2>&1 | grep -E "GPU id|deviceName|driverVersion" | sed 's/^/  /' >&2 || true
    fi
else
    log "no /dev/dri/renderD* nodes (AMD/Intel Vulkan GPU not passed through — add --device=/dev/dri and --group-add video --group-add render to docker run)"
fi

# STELNETTTS_USE_CUDA_COMPAT=1 — opt-in for hosts whose driver is OLDER
# than the runtime needs. The Dockerfile ships compat libs on disk but
# does NOT register them in ldconfig (that broke newer drivers per #31).
# Setting this var prepends the compat dir to LD_LIBRARY_PATH for this
# process tree so the container can fall back to forward-compatibility.
if [[ "$USE_CUDA_COMPAT" == "1" ]]; then
    if [[ -d /usr/local/cuda/compat ]]; then
        log "STELNETTTS_USE_CUDA_COMPAT=1 — prepending /usr/local/cuda/compat to LD_LIBRARY_PATH"
        export LD_LIBRARY_PATH="/usr/local/cuda/compat:${LD_LIBRARY_PATH:-}"
    else
        log "STELNETTTS_USE_CUDA_COMPAT=1 set but /usr/local/cuda/compat does not exist (non-CUDA image)"
    fi
fi

# Verbose mode: full diagnostic dump from the binary itself (build info,
# env, ggml device enumeration). This will trigger CUDA init in-process
# so any GGML_LOG_ERROR from the driver/runtime mismatch lands in the
# same log block as nvidia-smi.
if [[ "$VERBOSE" == "1" ]] || [[ "${STELNETTTS_DIAGNOSTICS:-0}" == "1" ]]; then
    log "running 'stelnettts --diagnostics':"
    stelnettts --diagnostics 2>&1 | sed 's/^/  /' >&2 || log "stelnettts --diagnostics failed (continuing)"
fi

# STELNETTTS_DIAGNOSTICS=1 — run diagnostics only, then exit.
# Useful for troubleshooting GPU issues (#31) without needing a model.
if [[ "${STELNETTTS_DIAGNOSTICS:-0}" == "1" ]]; then
    log "STELNETTTS_DIAGNOSTICS=1 — diagnostics-only mode, exiting."
    exit 0
fi

ensure_writable_dir "$CACHE_DIR" "cache"

if [[ "$AUTO_DOWNLOAD" != "1" ]]; then
    if [[ ! -r "$MODEL_PATH" ]]; then
        log "ERROR: model '$MODEL_PATH' is not readable."
        log "       Mount a model under /models, set STELNETTTS_MODEL to a readable file, or set STELNETTTS_AUTO_DOWNLOAD=1."
        if [[ -e "$(dirname "$MODEL_PATH")" ]]; then
            ls -ld "$(dirname "$MODEL_PATH")" >&2 || true
        fi
        exit 66
    fi
fi

declare -a args
args=(stelnettts --server --host "$SERVER_HOST" --port "$SERVER_PORT" --cache-dir "$CACHE_DIR")

if [[ "$AUTO_DOWNLOAD" == "1" ]]; then
    args+=(-m auto --auto-download)
else
    args+=(-m "$MODEL_PATH")
fi

if [[ -n "$BACKEND" ]]; then
    args+=(--backend "$BACKEND")
fi

if [[ -n "$LANGUAGE" ]]; then
    args+=(-l "$LANGUAGE")
fi

# STELNETTTS_VERBOSE=1 also propagates `-v` to the binary so the per-request
# server logs include the build banner + per-stage timings. Useful for
# bug reports — without this the binary's own log stream is silent on
# the critical CUDA init lines.
if [[ "$VERBOSE" == "1" ]]; then
    args+=(-v)
fi

# API keys are read from STELNETTTS_API_KEYS env var directly by the server
# binary — do NOT pass as --api-keys CLI arg (visible in ps/top, issue #28).
# The env var is already set by docker-compose.yml from the .env file.

if [[ -n "$EXTRA_ARGS" ]]; then
    eval "args+=($EXTRA_ARGS)"
fi

display_args=("${args[@]}")

log "server_host=$SERVER_HOST server_port=$SERVER_PORT backend=${BACKEND:-default} language=${LANGUAGE:-default} auto_download=$AUTO_DOWNLOAD cache_dir=$CACHE_DIR"
if [[ -n "$API_KEYS" ]]; then
    log "api_keys=enabled"
fi
log "launching: ${display_args[*]}"
exec "${args[@]}"
