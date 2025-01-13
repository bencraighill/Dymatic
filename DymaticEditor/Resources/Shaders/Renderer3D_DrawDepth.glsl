// Draws bound texture

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;

layout (set = 0, binding = 0) uniform sampler2D u_DepthTexture;

void main()
{
    gl_FragDepth = texture(u_DepthTexture, v_TexCoord).r;
}