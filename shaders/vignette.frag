#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;

layout( push_constant ) uniform Params {
    float strength;
    float radius;
} vignette;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    vec2 uv = gl_FragCoord.xy / canvas_size;
    vec4 color = texture(source_color, uv);

    float dist = length(uv - vec2(0.5));
    float norm_dist = dist / sqrt(0.5);
    color *= max(0.0, 1.0 - vignette.strength * pow(norm_dist, vignette.radius));

    out_color = color;
}