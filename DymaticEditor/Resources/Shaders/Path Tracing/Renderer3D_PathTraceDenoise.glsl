// Path Tracing Denoise Shader


// Creditation: Edge-Avoiding À-TrousWavelet Transform for denoising (https://www.shadertoy.com/view/ldKBzG)

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

layout (binding = 0) uniform sampler2D u_FrameColor;
layout (binding = 1) uniform sampler2D u_FrameNormal;

// TODO: Make these uniforms so they can be controlled in editor.
const float targetDenoiseStrength = 5.0;
const int denoiseFrames = 35;

void main()
{
    const float denoiseStrength = targetDenoiseStrength * (float(max(0, denoiseFrames - int(u_AccumulatedFrames))) / float(denoiseFrames));

    if (denoiseStrength == 0.0)
    {
        o_Color = vec4(texture(u_FrameColor, v_TexCoord).rgb, 1.0);
        return;
    }

    vec4 sum = vec4(0.0);
    float c_phi = 1.0;
    float n_phi = 0.5;
	vec4 cval = texture(u_FrameColor, v_TexCoord);
	vec4 nval = texture(u_FrameNormal, v_TexCoord);

    if (nval.rgb == vec3(0.0))
    {
        o_Color = vec4(texture(u_FrameColor, v_TexCoord).rgb, 1.0);
        return;
    }

    vec2 offset[25];
    offset[ 0] = vec2(-2,-2);
    offset[ 1] = vec2(-1,-2);
    offset[ 2] = vec2( 0,-2);
    offset[ 3] = vec2( 1,-2);
    offset[ 4] = vec2( 2,-2);
    
    offset[ 5] = vec2(-2,-1);
    offset[ 6] = vec2(-1,-1);
    offset[ 7] = vec2( 0,-1);
    offset[ 8] = vec2( 1,-1);
    offset[ 9] = vec2( 2,-1);
    
    offset[10] = vec2(-2, 0);
    offset[11] = vec2(-1, 0);
    offset[12] = vec2( 0, 0);
    offset[13] = vec2( 1, 0);
    offset[14] = vec2( 2, 0);
    
    offset[15] = vec2(-2, 1);
    offset[16] = vec2(-1, 1);
    offset[17] = vec2( 0, 1);
    offset[18] = vec2( 1, 1);
    offset[19] = vec2( 2, 1);
    
    offset[20] = vec2(-2, 2);
    offset[21] = vec2(-1, 2);
    offset[22] = vec2( 0, 2);
    offset[23] = vec2( 1, 2);
    offset[24] = vec2( 2, 2);
    
    float kernel[25];
    kernel[0] = 1.0 / 256.0;
    kernel[1] = 1.0 / 64.0;
    kernel[2] = 3.0 / 128.0;
    kernel[3] = 1.0 / 64.0;
    kernel[4] = 1.0 / 256.0;
    
    kernel[5] = 1.0 / 64.0;
    kernel[6] = 1.0 / 16.0;
    kernel[7] = 3.0 / 32.0;
    kernel[8] = 1.0 / 16.0;
    kernel[9] = 1.0 / 64.0;
    
    kernel[10] = 3.0 / 128.0;
    kernel[11] = 3.0 / 32.0;
    kernel[12] = 9.0 / 64.0;
    kernel[13] = 3.0 / 32.0;
    kernel[14] = 3.0 / 128.0;
    
    kernel[15] = 1.0 / 64.0;
    kernel[16] = 1.0 / 16.0;
    kernel[17] = 3.0 / 32.0;
    kernel[18] = 1.0 / 16.0;
    kernel[19] = 1.0 / 64.0;
    
    kernel[20] = 1.0 / 256.0;
    kernel[21] = 1.0 / 64.0;
    kernel[22] = 3.0 / 128.0;
    kernel[23] = 1.0 / 64.0;
    kernel[24] = 1.0 / 256.0;
    
    float cum_w = 0.0;
    for(int i = 0; i < 25; i++)
    {
        vec2 uv = v_TexCoord + (offset[i] / vec2(u_ScreenDimensions)) * denoiseStrength;
        
        vec4 ctmp = texture(u_FrameColor, uv);
        vec4 t = cval - ctmp;
        float dist2 = dot(t, t);
        float c_w = min(exp(-(dist2) / c_phi), 1.0);
        
        vec4 ntmp = texture(u_FrameNormal, uv);
        t = nval - ntmp;
        dist2 = max(dot(t,t), 0.0);
        float n_w = min(exp(-(dist2) / n_phi), 1.0);
        
        float weight = c_w * n_w;
        sum += ctmp * weight * kernel[i];
        cum_w += weight * kernel[i];
    }
    
    o_Color = sum / cum_w;
}