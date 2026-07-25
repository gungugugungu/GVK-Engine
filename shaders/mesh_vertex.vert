#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outUV;
layout(location = 1) out vec3 world_pos;
layout(location = 2) out vec3 world_normal;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
    Vertex vertices[];
};

layout( push_constant ) uniform constants
{
    mat4 model_matrix;
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.render_matrix * vec4(v.position, 1.0);

    world_pos = (PushConstants.model_matrix * vec4(v.position, 1.0)).xyz;
    world_normal = mat3(PushConstants.model_matrix) * v.normal;

    outUV = vec2(v.uv_x, v.uv_y);
}