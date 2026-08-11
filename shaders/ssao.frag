#version 460

layout(set = 0, binding = 0) uniform sampler2DMS depth_image;

layout(push_constant) uniform SSAOCompositeParams {
    float radius;
    float bias;
    int samples;
    vec2 proj;
    float u_near;
    float u_far;
} ssao;

layout(location = 0) out vec4 out_color;

const vec3 kernel16[16] = vec3[](
vec3( 0.2024, 0.0699, 0.1620), vec3( 0.0515, 0.0929, 0.1659),
vec3( 0.1261, 0.2124, 0.0214), vec3(-0.2616, -0.0168, -0.0071),
vec3( 0.0257, -0.0598, 0.3198), vec3( 0.0209, 0.0026, -0.0689),
vec3(-0.0055, 0.0524, 0.0285), vec3( 0.0037, -0.0719, -0.0129),
vec3(-0.1338, -0.1982, -0.1630), vec3(-0.1185, 0.0398, 0.0059),
vec3( 0.0039, -0.2195, 0.0017), vec3(-0.0335, -0.1848, 0.1229),
vec3( 0.2663, -0.0058, -0.0343), vec3(-0.0199, 0.0223, -0.2024),
vec3( 0.0132, -0.0236, 0.2041), vec3(-0.1786, 0.1065, -0.0101)
);

float sample_depth(ivec2 coord) {
    return texelFetch(depth_image, coord, 0).r;
}

float linearize_reversed_depth(float d, float near, float far) {
    float denom = max((near + d * (far - near)), 1e-6);
    float viewZ = (near * far) / denom;
    return viewZ;
}

vec3 reconstruct_view_pos(vec2 uvcoord, float depth_sample) {
    float viewZ = linearize_reversed_depth(depth_sample, ssao.u_near, ssao.u_far);
    vec2 ndc = uvcoord * 2.0 - 1.0;
    vec3 viewPos;
    viewPos.x = ndc.x * viewZ * ssao.proj.x;
    viewPos.y = ndc.y * viewZ * ssao.proj.y;
    viewPos.z = -viewZ;
    return viewPos;
}

vec3 estimate_normal(vec2 uvcoord, float center_depth) {
    ivec2 tex_size = textureSize(depth_image);
    ivec2 center = ivec2(uvcoord * vec2(tex_size));
    float depth_r = sample_depth(center + ivec2(1, 0));
    float depth_u = sample_depth(center + ivec2(0, 1));
    vec3 p = reconstruct_view_pos(uvcoord,center_depth);
    vec3 pr = reconstruct_view_pos(uvcoord+vec2(1.0/tex_size.x, 0.0), depth_r);
    vec3 pu = reconstruct_view_pos(uvcoord+vec2(0.0, 1.0/tex_size.y), depth_u);
    vec3 vx = pr - p;
    vec3 vy = pu - p;
    vec3 n = normalize(cross(vy, vx));
    if(length(n) < 1e-3) return vec3(0.0, 0.0, 1.0);
    return n;
}

vec2 project_view_to_uv(vec3 viewPos) {
    vec2 ndc;
    ndc.x = viewPos.x / (-viewPos.z * ssao.proj.x);
    ndc.y = viewPos.y / (-viewPos.z * ssao.proj.y);
    return ndc * 0.5 + 0.5;
}

vec3 sample_noise(vec2 uv) {
    ivec2 tex_size = textureSize(depth_image);
    vec2 p = uv * vec2(tex_size);
    float n = sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453;
    float angle = fract(n) * 6.28318;
    return vec3(cos(angle), sin(angle), 0.0);
}

void main() {
    ivec2 tex_size = textureSize(depth_image);
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec2 uv = (vec2(coord) + 0.5) / vec2(tex_size);

    float d = sample_depth(coord);

    if (d <= 0.0001) {
        out_color = vec4(1.0);
        return;
    }

    vec3 P = reconstruct_view_pos(uv, d);
    vec3 N = estimate_normal(uv, d);

    vec3 randomVec = sample_noise(uv);

    vec3 tangent = normalize(randomVec - N * dot(randomVec, N));
    vec3 bitangent = cross(N, tangent);
    mat3 TBN = mat3(tangent, bitangent, N);

    int sample_count = clamp(ssao.samples, 1, 16);
    float occlusion = 0.0;

    for (int i = 0; i < sample_count; ++i) {
        vec3 sampleVec = TBN * kernel16[i];
        if (dot(sampleVec, N) < 0.0) {
            sampleVec = -sampleVec;
        }

        vec3 samplePos = P + sampleVec * ssao.radius;

        vec2 sampleUV = project_view_to_uv(samplePos);

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;

        ivec2 sampleCoord = clamp(ivec2(sampleUV * vec2(textureSize(depth_image))), ivec2(0), textureSize(depth_image) - 1);
        float sampleDepthTex = sample_depth(sampleCoord);
        float sampleDepthViewZ = linearize_reversed_depth(sampleDepthTex, ssao.u_near, ssao.u_far);
        float samplePosViewZ = -samplePos.z;

        if (sampleDepthViewZ < (samplePosViewZ - ssao.bias)) {
            float distance = abs(sampleDepthViewZ - samplePosViewZ);
            float rangeCheck = 1.0 - smoothstep(ssao.radius * 0.5, ssao.radius, distance);
            occlusion += rangeCheck;
        }
    }

    float occ = clamp(occlusion / float(sample_count), 0.0, 1.0);
    float ao = 1.0 - occ;

    out_color = vec4(ao, ao, ao, 1.0);
}