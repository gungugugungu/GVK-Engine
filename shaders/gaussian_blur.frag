#version 460

layout(set = 0, binding = 0) uniform sampler2D source_color;

layout( push_constant ) uniform BlurParams {
    int radius;
    float sigma;
    int is_vertical;
} blur;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    vec2 uv = gl_FragCoord.xy / canvas_size;
    vec2 texel_size = 1.0 / vec2(canvas_size);

    vec2 direction = mix(vec2(1.0, 0.0), vec2(0.0, 1.0), float(blur.is_vertical));

    float two_sigma_sq = 2.0 * blur.sigma * blur.sigma;

    vec4 accum = vec4(0.0);
    float weight_sum = 0.0;

    for (int i = -blur.radius; i <= blur.radius; ++i) {
        float weight = exp(-float(i * i) / two_sigma_sq);
        vec2 offset = direction * texel_size * float(i);
        accum += texture(source_color, uv + offset) * weight;
        weight_sum += weight;
    }

    out_color = accum / weight_sum;
}
