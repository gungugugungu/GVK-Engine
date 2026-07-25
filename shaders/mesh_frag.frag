#version 450

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 frag_color;

layout(set =0, binding = 0) uniform sampler2D albedo_map;
layout(set =0, binding = 1) uniform sampler2D normal_map;
layout(set =0, binding = 2) uniform sampler2D roughness_map;
layout(set =0, binding = 3) uniform sampler2D metallic_map;
layout(set =0, binding = 4) uniform sampler2D emissive_map;
layout(set =0, binding = 5) uniform sampler2D ao_map;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float range;
    float intensity;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float range;
    float intensity;
};

layout(set = 1, binding = 0, std140) uniform DirectionalLightBuffer {
    DirectionalLight light;
} directionalLight;

layout(set = 1, binding = 1, std430) readonly buffer PointLightsBuffer {
    PointLight lights[];
} pointLights;

layout(set = 1, binding = 2, std430) readonly buffer SpotLightsBuffer {
    SpotLight lights[];
} spotLights;

layout(set = 1, binding = 3, std140) uniform LightCountsBuffer {
    uint point_light_count;
    uint spot_light_count;
} lightCounts;

layout(push_constant, std430) uniform constants
{
    layout(offset = 80) float scalar_tint;
    float roughness;
    float metallic;
} MaterialPushConstants;

void main()
{
    frag_color = texture(albedo_map, uv);
}