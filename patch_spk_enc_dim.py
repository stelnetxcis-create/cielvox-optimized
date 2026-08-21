path = "src/cielvox2_tts.cpp"
old = '    spk.fc_w = req("speaker.fc.weight");\n    spk.fc_b = req("speaker.fc.bias");\n    if (!spk.mfa.w || !spk.asp.tdnn.w || !spk.fc_w) {\n        return false;\n    }'
new = '    spk.fc_w = req("speaker.fc.weight");\n    spk.fc_b = req("speaker.fc.bias");\n    if (!spk.mfa.w || !spk.asp.tdnn.w || !spk.fc_w) {\n        return false;\n    }\n    if (spk.fc_b && spk.fc_b->ne[0] > 0 && (uint32_t)spk.fc_b->ne[0] != c->hp.spk_enc_dim) {\n        fprintf(stderr, "cielvox2_tts: speaker.fc.bias shape (%lld) overrides stale spk_enc_dim metadata (%u)\\n",\n                (long long)spk.fc_b->ne[0], c->hp.spk_enc_dim);\n        c->hp.spk_enc_dim = (uint32_t)spk.fc_b->ne[0];\n    }'
with open(path) as f:
    content = f.read()
assert content.count(old) == 1, f"match count: {content.count(old)}"
content = content.replace(old, new)
with open(path, "w") as f:
    f.write(content)
print("Patched successfully.")
