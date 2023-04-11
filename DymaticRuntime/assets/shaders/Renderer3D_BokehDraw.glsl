// Bokeh Sprite Draw Post Processing Shader

#type vertex
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_BokehIndex;

layout (location = 0) out vec2 o_TexCoord;
layout (location = 1) out vec3 o_Color;

const vec2 madd = vec2(0.5,0.5);

void main() 
{
   o_TexCoord = a_Position.xy*madd+madd; // scale vertex attribute to [0-1] range
   o_Color = u_BokehList[a_BokehIndex].color;
   gl_Position = vec4(a_Position.xy * (u_PixelSize * u_BokehList[a_BokehIndex].size) + (u_BokehList[a_BokehIndex].position * 2.0 - vec2(1.0)), 0.0, 1.0);
}

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;
layout (location = 1) in vec3 v_Color;

layout (location = 0) out vec4 o_Color;

layout (set = 0, binding = 0) uniform sampler2D u_BokehShapeTexture;

void main()
{
    o_Color = vec4(v_Color, texture(u_BokehShapeTexture, v_TexCoord).r * 0.25);
}