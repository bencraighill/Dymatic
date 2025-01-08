// Volumetric Lighting Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout(set = 0, binding = 1) uniform sampler3D u_LightVoxelTexture;

layout (set = 0, binding = 13) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 14) uniform samplerCubeArray u_ShadowMapArray;

// Additional Includes
#include Include/DirectionalLighting.glslh

float LinearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
}

vec3 WorldPosFromDepth(float depth)
{
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(v_TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_InverseProjection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = u_InverseView * viewSpacePosition;

    return worldSpacePosition.xyz;
}

// Mie scaterring approximated with Henyey-Greenstein phase function.
float ComputeScattering(float lightDotView, float scatteringDistribution)
{
    float result = 1.0 - scatteringDistribution * scatteringDistribution;
    result /= (4.0 * PI * pow(1.0f + scatteringDistribution * scatteringDistribution - (2.0 * scatteringDistribution) * lightDotView, 1.5));
    return result;
}

bool IsInBounds(vec3 point, vec3 min, vec3 max)
{
    return point.x > min.x && point.y > min.y && point.z > min.z && point.x < max.x && point.y < max.y && point.z < max.z;
}

struct ScatteringData
{
    bool inside;

    vec3 color;
    float scatteringDistribution;
    float scatteringIntensity;
};

ScatteringData CalculateScatteringData(vec3 currentPosition) 
{
    ScatteringData scatteringData;
    scatteringData.inside = false;
    scatteringData.color = vec3(1.0);
    scatteringData.scatteringDistribution = 0.0;
    scatteringData.scatteringIntensity = 0.0;

    for (uint i = 0; i < u_VolumeCount; i++) 
    {
        if (IsInBounds(currentPosition, u_Volumes[i].min.xyz, u_Volumes[i].max.xyz)) 
        {
            scatteringData.inside = true;

            if (u_Volumes[i].blend == 0) 
            {
                // Set Mode
                scatteringData.color = u_Volumes[i].color.xyz;
                scatteringData.scatteringDistribution = u_Volumes[i].scatteringDistribution;
                scatteringData.scatteringIntensity = u_Volumes[i].scatteringIntensity;
            } else 
            {
                // Additive Mode
                scatteringData.color *= u_Volumes[i].color.xyz;
                scatteringData.scatteringDistribution += u_Volumes[i].scatteringDistribution;
                scatteringData.scatteringIntensity += u_Volumes[i].scatteringIntensity;
            }
        }
    }

    return scatteringData;
}

void main()
{
    float depth = texture(g_Depth, v_TexCoord).r;
    float linearDepth = LinearDepth(depth);
	vec3 position = WorldPosFromDepth(depth);
    vec3 viewDir = normalize(vec3(u_ViewPosition) - position);

    vec3 radianceOut = vec3(0.0);

    // Point Light Volumetrics
    if (u_PointLightCount != 0)
    {
        float stepSize = 0.05;
        vec3 p = u_ViewPosition.xyz;
        for (float i = 0; i < 4096; i++)
        {
            const float length = length(u_ViewPosition.xyz - p);
            if (length > 200.0 || length > linearDepth)
                break;

            ScatteringData scatteringData = CalculateScatteringData(p);

            if (scatteringData.inside)
            {
                vec3 gridPos = vec3(v_TexCoord, pow(length / 200.0, 1.0));
                radianceOut += texture(u_LightVoxelTexture, gridPos).rgb * stepSize * scatteringData.scatteringIntensity;
            }
        
            p += (-viewDir) * stepSize;
            stepSize *= 1.05;
        }
    }

    // Directional Lighting Volumetrics
    if (u_UsingDirectionalLight == 1)
    {
        vec3 rayVector = position.xyz - u_ViewPosition.xyz;
        float rayLength = length(rayVector);
        vec3 rayDirection = rayVector / rayLength;

        float stepLength = 0.025;

        vec3 currentPosition = u_ViewPosition.xyz;
        vec3 accumFog = vec3(0.0);

        while (length(u_ViewPosition.xyz - currentPosition) < linearDepth)
        {            
            ScatteringData scatteringData = CalculateScatteringData(currentPosition);

            if (scatteringData.inside)
            {
                accumFog = min(accumFog + ComputeScattering(dot(rayDirection, u_DirectionalLight.direction.xyz), scatteringData.scatteringDistribution).xxx * (u_DirectionalLight.color.xyz * u_DirectionalLight.color.w) * scatteringData.color
                    * (1.0 - DirectionalShadow(currentPosition, vec3(0.0, 0.0, 0.0))) * stepLength, scatteringData.scatteringIntensity);
            }

            currentPosition += rayDirection * stepLength;
            stepLength *= 1.01;
        }

        radianceOut += accumFog;
    }

    o_Color = vec4(radianceOut, 1.0);
}