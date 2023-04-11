// Final Compositing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;
layout (location = 1) out float o_EntityID;

layout (binding = 0) uniform sampler2D u_ColorTexture;
layout (binding = 1) uniform sampler2D u_EntityIDTexture;

void main()
{
  o_Color = vec4(texture(u_ColorTexture, v_TexCoord).rgb, 1.0);
  o_EntityID = texture(u_EntityIDTexture, v_TexCoord).r;
}