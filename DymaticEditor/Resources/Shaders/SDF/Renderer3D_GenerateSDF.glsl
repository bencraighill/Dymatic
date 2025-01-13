// Dymatic Mesh SDF Generation Shader
// Note: This is an editor only engine routine

#type compute
#version 450 core
#include Include/Constants.glslh

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (std140, binding = EDITOR_BUFFER_BINDING) uniform SDFSettings
{
    vec4 u_MaxBounds;
    vec4 u_MinBounds;
    uint u_Resolution;
    uint u_IndexCount;
};

layout (std430, binding = 28) buffer IndexBuffer
{
    uint u_Indices[];
};

// Note: These constants are in terms of floats
#define VERTEX_FLOAT_COUNT 27
#define OFFSET_POSITION_X 1
#define OFFSET_POSITION_Y 2
#define OFFSET_POSITION_Z 3
#define OFFSET_NORMAL_X 4
#define OFFSET_NORMAL_Y 5
#define OFFSET_NORMAL_Z 6

layout (std430, binding = EDITOR_BUFFER_BINDING) buffer VertexBuffer
{
    // Represent vertex data as sequence of floats
    float u_VertexData[];
};

layout (binding = 0, r32f) restrict uniform image3D u_SDF;

float dot2(in vec2 v) { return dot(v, v); }
float dot2(in vec3 v) { return dot(v, v); }
float ndot(in vec2 a, in vec2 b) { return a.x * b.x - a.y * b.y; }

float SDFTriangle(vec3 p, vec3 a, vec3 b, vec3 c);
bool IsFrontFacing(vec3 p, vec3 p0, vec3 p1, vec3 p2, vec3 normal);

void main()
{
    const ivec3 coord = ivec3(gl_GlobalInvocationID);
    
    const vec3 texelSize = (u_MaxBounds.xyz - u_MinBounds.xyz) / vec3(u_Resolution);
    const vec3 worldPos = u_MinBounds.xyz + vec3(coord) * texelSize;

    float minDistance = FLT_MAX;
    bool frontFacing = true;

    for (uint i = 0; i < u_IndexCount; i += 3)
    {
        const uint v0 = u_Indices[i + 0] * VERTEX_FLOAT_COUNT;
        const uint v1 = u_Indices[i + 1] * VERTEX_FLOAT_COUNT;
        const uint v2 = u_Indices[i + 2] * VERTEX_FLOAT_COUNT;

        const vec3 p0 = vec3(u_VertexData[v0 + OFFSET_POSITION_X], u_VertexData[v0 + OFFSET_POSITION_Y], u_VertexData[v0 + OFFSET_POSITION_Z]);
        const vec3 p1 = vec3(u_VertexData[v1 + OFFSET_POSITION_X], u_VertexData[v1 + OFFSET_POSITION_Y], u_VertexData[v1 + OFFSET_POSITION_Z]);
        const vec3 p2 = vec3(u_VertexData[v2 + OFFSET_POSITION_X], u_VertexData[v2 + OFFSET_POSITION_Y], u_VertexData[v2 + OFFSET_POSITION_Z]);

        const vec3 n0 = vec3(u_VertexData[v0 + OFFSET_NORMAL_X], u_VertexData[v0 + OFFSET_NORMAL_Y], u_VertexData[v0 + OFFSET_NORMAL_Z]);
        const vec3 n1 = vec3(u_VertexData[v1 + OFFSET_NORMAL_X], u_VertexData[v1 + OFFSET_NORMAL_Y], u_VertexData[v1 + OFFSET_NORMAL_Z]);
        const vec3 n2 = vec3(u_VertexData[v2 + OFFSET_NORMAL_X], u_VertexData[v2 + OFFSET_NORMAL_Y], u_VertexData[v2 + OFFSET_NORMAL_Z]);
        const vec3 normal = (n0 + n1 + n2) / 3.0f;

        const float currentDistance = SDFTriangle(worldPos, p0, p1, p2);
        
        if (currentDistance < minDistance)
        {
            minDistance = currentDistance;
            frontFacing = IsFrontFacing(worldPos, p0, p1, p2, normal);
        }
    }

    imageStore(u_SDF, coord, vec4(frontFacing ? minDistance : - minDistance, 0.0, 0.0, 1.0));
}

float SDFTriangle(vec3 p, vec3 a, vec3 b, vec3 c)
{
    const vec3 ba  = b - a;
    const vec3 pa  = p - a;
    const vec3 cb  = c - b;
    const vec3 pb  = p - b;
    const vec3 ac  = a - c;
    const vec3 pc  = p - c;
    const vec3 nor = cross(ba, ac);

    return sqrt(
        (sign(dot(cross(ba, nor), pa)) + sign(dot(cross(cb, nor), pb)) + sign(dot(cross(ac, nor), pc)) < 2.0) ?
            min(min(
                    dot2(ba * clamp(dot(ba, pa) / dot2(ba), 0.0, 1.0) - pa),
                    dot2(cb * clamp(dot(cb, pb) / dot2(cb), 0.0, 1.0) - pb)),
                dot2(ac * clamp(dot(ac, pc) / dot2(ac), 0.0, 1.0) - pc)) :
            dot(nor, pa) * dot(nor, pa) / dot2(nor));
}

bool IsFrontFacing(vec3 p, vec3 p0, vec3 p1, vec3 p2, vec3 normal)
{
    return dot(normalize(p - p0), normal) >= 0.0f || dot(normalize(p - p1), normal) >= 0.0f || dot(normalize(p - p2), normal) >= 0.0f;
}