#version 460
layout(location = 0) in vec3 uv;

layout(set = 0, binding = 0) uniform samplerCube smp;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(smp, uv);
}