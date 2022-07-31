// Bloom Isolate Shader

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

layout (binding = 0) uniform sampler2D u_ColorTexture;

void main()
{
    vec4 color = texture(u_ColorTexture, v_TexCoord);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.99)
        o_Color = vec4(color.rgb, 1.0);
    else
        o_Color = vec4(0.0, 0.0, 0.0, 1.0);
}