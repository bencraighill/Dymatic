// Solid Color Mesh Shader

#type vertex
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

void main()
{
	gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

void main()
{
    o_Color = u_Albedo;
    o_EntityID = u_EntityID;
}