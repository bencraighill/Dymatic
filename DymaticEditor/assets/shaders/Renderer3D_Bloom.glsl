// Bloom Bluring Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_ColorTexture;

const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

layout(std140, binding = 4) uniform PostProcessing
{
    vec4 u_SSAOSamples[64];
    float u_Exposure;
    int u_HorizontalBlur;
};

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(u_ColorTexture, 0); // gets size of single texel
    vec3 result = texture(u_ColorTexture, v_TexCoord).rgb * weight[0]; // current fragment's contribution
    if(u_HorizontalBlur == 1)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_ColorTexture, v_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(u_ColorTexture, v_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_ColorTexture, v_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(u_ColorTexture, v_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    o_Color = vec4(result, 1.0);
}