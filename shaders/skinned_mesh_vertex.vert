#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outUV;
layout(location = 1) out vec3 world_pos;
layout(location = 2) out vec3 world_normal;
layout(location = 3) out vec4 world_tangent;

struct SkinnedVertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec4 tangent;
    uvec4 joints;
    vec4 weights;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    SkinnedVertex vertices[];
};

layout(buffer_reference, std430) readonly buffer JointBuffer {
    mat4 joints[];
};

layout(push_constant) uniform constants
{
    mat4 model_matrix;
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
    JointBuffer jointBuffer;
} PushConstants;

void main()
{
    SkinnedVertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    mat4 skin =
    PushConstants.jointBuffer.joints[v.joints.x] * v.weights.x +
    PushConstants.jointBuffer.joints[v.joints.y] * v.weights.y +
    PushConstants.jointBuffer.joints[v.joints.z] * v.weights.z +
    PushConstants.jointBuffer.joints[v.joints.w] * v.weights.w;

    vec4 skinned_pos = skin * vec4(v.position, 1.0);
    mat3 skin3 = mat3(skin);

    gl_Position = PushConstants.render_matrix * skinned_pos;

    world_pos = (PushConstants.model_matrix * skinned_pos).xyz;

    world_normal = normalize(mat3(PushConstants.model_matrix) * (skin3 * v.normal));

    world_tangent = vec4(normalize(mat3(PushConstants.model_matrix) * (skin3 * v.tangent.xyz)), v.tangent.w);

    outUV = v.uv;
}