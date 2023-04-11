// SSAO Post Processing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

float noiseSize = 4.0;
const int kernelSize = 64;
const float radius = 0.5;
const float bias = 0.025;

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (set = 0, binding = 0) uniform sampler2D u_Depth;
layout (set = 0, binding = 1) uniform sampler2D u_Normal;
layout (set = 0, binding = 2) uniform sampler2D u_TexNoise;

vec3 PositionFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(v_TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_InverseProjection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

void main()
{
    // tile noise texture over screen based on screen dimensions divided by noise size
    const vec2 noiseScale = vec2(u_ScreenDimensions) / vec2(noiseSize);

    // get input for SSAO algorithm
    vec3 fragPos = PositionFromDepth(texture(u_Depth, v_TexCoord).r).xyz;
    vec3 normal = normalize(texture(u_Normal, v_TexCoord).rgb);
    vec3 randomVec = normalize(texture(u_TexNoise, v_TexCoord * noiseScale).xyz * 2.0 - vec3(1.0, 1.0, 0.0));
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = vec3(TBN * vec3(u_SSAOSamples[i])); // from tangent to view-space
        samplePos = fragPos + samplePos * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = u_Projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = PositionFromDepth(texture(u_Depth, offset.xy).r).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = /*1.0 - */(occlusion / kernelSize);
    
    o_Color = vec4(vec3(0.0), occlusion);
}