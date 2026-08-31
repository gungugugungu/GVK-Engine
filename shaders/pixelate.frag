#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;

layout( push_constant ) uniform Params {
    int size;
} pixelation;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    pixel = (pixel / pixelation.size) * pixelation.size;

    vec2 uv = (vec2(pixel) + 0.5) / vec2(canvas_size);
    out_color = texture(source_color, uv);
}