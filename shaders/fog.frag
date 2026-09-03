#version 460
layout(set = 0, binding = 0) uniform sampler2D source_color;
layout(set = 0, binding = 1) uniform sampler2DMS source_depth;

layout( push_constant ) uniform Params {
    float distance;
    float intensity;
    float near;
    float far;
    vec3 color;
} fog;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(source_color, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 uv = (vec2(pixel) + 0.5) / vec2(canvas_size);

    vec4 scene_color = texture(source_color, uv);

    int sample_count = textureSamples(source_depth);
    float raw_depth = 0.0;
    for (int i = 0; i < sample_count; ++i) {
        raw_depth += texelFetch(source_depth, pixel, i).r;
    }
    raw_depth /= float(sample_count);

    float dist = (fog.near * fog.far) / (fog.near + raw_depth * (fog.far - fog.near));

    float fog_amount = 1.0 - exp(-dist / max(fog.distance, 1e-4));
    fog_amount = clamp(fog_amount * fog.intensity, 0.0, 1.0);

    vec3 result = mix(scene_color.rgb, fog.color, fog_amount);
    out_color = vec4(result, scene_color.a);
}