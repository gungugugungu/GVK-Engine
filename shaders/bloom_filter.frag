#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;

layout( push_constant ) uniform BloomThresholdParams {
    float threshold;
    float knee;
} bloom;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    vec2 uv = gl_FragCoord.xy / vec2(canvas_size);

    vec4 color = texture(source_color, uv);
    float brightness = max(color.r, max(color.g, color.b));

    float soft = brightness - bloom.threshold + bloom.knee;
    soft = clamp(soft, 0.0, 2.0 * bloom.knee);
    soft = soft * soft / (4.0 * bloom.knee + 1e-5);

    float contribution = max(soft, brightness - bloom.threshold);
    contribution /= max(brightness, 1e-5);

    out_color = color * contribution;
}