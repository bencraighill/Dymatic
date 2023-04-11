// Bloom Composite Post Processing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_SceneTexture;
layout (binding = 1) uniform sampler2D u_BloomTexture;
layout (binding = 2) uniform sampler2D u_DirtTexture;

void main()
{
    // Get Bloom Texture divided by the number of downsamples
    vec3 bloom = texture(u_BloomTexture, v_TexCoord).rgb / 13.0f;

    o_Color = vec4(
        texture(u_SceneTexture, v_TexCoord).rgb +
        bloom + bloom * texture(u_DirtTexture, v_TexCoord).rgb * 10.0
    , 1.0);
}