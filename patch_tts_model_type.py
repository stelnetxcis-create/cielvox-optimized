path = "src/cielvox2_tts.cpp"
old = '''    const bool is_custom_voice = (ctx->hp.tts_model_type == "custom_voice");
    const bool is_voice_design = (ctx->hp.tts_model_type == "voice_design");
    if (is_custom_voice) {'''
new = '''    // FIX #3: tts_model_type metadata is unreliable for some rebranded 1.7B ggufs
    // (silently stuck at default "base" even for voicedesign/customvoice variants).
    // Infer the real type from independent, already-verified-reliable signals instead
    // of trusting the metadata string directly.
    std::string effective_model_type = ctx->hp.tts_model_type;
    if (effective_model_type == "base") {
        if (!ctx->hp.spk_names.empty()) {
            effective_model_type = "custom_voice";
            fprintf(stderr, "cielvox2_tts: spk_names present -- overriding stale tts_model_type 'base' to 'custom_voice'\n");
        } else if (!ctx->runtime_instruct.empty() && !ctx->spk_enc.loaded) {
            effective_model_type = "voice_design";
            fprintf(stderr, "cielvox2_tts: instruct set + no speaker encoder -- overriding stale tts_model_type 'base' to 'voice_design'\n");
        }
    }
    const bool is_custom_voice = (effective_model_type == "custom_voice");
    const bool is_voice_design = (effective_model_type == "voice_design");
    if (is_custom_voice) {'''
with open(path) as f:
    content = f.read()
assert content.count(old) == 1, f"match count: {content.count(old)}"
content = content.replace(old, new)
with open(path, "w") as f:
    f.write(content)
print("Patched successfully.")