#version 460
layout(set = 0, binding = 0) uniform sampler2D scene_color;
layout(set = 0, binding = 1) uniform sampler2D skybox_color;

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 canvas_size = textureSize(scene_color, 0);
    vec2 uv = gl_FragCoord.xy / canvas_size;
    vec4 scene = texture(scene_color, uv);
    vec4 sky = texture(skybox_color, uv);

    out_color = mix(sky, scene, scene.a);
}