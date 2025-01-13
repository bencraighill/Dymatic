// Gaussian Blur Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_Texture;

const float directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
const float quality = 4.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
const float size = 10.0; // BLUR SIZE (Radius)

void main()
{
    float Pi2 = 6.28318530718; // Pi*2
   
    vec2 radius = size/u_ScreenDimensions.xy;
    
    // Pixel colour
    vec4 color = texture(u_Texture, v_TexCoord);
    
    // Blur calculations
    for( float d=0.0; d<Pi2; d+=Pi2/directions)
    {
		for(float i=1.0/quality; i<=1.0; i+=1.0/quality)
        {
			color += texture(u_Texture, v_TexCoord + vec2(cos(d),sin(d))*radius*i);		
        }
    }
    
    // Output to screen
    color /= quality * directions - 15.0;
    o_Color = color;
}