"""Smoke test: the maintained shared C-ABI symbol set is reachable from
the Python ctypes binding.

Does NOT instantiate models or run inference — purely checks that the
binding declares (and can look up) all 153 symbols in this smoke-test set.
Requires STELNETTTS_LIB_PATH pointing at a built libstelnettts.{so,dylib}.

KNOWN GAP: this list is a curated subset, NOT the full export surface.
src/stelnettts_c_api.cpp currently declares 200 distinct CA_EXPORT symbols, so
~47 are unlisted and this test would not notice if a binding stopped exposing
one of them. Treat a green run as "the listed symbols are reachable", not as
"the ABI is fully covered". When adding symbols here, prefer closing the gap
over matching the existing count. Regenerate the true export list with:

    grep -oE 'CA_EXPORT[[:space:]]+[A-Za-z_]+[[:space:]*]+stelnettts_[A-Za-z0-9_]+' \\
        src/stelnettts_c_api.cpp | grep -oE 'stelnettts_[A-Za-z0-9_]+' | sort -u

    STELNETTTS_LIB_PATH=build/src/libstelnettts.so python -m pytest tests/test_binding_parity.py -v
"""

import ctypes
import os
import sys

import pytest

# Maintained shared C-ABI smoke-test symbols (sorted; additions can be found with:
#   grep -oP 'CA_EXPORT\s+\w+[\s*]+\K(stelnettts_\w+)' src/stelnettts_c_api.cpp | sort -u
# )
ALL_SYMBOLS = [
    "stelnettts_align_result_free",
    "stelnettts_align_result_n_words",
    "stelnettts_align_result_word_t0",
    "stelnettts_align_result_word_t1",
    "stelnettts_align_words_abi",
    "stelnettts_cache_dir_abi",
    "stelnettts_cache_ensure_file_abi",
    "stelnettts_detect_backend_from_gguf",
    "stelnettts_detect_language",
    "stelnettts_detect_language_pcm",
    "stelnettts_diarize_segments_abi",
    "stelnettts_enhance_audio_rnnoise",
    "stelnettts_kokoro_lang_has_native_voice_abi",
    "stelnettts_kokoro_lang_is_german_abi",
    "stelnettts_kokoro_resolve_fallback_voice_abi",
    "stelnettts_kokoro_resolve_model_for_lang_abi",
    "stelnettts_lcs_dedup_prefix_count",
    "stelnettts_parakeet_free",
    "stelnettts_parakeet_init",
    "stelnettts_parakeet_result_free",
    "stelnettts_parakeet_result_n_tokens",
    "stelnettts_parakeet_result_n_words",
    "stelnettts_parakeet_result_token_p",
    "stelnettts_parakeet_result_token_t0",
    "stelnettts_parakeet_result_token_t1",
    "stelnettts_parakeet_result_word_t0",
    "stelnettts_parakeet_result_word_t1",
    "stelnettts_parakeet_transcribe",
    "stelnettts_params_set_alt_n",
    "stelnettts_params_set_best_of",
    "stelnettts_params_set_detect_language",
    "stelnettts_params_set_initial_prompt",
    "stelnettts_params_set_language",
    "stelnettts_params_set_max_len",
    "stelnettts_params_set_max_tokens",
    "stelnettts_params_set_no_context",
    "stelnettts_params_set_n_threads",
    "stelnettts_params_set_print_progress",
    "stelnettts_params_set_print_realtime",
    "stelnettts_params_set_print_special",
    "stelnettts_params_set_print_timestamps",
    "stelnettts_params_set_single_segment",
    "stelnettts_params_set_split_on_word",
    "stelnettts_params_set_suppress_blank",
    "stelnettts_params_set_tdrz",
    "stelnettts_params_set_temperature",
    "stelnettts_params_set_token_timestamps",
    "stelnettts_params_set_translate",
    "stelnettts_params_set_vad",
    "stelnettts_params_set_vad_min_silence_ms",
    "stelnettts_params_set_vad_min_speech_ms",
    "stelnettts_params_set_vad_model_path",
    "stelnettts_params_set_vad_threshold",
    "stelnettts_pcm_free",
    "stelnettts_punc_free",
    "stelnettts_punc_free_text",
    "stelnettts_punc_init",
    "stelnettts_pyannote_cache_apply_abi",
    "stelnettts_pyannote_cache_compute_abi",
    "stelnettts_pyannote_cache_free_abi",
    "stelnettts_registry_default_bundle_artifact_abi",
    "stelnettts_registry_default_bundle_info_abi",
    "stelnettts_registry_list_backends_abi",
    "stelnettts_registry_lookup_abi",
    "stelnettts_registry_lookup_by_filename_abi",
    "stelnettts_session_available_backends",
    "stelnettts_session_close",
    "stelnettts_session_detect_language",
    "stelnettts_session_is_custom_voice",
    "stelnettts_session_is_voice_design",
    "stelnettts_session_kokoro_clear_phoneme_cache",
    "stelnettts_session_n_speakers",
    "stelnettts_session_open",
    "stelnettts_session_open_explicit",
    "stelnettts_session_open_with_params",
    "stelnettts_session_result_free",
    "stelnettts_session_result_n_segments",
    "stelnettts_session_result_n_words",
    "stelnettts_session_result_segment_t0",
    "stelnettts_session_result_segment_t1",
    "stelnettts_session_result_word_alt_p",
    "stelnettts_session_result_word_n_alts",
    "stelnettts_session_result_word_p",
    "stelnettts_session_result_word_t0",
    "stelnettts_session_result_word_t1",
    "stelnettts_session_set_alt_n",
    "stelnettts_session_set_ask",
    "stelnettts_session_set_beam_size",
    "stelnettts_session_set_best_of",
    "stelnettts_session_set_cfg_weight",
    "stelnettts_session_set_codec_path",
    "stelnettts_session_set_exaggeration",
    "stelnettts_session_set_fallback_thresholds",
    "stelnettts_session_set_frequency_penalty",
    "stelnettts_session_set_grammar_text",
    "stelnettts_session_set_instruct",
    "stelnettts_session_set_length_scale",
    "stelnettts_session_set_max_new_tokens",
    "stelnettts_session_set_max_speech_tokens",
    "stelnettts_session_set_min_p",
    "stelnettts_session_set_punc_model",
    "stelnettts_session_set_punctuation",
    "stelnettts_session_set_repetition_penalty",
    "stelnettts_session_set_sensitivity",
    "stelnettts_session_set_source_language",
    "stelnettts_session_set_speaker_name",
    "stelnettts_session_set_target_language",
    "stelnettts_session_set_temperature",
    "stelnettts_session_set_top_p",
    "stelnettts_session_set_tts_reference_language",
    # EU AI Act Art. 50(4): whose voice a PRESET voice is. Listed here so the
    # parity suite proves every binding can actually resolve it — the mechanism
    # shipped reachable only from the C ABI once already.
    "stelnettts_session_set_speaker_identity",
    "stelnettts_session_set_translate",
    "stelnettts_session_set_tts_seed",
    "stelnettts_session_set_tts_steps",
    "stelnettts_session_set_voice",
    "stelnettts_session_set_whisper_decode_extras",
    "stelnettts_session_stream_open",
    "stelnettts_session_synthesize",
    "stelnettts_session_transcribe",
    "stelnettts_session_transcribe_lang",
    "stelnettts_session_transcribe_vad",
    "stelnettts_session_transcribe_vad_lang",
    "stelnettts_session_translate_text",
    "stelnettts_session_translate_text_free",
    "stelnettts_speaker_cluster_abi",
    "stelnettts_speaker_db_count",
    "stelnettts_speaker_db_enroll2",
    "stelnettts_speaker_db_free",
    "stelnettts_speaker_db_match",
    "stelnettts_speaker_db_open",
    "stelnettts_speaker_embedder_dim_abi",
    "stelnettts_speaker_embedder_embed_abi",
    "stelnettts_speaker_embedder_free_abi",
    "stelnettts_speaker_embedder_make_abi",
    "stelnettts_stream_close",
    "stelnettts_stream_feed",
    "stelnettts_stream_flush",
    "stelnettts_stream_get_text",
    "stelnettts_stream_open",
    "stelnettts_stream_set_live_decode",
    "stelnettts_text_detect_language",
    "stelnettts_titanet_cosine_sim",
    "stelnettts_titanet_embed",
    "stelnettts_titanet_free",
    "stelnettts_titanet_init",
    "stelnettts_token_alt_id",
    "stelnettts_token_alt_p",
    "stelnettts_token_alt_text",
    "stelnettts_token_n_alts",
    "stelnettts_token_p",
    "stelnettts_token_t0",
    "stelnettts_token_t1",
    "stelnettts_vad_free",
    "stelnettts_vad_segments",
    "stelnettts_vad_slices",
]


