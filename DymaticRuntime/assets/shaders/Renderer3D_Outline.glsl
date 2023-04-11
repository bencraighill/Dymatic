// Outline Post Processing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

#define WIDTH 4

layout (location = 0) in vec2 a_TexCoord;
layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

layout (binding = 0) uniform sampler2D u_Texture;

vec2 UV(float x, float y)
{
    return vec2(x / float(u_ScreenDimensions.x), y / float(u_ScreenDimensions.y));
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

    o_Color = vec4(SELECTION_COLOR, 1.0);
    o_EntityID = -1;
}