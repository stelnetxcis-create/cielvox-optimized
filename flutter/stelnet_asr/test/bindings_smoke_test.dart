// Smoke test — confirms every 0.2.0 FFI symbol resolves against the freshly
// built libwhisper. Does NOT run real transcription (that needs a model
// download); purely checks the binding surface.
//
// Requires STELNET_ASR_LIB pointing at the built libwhisper:
//   STELNET_ASR_LIB=../../../build/src/libwhisper.dylib dart test/bindings_smoke_test.dart

import 'dart:ffi';
import 'dart:io';

import 'package:stelnet_asr/stelnet_asr.dart' show DiarizeMethod, LidMethod;
import 'package:ffi/ffi.dart';
import 'package:test/test.dart';

DynamicLibrary _openLib() {
  final path = Platform.environment['STELNET_ASR_LIB'];
  if (path != null && path.isNotEmpty) {
    return DynamicLibrary.open(path);
  }
  if (Platform.isMacOS || Platform.isIOS) {
    return DynamicLibrary.open('../../build/src/libwhisper.dylib');
  }
  return DynamicLibrary.open('../../build/src/libwhisper.so');
}

void main() {
  late DynamicLibrary lib;

  setUpAll(() {
    lib = _openLib();
  });

  test('0.1.0 whisper symbols resolve', () {
    for (final s in [
      'whisper_init_from_file_with_params',
      'whisper_free',
      'whisper_full',
      'whisper_full_default_params_by_ref',
      'whisper_context_default_params_by_ref',
      'whisper_full_n_segments',
      'whisper_full_get_segment_text',
      'whisper_full_get_segment_t0',
      'whisper_full_get_segment_t1',
      'whisper_full_get_segment_no_speech_prob',
      'whisper_free_params',
    ]) {
      expect(() => lib.lookup(s), returnsNormally, reason: s);
    }
  });

  test('0.2.0 stelnet_asr_ helpers resolve', () {
    for (final s in [
      'stelnet_asr_params_set_language',
      'stelnet_asr_params_set_translate',
      'stelnet_asr_params_set_detect_language',
      'stelnet_asr_params_set_token_timestamps',
      'stelnet_asr_params_set_n_threads',
      'stelnet_asr_params_set_max_len',
      'stelnet_asr_params_set_split_on_word',
      'stelnet_asr_params_set_no_context',
      'stelnet_asr_params_set_single_segment',
      'stelnet_asr_params_set_print_realtime',
      'stelnet_asr_params_set_print_progress',
      'stelnet_asr_params_set_print_timestamps',
      'stelnet_asr_params_set_print_special',
      'stelnet_asr_params_set_suppress_blank',
      'stelnet_asr_params_set_temperature',
      'stelnet_asr_params_set_initial_prompt',
      'stelnet_asr_token_t0',
      'stelnet_asr_token_t1',
      'stelnet_asr_token_p',
      'stelnet_asr_detect_language',
      'stelnet_asr_vad_segments',
      'stelnet_asr_vad_free',
      // Shared VAD-driven session transcribe (0.4.3+). Merges slices,
      // stitches with 0.1s gaps, remaps timestamps — same algorithm
      // as the CLI's --vad path, reachable from every binding.
      'stelnet_asr_session_transcribe_vad',
      // Language-aware session transcribe (0.4.9+). Accepts an ISO
      // 639-1 code so caller-supplied LID results can feed into the
      // backend's source-language hint.
      'stelnet_asr_session_transcribe_lang',
      'stelnet_asr_session_transcribe_vad_lang',
      // Shared diarization (0.4.5+). Assigns a speaker index to each
      // caller-supplied segment; 4 methods (energy, xcorr, vad-turns,
      // pyannote) share the library path with the CLI.
      'stelnet_asr_diarize_segments_abi',
      // Shared language identification (0.4.6+). Two methods (whisper,
      // silero) share the library path with the CLI.
      'stelnet_asr_detect_language_pcm',
      // Shared CTC / forced-aligner word timings (0.4.7+). Canary-CTC
      // and Qwen3-ForcedAligner paths share one entry point.
      'stelnet_asr_align_words_abi',
      'stelnet_asr_align_result_n_words',
      'stelnet_asr_align_result_word_text',
      'stelnet_asr_align_result_word_t0',
      'stelnet_asr_align_result_word_t1',
      'stelnet_asr_align_result_free',
      // Shared HF download + cache + model registry (0.4.8+).
      'stelnet_asr_cache_ensure_file_abi',
      'stelnet_asr_cache_dir_abi',
      'stelnet_asr_registry_lookup_abi',
      'stelnet_asr_registry_lookup_by_filename_abi',
      'stelnet_asr_registry_default_bundle_info_abi',
      'stelnet_asr_registry_default_bundle_artifact_abi',
      // Canonical C-ABI version symbol (was `stelnet_asr_dart_helpers_version`
      // before the file moved to `src/stelnet_asr_c_api.cpp`).
      'stelnet_asr_c_api_version',
      // Back-compat alias, kept for one release cycle.
      'stelnet_asr_dart_helpers_version',
    ]) {
      expect(() => lib.lookup(s), returnsNormally, reason: s);
    }
  });

  test('0.2.0 whisper language helpers resolve', () {
    for (final s in [
      'whisper_lang_max_id',
      'whisper_lang_id',
      'whisper_lang_str',
      'whisper_full_n_tokens',
      'whisper_full_get_token_text',
      'whisper_full_get_token_p',
    ]) {
      expect(() => lib.lookup(s), returnsNormally, reason: s);
    }
  });

  test('c_api_version: canonical + alias agree on the same string', () {
    // Don't pin a specific version — the C side bumps independently
    // of the Dart binding. The test's real intent is to verify that
    // the deprecated alias `stelnet_asr_dart_helpers_version` keeps
    // returning the same string as the canonical
    // `stelnet_asr_c_api_version` until the alias is removed.
    final versions = <String>[];
    for (final sym in const [
      'stelnet_asr_c_api_version',
      'stelnet_asr_dart_helpers_version', // deprecated alias
    ]) {
      final fn = lib.lookupFunction<Pointer<Utf8> Function(),
          Pointer<Utf8> Function()>(sym);
      final ptr = fn();
      expect(ptr.cast<Uint8>().address, isNot(0), reason: sym);
      final s = ptr.toDartString();
      expect(s, matches(RegExp(r'^\d+\.\d+\.\d+')),
          reason: '$sym should return a semver-shaped string');
      versions.add(s);
    }
    expect(versions[0], versions[1],
        reason: 'canonical + alias must report the same version string');
  });

  test('0.3.0 streaming helpers resolve', () {
    for (final s in [
      'stelnet_asr_stream_open',
      'stelnet_asr_stream_feed',
      'stelnet_asr_stream_flush',
      'stelnet_asr_stream_get_text',
      'stelnet_asr_stream_close',
    ]) {
      expect(() => lib.lookup(s), returnsNormally, reason: s);
    }
  });

  test('0.4.1 audio decoder helpers resolve', () {
    for (final s in [
      'stelnet_asr_audio_load',
      'stelnet_asr_audio_free',
    ]) {
      expect(() => lib.lookup(s), returnsNormally, reason: s);
    }
  });

  test('0.4.0 unified session helpers resolve', () {
    for (final s in [
      'stelnet_asr_session_open',
      'stelnet_asr_session_open_explicit',
      'stelnet_asr_session_backend',
      'stelnet_asr_session_available_backends',
      'stelnet_asr_session_transcribe',
      'stelnet_asr_session_result_n_segments',
      'stelnet_asr_session_result_segment_text',
      'stelnet_asr_session_result_segment_t0',
      'stelnet_asr_session_result_segment_t1',
      'stelnet_asr_session_result_n_words',
      'stelnet_asr_session_result_word_text',
      'stelnet_asr_session_result_word_t0',
      'stelnet_asr_session_result_word_t1',
      'stelnet_asr_session_result_free',
      'stelnet_asr_session_close',
      'stelnet_asr_detect_backend_from_gguf',
    ]) {
      expect(() => lib.lookup(s), returnsNormally, reason: s);
    }
  });

  test('0.5.9 grammar-constrained sampling helpers resolve', () {
    // stelnet_asr_session_set_grammar_text was added in 0.5.9 — it
    // threads a parsed GBNF graph through wparams.grammar_rules
    // on the whisper transcribe dispatch. Symbol-presence test;
    // the real parse-and-bind path is exercised by the
    // StelnetAsrSession.setGrammar() Dart smoke below.
    expect(
        () => lib.lookup('stelnet_asr_session_set_grammar_text'), returnsNormally,
        reason: 'rebuild libstelnet_asr — 0.5.9 grammar setter is missing');
  });

  test('0.5.13 whisper alt-token capture symbols resolve', () {
    // 0.5.13 adds top-N alternative-candidate capture for whisper
    // greedy decode. The C side exposes them at three layers:
    //   * params-level (`stelnet_asr_params_set_alt_n`) for the
    //     low-level transcribePcm path
    //   * session-level sticky setter
    //     (`stelnet_asr_session_set_alt_n`)
    //   * per-token + per-word accessors so consumers can surface
    //     runner-up candidates in tap-to-pick UIs.
    // Pre-0.5.13 dylibs don't have any of these; the Dart wrapper
    // raises UnsupportedError so apps can graceful-degrade.
    for (final s in [
      'stelnet_asr_params_set_alt_n',
      'stelnet_asr_session_set_alt_n',
      'stelnet_asr_token_n_alts',
      'stelnet_asr_token_alt_id',
      'stelnet_asr_token_alt_p',
      'stelnet_asr_token_alt_text',
      'stelnet_asr_session_result_word_n_alts',
      'stelnet_asr_session_result_word_alt_text',
      'stelnet_asr_session_result_word_alt_p',
    ]) {
      expect(() => lib.lookup(s), returnsNormally,
          reason: 'rebuild libstelnet_asr — 0.5.13 alt-token symbol '
              '$s is missing');
    }
  });

  test('0.5.12 audio enhancement helper resolves', () {
    // stelnet_asr_enhance_audio_rnnoise runs RNNoise on a 16 kHz mono
    // float32 buffer (upsample → denoise frames → downsample) as a
    // transcribe pre-step. Pre-0.5.12 dylibs don't have the symbol;
    // the Dart wrapper raises UnsupportedError so apps can
    // graceful-degrade to the un-enhanced PCM.
    expect(() => lib.lookup('stelnet_asr_enhance_audio_rnnoise'), returnsNormally,
        reason: 'rebuild libstelnet_asr — 0.5.12 audio enhancement '
            'helper is missing');
  });

  test('0.5.11 whisper decode-extras setter resolves', () {
    // stelnet_asr_session_set_whisper_decode_extras writes
    // wparams.suppress_nst + suppress_regex + carry_initial_prompt
    // on every whisper transcribe. Pre-0.5.11 dylibs don't have
    // the symbol; the Dart wrapper raises UnsupportedError so
    // apps can graceful-degrade.
    expect(() => lib.lookup('stelnet_asr_session_set_whisper_decode_extras'),
        returnsNormally,
        reason: 'rebuild libstelnet_asr — 0.5.11 whisper decode-extras '
            'setter is missing');
  });

  test('0.5.10 whisper decoder-fallback thresholds setter resolves', () {
    // stelnet_asr_session_set_fallback_thresholds writes the four
    // wparams.*_thold fields + wparams.temperature_inc on every
    // whisper transcribe. Pre-0.5.10 dylibs don't have the
    // symbol; the Dart wrapper raises UnsupportedError so apps
    // can graceful-degrade.
    expect(() => lib.lookup('stelnet_asr_session_set_fallback_thresholds'),
        returnsNormally,
        reason: 'rebuild libstelnet_asr — 0.5.10 fallback-threshold '
            'setter is missing');
  });

  test('C-ABI parity: parakeet + kokoro + vad_slices + lcs symbols resolve',
      () {
    for (final s in [
      'stelnet_asr_parakeet_init',
      'stelnet_asr_parakeet_free',
      'stelnet_asr_parakeet_transcribe',
      'stelnet_asr_parakeet_result_text',
      'stelnet_asr_parakeet_result_n_words',
      'stelnet_asr_parakeet_result_word_text',
      'stelnet_asr_parakeet_result_word_t0',
      'stelnet_asr_parakeet_result_word_t1',
      'stelnet_asr_parakeet_result_n_tokens',
      'stelnet_asr_parakeet_result_token_text',
      'stelnet_asr_parakeet_result_token_t0',
      'stelnet_asr_parakeet_result_token_t1',
      'stelnet_asr_parakeet_result_token_p',
      'stelnet_asr_parakeet_result_free',
      'stelnet_asr_kokoro_lang_is_german_abi',
      'stelnet_asr_kokoro_lang_has_native_voice_abi',
      'stelnet_asr_kokoro_resolve_model_for_lang_abi',
      'stelnet_asr_kokoro_resolve_fallback_voice_abi',
      'stelnet_asr_lcs_dedup_prefix_count',
      'stelnet_asr_vad_slices',
      'stelnet_asr_stream_set_live_decode',
      'stelnet_asr_titanet_cosine_sim',
      'stelnet_asr_session_open_with_params',
      'stelnet_asr_session_translate_text',
      'stelnet_asr_session_translate_text_free',
      'stelnet_asr_session_result_word_p',
      'stelnet_asr_params_set_max_tokens',
      'stelnet_asr_text_detect_language',
      'stelnet_asr_enhance_audio_rnnoise',
    ]) {
      expect(() => lib.lookup(s), returnsNormally,
          reason: 'missing C-ABI symbol: $s');
    }
  });

  test('LidMethod enum indexes match the C-side StelnetAsrLidMethod', () {
    // stelnet_asr_detect_language_pcm dispatches on the int value of
    // `method.index`; the C side's `enum class StelnetAsrLidMethod`
    // hard-codes Whisper=0, Silero=1, Firered=2, Ecapa=3 (see
    // src/stelnet_asr_lid.h). If somebody reorders the Dart enum or
    // inserts a new variant in the middle, every Firered/Ecapa
    // call silently routes to the wrong backend with no compile
    // error. Pin the indexes here so a reorder shows up as a red
    // test, not a runtime regression.
    expect(LidMethod.whisper.index, 0);
    expect(LidMethod.silero.index, 1);
    expect(LidMethod.firered.index, 2);
    expect(LidMethod.ecapa.index, 3);
    expect(LidMethod.values.length, 4,
        reason: 'extending LidMethod without bumping the C-side enum '
            'will silently drop the new variant');
  });

  test('DiarizeMethod enum indexes match the C-side StelnetAsrDiarizeMethod', () {
    // diarizeSegments dispatches on `method.index`; the C side's
    // `enum class StelnetAsrDiarizeMethod` hard-codes Energy=0, Xcorr=1,
    // VadTurns=2, Pyannote=3, FoxNose=4 (see src/stelnet_asr_diarize.h).
    // Same trap as LidMethod above: a reorder or mid-enum insertion
    // silently routes calls to the wrong diarizer (#332).
    expect(DiarizeMethod.energy.index, 0);
    expect(DiarizeMethod.xcorr.index, 1);
    expect(DiarizeMethod.vadTurns.index, 2);
    expect(DiarizeMethod.pyannote.index, 3);
    expect(DiarizeMethod.foxNose.index, 4);
    expect(DiarizeMethod.values.length, 5,
        reason: 'extending DiarizeMethod without bumping the C-side enum '
            'will silently drop the new variant');
  });
}
