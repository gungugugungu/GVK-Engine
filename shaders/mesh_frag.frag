#version 450

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 world_pos;
layout (location = 2) in vec3 world_normal;
layout (location = 3) in vec4 world_tangent;

layout (location = 0) out vec4 frag_color;

layout(set =0, binding = 0) uniform sampler2D albedo_map;
layout(set =0, binding = 1) uniform sampler2D normal_map;
layout(set =0, binding = 2) uniform sampler2D roughness_map;
layout(set =0, binding = 3) uniform sampler2D metallic_map;
layout(set =0, binding = 4) uniform sampler2D emissive_map;
layout(set =0, binding = 5) uniform sampler2D ao_map;

struct DirectionalLight { // and the camera for my lazy ass
    vec3  direction;
    vec3  color;
    float intensity;
    vec3  camera_pos;
    vec3  ambient_color;
    float ambient_intensity;
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
    layout(offset = 136) float scalar_tint;
    float roughness;
    float metallic;
} MaterialPushConstants;

const float PI = 3.14159265359;

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 specularBRDF(vec3 N, vec3 V, vec3 L, float roughness, vec3 F0)
{
    vec3 H = normalize(V + L);

    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(HdotV, F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    return numerator / denominator;
}

float attenuation(float distance, float range)
{
    float att = 1.0 - smoothstep(0.0, range, distance);
    return att * att;
}

void main()
{
    vec4 albedoSample = texture(albedo_map, uv);
    vec3 albedo = albedoSample.rgb * MaterialPushConstants.scalar_tint;
    float alpha = albedoSample.a;

    float roughnessTex = texture(roughness_map, uv).r;
    float metallicTex = texture(metallic_map, uv).r;
    float ao = texture(ao_map, uv).r;
    vec3 emissive = texture(emissive_map, uv).rgb;

    float roughness = clamp(roughnessTex * MaterialPushConstants.roughness, 0.04, 1.0);
    float metallic = clamp(metallicTex * MaterialPushConstants.metallic, 0.0, 1.0);

    vec3 N = normalize(world_normal);
    vec3 T = normalize(world_tangent.xyz);
    T = normalize(T - N * dot(N, T));
    vec3 B = cross(N, T) * world_tangent.w;
    vec3 tangentNormal = texture(normal_map, uv).xyz * 2.0 - 1.0;
    mat3 TBN = mat3(T, B, N);
    N = normalize(TBN * tangentNormal);
    vec3 V = normalize(directionalLight.light.camera_pos - world_pos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 ambient = albedo * ao * directionalLight.light.ambient_color * directionalLight.light.ambient_intensity;

    vec3 Lo = vec3(0.0);

    {
        vec3 L = normalize(-directionalLight.light.direction);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0)
        {
            vec3 radiance = directionalLight.light.color * directionalLight.light.intensity;

            vec3 kS = F_Schlick(max(dot(N, V), 0.0), F0);
            vec3 kD = (1.0 - kS) * (1.0 - metallic);
            vec3 diffuse = kD * albedo / PI;

            vec3 specular = specularBRDF(N, V, L, roughness, F0);

            Lo += (diffuse + specular) * radiance * NdotL;
        }
    }

    for (uint i = 0u; i < lightCounts.point_light_count; ++i)
    {
        PointLight light = pointLights.lights[i];

        vec3 toLight = light.position - world_pos;
        float dist = length(toLight);
        vec3 L = toLight / dist;
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0 && dist < light.range)
        {
            float att = attenuation(dist, light.range);
            vec3 radiance = light.color * light.intensity * att;

            vec3 kS = F_Schlick(max(dot(N, V), 0.0), F0);
            vec3 kD = (1.0 - kS) * (1.0 - metallic);
            vec3 diffuse = kD * albedo / PI;
            vec3 specular = specularBRDF(N, V, L, roughness, F0);

            Lo += (diffuse + specular) * radiance * NdotL;
        }
    }

    for (uint i = 0u; i < lightCounts.spot_light_count; ++i)
    {
        SpotLight light = spotLights.lights[i];

        vec3 toLight = light.position - world_pos;
        float dist = length(toLight);
        vec3 L = toLight / dist;
        float NdotL = max(dot(N, L), 0.0);

        float cosTheta = dot(-L, normalize(light.direction));
        float spotFactor = step(0.7, cosTheta);

        if (NdotL > 0.0 && dist < light.range && spotFactor > 0.0)
        {
            float att = attenuation(dist, light.range) * spotFactor;
            vec3 radiance = light.color * light.intensity * att;

            vec3 kS = F_Schlick(max(dot(N, V), 0.0), F0);
            vec3 kD = (1.0 - kS) * (1.0 - metallic);
            vec3 diffuse = kD * albedo / PI;
            vec3 specular = specularBRDF(N, V, L, roughness, F0);

            Lo += (diffuse + specular) * radiance * NdotL;
        }
    }

    vec3 color = ambient + Lo + emissive;
    color = color / (color + vec3(1.0));
    frag_color = vec4(color, alpha);
}