// Draws bound texture

#type vertex
#version 450 core

const vec2 madd = vec2(0.5,0.5);

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec2 o_TexCoord;

void main() 
{
   o_TexCoord = a_Position.xy*madd+madd; // scale vertex attribute to [0-1] range
   gl_Position = vec4(a_Position.xy,0.0,1.0);
}

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (set = 0, binding = 0) uniform sampler2D u_ColorTexture;

void main()
{
    o_Color = texture(u_ColorTexture, v_TexCoord);
}