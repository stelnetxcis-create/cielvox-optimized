# stelnettts-sys

Raw FFI bindings for [StelnetTTS](https://github.com/Cyna/StelnetTTS) — lightweight on-device speech recognition via ggml.

This crate is the raw `extern "C"` FFI shim. Its `build.rs` builds `libstelnettts`
from source with cmake by default, or links a pre-built copy when
`STELNETTTS_LIB_DIR` is set.

## Install

On [crates.io](https://crates.io/crates/stelnettts-sys). `build.rs` needs either
the StelnetTTS C/C++ sources (to cmake `libstelnettts`) or a pre-built copy. To
build from source, depend via git so the sources are present:

```toml
[dependencies]
stelnettts-sys = { git = "https://github.com/Cyna/StelnetTTS" }
```

To use the crates.io release you must link a **pre-built** library, since the
package does not vendor the sources — build/install it and set the linker path:

```toml
[dependencies]
stelnettts-sys = "0.8"
```

```bash
git clone https://github.com/Cyna/StelnetTTS
cd StelnetTTS && cmake -B build && cmake --build build -j && sudo cmake --install build
export STELNETTTS_LIB_DIR=/path/to/lib   # e.g. /usr/local/lib
```

### Prebuilt release bundle (no build)

The [Releases](https://github.com/Cyna/StelnetTTS/releases) page ships
relocatable `libstelnettts-<platform>[-cuda|-vulkan].tar.gz` bundles. Extract one
and point **`STELNETTTS_SYS_LIB_DIR`** at its root — `build.rs` finds the import
lib and skips cmake. The bundle's tag must match the crate version (ABI); the
`-cuda`/`-vulkan` bundles carry the matching ggml backend libraries.

**Linux / macOS** — libs are flattened into `lib/`:

```bash
export STELNETTTS_SYS_LIB_DIR=/path/to/libstelnettts-linux-x86_64
# runtime (if the rpath doesn't resolve after moving the bundle):
export LD_LIBRARY_PATH=$STELNETTTS_SYS_LIB_DIR/lib:$LD_LIBRARY_PATH
```

**Windows** — import libs live under `src\Release\` (+ `ggml\src\Release\`) and
the DLLs under `bin\`, so set the linker dir *and* put `bin\` on `PATH` for
runtime:

```powershell
$env:STELNETTTS_SYS_LIB_DIR = "C:\path\to\stelnettts"    # the folder with include\, src\, bin\
$env:PATH = "C:\path\to\stelnettts\bin;$env:PATH"       # stelnettts.dll + ggml*.dll + cublas/cudart
cargo build
```

The legacy `libwhisper` alias also works:

```bash
export STELNETTTS_LIB_NAME=whisper
```

For the safe high-level wrapper see the [`stelnettts`](https://crates.io/crates/stelnettts) crate.

## License

MIT — see [LICENSE](LICENSE).
