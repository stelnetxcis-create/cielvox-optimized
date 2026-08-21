#ifndef STELNETTTS_PUNC_LOADER_H
#define STELNETTTS_PUNC_LOADER_H

// CLI-layer alias for the shared `--punc-model` resolver. The pure resolution
// table now lives in src/stelnettts_punc_model.h so the C-ABI session layer
// (src/stelnettts_c_api.cpp) can share it too; this header just re-exports it for
// the CLI one-shot path (stelnettts_run.cpp) and the HTTP server
// (stelnettts_server.cpp), which include it by this name.
#include "stelnettts_punc_model.h"

#endif // STELNETTTS_PUNC_LOADER_H
