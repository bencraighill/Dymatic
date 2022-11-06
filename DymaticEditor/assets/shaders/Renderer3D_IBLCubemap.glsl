// IBL Cubemap Vertex Shader

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec3 o_Position;

layout(std140, binding = 2) uniform Object
{
	mat4 u_ViewProjection;
};

void main()
{
    o_Position = a_Position;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}