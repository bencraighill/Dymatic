// IBL Background Shader

// Include cubemap vertex shader
#type vertex
#version 450 core
#include Include/Buffers.glslh
#include Renderer3D_IBLCubemap.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec3 v_Position;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform samplerCube u_EnvironmentMapSampler;

void main()
{
    vec3 envColor = textureLod(u_EnvironmentMapSampler, v_Position, 0.0).rgb;    
    o_Color = vec4(envColor, 1.0);
}