// Lens Distortion Post Processing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_SceneTexture;

const float barrelDistortion = 0.05;
const float rescale = 1.0 - (0.25 * barrelDistortion);

void main()
{
    vec2 texCoord = v_TexCoord - vec2(0.5);
    float rsq = texCoord.x * texCoord.x + texCoord.y * texCoord.y;
    texCoord = texCoord + (texCoord * (barrelDistortion * rsq));
    texCoord *= rescale;

    texCoord += vec2(0.5);
    o_Color = vec4(texture(u_SceneTexture, texCoord).rgb, 1.0);
}