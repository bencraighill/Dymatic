// Bloom Addition Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_TextureA;
layout (binding = 1) uniform sampler2D u_TextureB;

void main()
{
    o_Color = vec4(texture(u_TextureA, v_TexCoord).rgb + texture(u_TextureB, v_TexCoord).rgb, 1.0);
}