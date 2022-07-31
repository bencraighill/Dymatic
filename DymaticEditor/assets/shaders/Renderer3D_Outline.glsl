// Outline Post Processing Shader

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

#define WIDTH 2

layout (location = 0) in vec2 a_TexCoord;
layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

layout (binding = 0) uniform sampler2D u_Texture;

vec2 UV(float x, float y)
{
    return vec2(x / 1920, y / 1080);
}

void main() 
{
   if (texture(u_Texture, a_TexCoord).r != 1.0)
       discard;

   bool found = false;
   for (int x = -WIDTH; x <= WIDTH; x++)
       for (int y = -WIDTH; y <= WIDTH; y++)
           if (texture(u_Texture, a_TexCoord + UV(x, y)).r != 1.0)
           {
               found = true;
               break;
           }
   if (!found)
       discard;

    o_Color = vec4(0.82f, 0.62f, 0.13f, 1.0);
    o_EntityID = -1;
}