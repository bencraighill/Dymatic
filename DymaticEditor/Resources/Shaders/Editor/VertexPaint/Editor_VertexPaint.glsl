// Dymatic Editor Vertex Color Visualization Shader

#type vertex
#version 450 core
#include Include/Buffers.glslh

#define MAX_VERTEX_SELECTION_PER_FRAME 256

layout (location = 0) in uint a_GlobalIndex;
layout (location = 1) in vec3 a_Position;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in vec3 a_Tangent;
layout (location = 5) in vec3 a_Bitangent;
layout (location = 6) in vec4 a_Color;
layout (location = 7) in ivec4 a_BoneIDs;
layout (location = 8) in vec4 a_Weights;

layout (location = 0) out vec4 v_Color;

layout (std140, binding = EDITOR_BUFFER_BINDING) uniform PaintSettings
{
    vec2 u_BrushPosition;
    float u_BrushRadius;
    bool u_Painting;
};

layout (std430, binding = EDITOR_BUFFER_BINDING) buffer PaintedVertices
{
    uint u_PaintedVertexCount;
    uint u_PaintedVertices[MAX_VERTEX_SELECTION_PER_FRAME];
};

void main()
{
    v_Color = a_Color;

    const vec4 clip = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
    const vec3 ndc = clip.xyz / clip.w;

    if (ndc.z >= 0.0 && u_Painting && u_PaintedVertexCount < MAX_VERTEX_SELECTION_PER_FRAME)
    {
        if (distance(ndc.xy, u_BrushPosition) <= u_BrushRadius)
            u_PaintedVertices[atomicAdd(u_PaintedVertexCount, 1)] = a_GlobalIndex;
    }

	gl_Position = clip;
}

#type fragment
#version 450 core

layout (location = 0) in vec4 v_Color;
layout(location = 0) out vec4 o_Color;

void main()
{
    if (v_Color.a == 1.0)
    {
        o_Color = v_Color;
        return;
    }

    const vec3 gridColor = ((int(gl_FragCoord.x) / 7 + int(gl_FragCoord.y) / 7) % 2 == 0) ? vec3(0.8) : vec3(0.2);
    o_Color = vec4(mix(gridColor, v_Color.rgb, v_Color.a), 1.0);
}