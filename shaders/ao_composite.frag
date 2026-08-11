#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;
layout(set = 0, binding = 1) uniform sampler2D ao_color;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    vec2 uv = gl_FragCoord.xy / vec2(canvas_size);

    vec4 base = texture(source_color, uv);
    float ao = texture(ao_color, uv).r;
    ao = pow(ao, 1.2);
    ao = mix(1.0, ao, 0.85);

    out_color = vec4(base.rgb * ao, base.a);
}