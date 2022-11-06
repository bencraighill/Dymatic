// Bloom Isolate Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_ColorTexture;

void main()
{
    vec4 color = texture(u_ColorTexture, v_TexCoord);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 2.0)
        o_Color = color - vec4(2.0, 2.0, 2.0, 0.0);
    else
        o_Color = vec4(0.0, 0.0, 0.0, 1.0);
}