#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;

layout( push_constant ) uniform BlurParams {
    int radius;
    int is_vertical;
} blur;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    vec2 uv = gl_FragCoord.xy / vec2(canvas_size);
    vec2 texel_size = 1.0 / vec2(canvas_size);

    vec2 direction = mix(vec2(1.0, 0.0), vec2(0.0, 1.0), float(blur.is_vertical));

    int kernel_size = 2 * blur.radius + 1;
    vec4 accum = vec4(0.0);

    for (int i = -blur.radius; i <= blur.radius; ++i) {
        vec2 offset = direction * texel_size * float(i);
        accum += texture(source_color, uv + offset);
    }

    out_color = accum / float(kernel_size);
}