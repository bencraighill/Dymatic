// IBL Background Shader

// Include cubemap vertex shader
#include assets/shaders/Renderer3D_IBLCubemap.glsl

#type fragment
#version 450 core

layout (location = 0) in vec3 v_Position;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform samplerCube u_EnvironmentMap;

void main()
{
    vec3 envColor = textureLod(u_EnvironmentMap, v_Position, 0.0).rgb;
    
    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));
    
    o_Color = vec4(envColor, 1.0);
}