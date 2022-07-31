// Skybox Shader

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;

layout (location = 1) out vec3 TexCoords;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
};

void main()
{
    TexCoords = vec3(a_Position.xy, a_Position.z);
    vec4 pos = u_ViewProjection * vec4(vec3(u_ViewPosition) + a_Position, 1.0);
    gl_Position = pos.xyww;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

layout(location = 1) in vec3 TexCoords;

layout (set = 0, binding = 7) uniform samplerCube u_EnvironmentMap;

void main()
{    
    o_Color = texture(u_EnvironmentMap, TexCoords);
    o_EntityID = -1;
}