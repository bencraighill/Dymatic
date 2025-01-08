// Draws bound texture

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (set = 0, binding = 0) uniform sampler2D u_ColorTexture;

void main()
{
    o_Color = texture(u_ColorTexture, v_TexCoord);
}