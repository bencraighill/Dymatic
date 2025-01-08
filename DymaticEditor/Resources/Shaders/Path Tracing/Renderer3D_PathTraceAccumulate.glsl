// Path Tracing Accumulation Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;
layout (location = 1) out float o_EntityID;

layout (binding = 0) uniform sampler2D u_AccumulatedColor;
layout (binding = 1) uniform sampler2D u_CurrentFrame;

void main()
{
    vec3 accumulatedColor = texture(u_AccumulatedColor, v_TexCoord).rgb;
    vec3 currentFrameColor = texture(u_CurrentFrame, v_TexCoord).rgb;

    // Calculate the new accumulated color  
    if (u_AccumulatedFrames == 0)
        o_Color = vec4(currentFrameColor, 1.0);
    else
        o_Color = vec4(vec3((accumulatedColor * float(u_AccumulatedFrames) + currentFrameColor) / float(u_AccumulatedFrames + 1)), 1.0);
}