@pytest.fixture(scope="module")
def lib():
    lib_path = os.environ.get("STELNETTTS_LIB_PATH")
    if not lib_path:
        # Try common build paths
        for candidate in [
            "build/src/libstelnettts.so",
            "/mnt/volume1/build-main/src/libstelnettts.so",
            "build/libstelnettts.so",
        ]:
            if os.path.exists(candidate):
                lib_path = candidate
                break
    if not lib_path or not os.path.exists(lib_path):
        pytest.skip("libstelnettts not found — set STELNETTTS_LIB_PATH")
    return ctypes.CDLL(lib_path)


@pytest.mark.parametrize("symbol", ALL_SYMBOLS)
def test_symbol_resolves(lib, symbol):
    """Every CA_EXPORT symbol from stelnettts_c_api.cpp must be resolvable."""
    assert hasattr(lib, symbol), f"symbol {symbol} not found in loaded libstelnettts"


def test_symbol_count(lib):
    """Sanity-check accidental edits to the maintained symbol set."""
    assert len(ALL_SYMBOLS) == 155, f"expected 155 symbols, got {len(ALL_SYMBOLS)}"


def test_python_binding_imports():
    """The Python binding module must import without error."""
    # This validates syntax and top-level structure.
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))
    import ast
    binding_path = os.path.join(
        os.path.dirname(__file__), "..", "python", "stelnettts", "_binding.py"
    )
    with open(binding_path) as f:
        ast.parse(f.read())


def test_python_binding_declares_all_symbols():
    """Every CA_EXPORT symbol must appear somewhere in _binding.py."""
    binding_path = os.path.join(
        os.path.dirname(__file__), "..", "python", "stelnettts", "_binding.py"
    )
    with open(binding_path) as f:
        content = f.read()
    missing = [s for s in ALL_SYMBOLS if s not in content]
    assert missing == [], f"Python binding missing symbols: {missing}"
