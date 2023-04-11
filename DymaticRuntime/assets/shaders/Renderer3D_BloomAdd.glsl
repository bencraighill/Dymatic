// Bloom Addition Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_ColorTextureA;
layout (binding = 1) uniform sampler2D u_ColorTextureB;

void main()
{
    o_Color = vec4(texture(u_ColorTextureA, v_TexCoord).rgb + texture(u_ColorTextureB, v_TexCoord).rgb, 1.0);
}