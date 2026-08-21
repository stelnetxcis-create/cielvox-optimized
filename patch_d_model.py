path = "src/cielvox2_tts.cpp"
old = '''    t.output_norm_w = require(c, "talker.output_norm.weight");
    t.codec_head_w = require(c, "talker.output.weight");
    if (!t.token_embd_w || !t.token_embd_text_w || !t.text_proj_fc1_w || !t.output_norm_w || !t.codec_head_w) {
        return false;
    }'''
new = '''    t.output_norm_w = require(c, "talker.output_norm.weight");
    t.codec_head_w = require(c, "talker.output.weight");
    if (!t.token_embd_w || !t.token_embd_text_w || !t.text_proj_fc1_w || !t.output_norm_w || !t.codec_head_w) {
        return false;
    }
    if (t.output_norm_w->ne[0] > 0 && (uint32_t)t.output_norm_w->ne[0] != c->hp.d_model) {
        fprintf(stderr, "cielvox2_tts: talker.output_norm shape (%lld) overrides stale d_model metadata (%u)\\n",
                (long long)t.output_norm_w->ne[0], c->hp.d_model);
        c->hp.d_model = (uint32_t)t.output_norm_w->ne[0];
    }'''
with open(path) as f:
    content = f.read()
assert content.count(old) == 1, f"match count: {content.count(old)}"
content = content.replace(old, new)
with open(path, "w") as f:
    f.write(content)
print("Patched successfully.")
