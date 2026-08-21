# stelnettts

Safe Rust wrapper for [StelnetTTS](https://github.com/Cyna/StelnetTTS) — lightweight on-device speech recognition via ggml.

Supports 17 ASR backends including Whisper, Qwen3-ASR, FastConformer, Canary, Parakeet, Cohere, Granite-Speech, Voxtral, wav2vec2, GLM-ASR, Kyutai-STT, Moonshine, FireRed, OmniASR, and VibeVoice-ASR.

## Install

`stelnettts` and `stelnettts-sys` are on [crates.io](https://crates.io/crates/stelnettts),
but the FFI crate still needs a native `libstelnettts` to link against. Pick one
of two modes:

**A — build from source (git dependency).** `build.rs` cmakes `libstelnettts` for
you (needs `cmake` + a C++ toolchain; the git checkout supplies the sources):

```toml
[dependencies]
stelnettts = { git = "https://github.com/Cyna/StelnetTTS" }
```

**B — link a pre-built library (crates.io dependency).** Build/install
`libstelnettts` once, then point `STELNETTTS_LIB_DIR` at it — this is required
because the crates.io package does **not** vendor the C/C++ sources, so
`build.rs` cannot cmake it:

```toml
[dependencies]
stelnettts = "0.8"
```

```bash
git clone https://github.com/Cyna/StelnetTTS
cd StelnetTTS && cmake -B build && cmake --build build -j && sudo cmake --install build
# then, in your project:  export STELNETTTS_LIB_DIR=/usr/local/lib
```

## Quick start

```rust
use stelnettts::CrispAsr;

let model = CrispAsr::open("ggml-base.en.bin")?;
for seg in model.transcribe_file("audio.wav")? {
    println!("[{:.1}s - {:.1}s] {}", seg.start, seg.end, seg.text);
}
```

See the [main repo](https://github.com/Cyna/StelnetTTS) for full documentation, the model registry, and the CLI.

## License

MIT — see [LICENSE](LICENSE).
