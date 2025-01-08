// Bloom Isolate Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_ColorTexture;

void main()
{
    const vec3 color = texture(u_ColorTexture, v_TexCoord).rgb;
    const float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    o_Color = vec4(brightness > u_BloomThreshold ? (color - vec3(u_BloomThreshold)) : vec3(0.0), 1.0);
}