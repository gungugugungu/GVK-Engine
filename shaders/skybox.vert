#version 460
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outUVW;

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
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    outUVW = v.position;
    vec4 pos = PushConstants.render_matrix * vec4(v.position, 1.0);
    gl_Position = pos.xyww;
}