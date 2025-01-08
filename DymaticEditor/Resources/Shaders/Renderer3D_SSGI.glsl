// Screen Space Global Illumination Shader Header File

#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh
#include Include/Random.glslh
#include Include/Sampling.glslh

#define EPSILON 1e-3

layout(set = 0, binding = 0) uniform sampler2D u_DirectLighting;

layout (location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 o_Color;

float GetMaterialVariance(float specularChance, float roughness)
{
    float diffuseChance = 1.0 - specularChance;
    float perceivedFinalRoughness = 1.0 - (specularChance * (1.0 - roughness));
    return mix(perceivedFinalRoughness, 1.0, diffuseChance);
}

void main() 
{
    const float depth = texture(g_Depth, v_TexCoord).r;

    if (depth >= 1.0)
    {
        o_Color = texture(u_DirectLighting, v_TexCoord);
        return;
    }

    const vec3 position = WorldPosFromDepth(depth, v_TexCoord);
    const vec3 normal = texture(g_Normal, v_TexCoord).xyz;

    const vec2 roughness_metallic = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).xy;
    float roughness = roughness_metallic.x;
    const float metallic = roughness_metallic.y;

    // Determine ray direction for pixel (similar to VXGI logic)

    // Convention to make roughness feel more linear perceptually
    roughness *= roughness;

    float materialVariance = GetMaterialVariance(metallic, roughness);
    uint samples = uint(mix(1.0, float(u_MaxSamples), materialVariance));
    uint noiseIndex = u_Frame;

    uvec2 pixel = uvec2(v_TexCoord * u_ScreenDimensions);
    
    uint validSamples = 0;
    vec3 indirectLighting = vec3(0.0);

    for (uint i = 0; i < samples; i++)
    {
        InitializeRandomSeed(pixel.x + pixel.y * u_ScreenDimensions.x + u_Frame * (u_ScreenDimensions.x * u_ScreenDimensions.y));
        float rnd0 = GetRandomFloat01();
        float rnd1 = GetRandomFloat01();
        float rnd2 = GetRandomFloat01();

        const vec3 diffuseDir = CosineSampleHemisphere(normal, rnd0, rnd1);
        const vec3 viewDir = normalize(position - u_ViewPosition.xyz);

        vec3 worldRay;

        if (metallic > rnd2)
        {
            const vec3 reflectionDir = reflect(viewDir, normal);
            worldRay = normalize(mix(reflectionDir, diffuseDir, roughness));
        }
        else
            worldRay = diffuseDir;

        // Begin ray marching
        const float stepSize = 1.0;
        float t = stepSize;

        const float minStep = 0.1;
        const float maxStep = 50.0;

        for (int i = 0; i < 1000; i++)
        {
            const vec3 samplePos = position + worldRay * t;

            // Project to screen space
            vec4 clip = u_ViewProjection * vec4(samplePos, 1.0);
            vec3 screenSpace = clip.xyz / clip.w;

            const vec3 sampleUV = screenSpace * 0.5 + 0.5;

            // Terminate early if the ray is no longer on screen
            if (any(lessThan(sampleUV.xyz, vec3(0.0 - EPSILON))) || any(greaterThan(sampleUV.xyz, vec3(1 + EPSILON))))
                break;

            // Sample depth texture
            const float sceneDepth = texture(g_Depth, sampleUV.xy).r;

            if (sampleUV.z > sceneDepth)
            {
                indirectLighting += texture(u_DirectLighting, sampleUV.xy).rgb;
                validSamples++;

                break;
            }

            const float depthFactor = clamp(LinearDepth(sampleUV.z), minStep, maxStep);
            t += stepSize * depthFactor * depthFactor;
        }
    }

    if (validSamples != 0)
        indirectLighting /= float(validSamples);

    o_Color = vec4(texture(u_DirectLighting, v_TexCoord).rgb + indirectLighting, 1.0);
}