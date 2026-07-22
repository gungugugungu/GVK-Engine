#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;
layout(set = 0, binding = 1) uniform sampler2D bloom_color;

layout( push_constant ) uniform BloomCompositeParams {
    float intensity;
} bloom;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    vec2 uv = gl_FragCoord.xy / vec2(canvas_size);

    vec4 base = texture(source_color, uv);
    vec4 glow = texture(bloom_color, uv);

    out_color = vec4(base.rgb + glow.rgb * bloom.intensity, base.a);
}
