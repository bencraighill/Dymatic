// VXGI Cone Tracing Compute Shader

#type compute
#version 450 core
#include Include/Buffers.glslh

#define EPSILON 1e-3

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (set = 0, binding = 0) restrict writeonly uniform image2D u_Output;

// Additional includes
#include Include/IndirectLighting.glslh

void main()
{
    const ivec2 outputSize = imageSize(u_Output);
    const ivec2 outputCoord = ivec2(gl_GlobalInvocationID.xy);

    if (outputCoord.x > outputSize.x || outputCoord.y > outputSize.y)
        return;

    const vec2 uv = (outputCoord + 0.5) / outputSize;

    float depth = texture(g_Depth, uv).r;
    if (depth == 1.0)
    {
        imageStore(u_Output, outputCoord, vec4(0.0));
        return;
    }

    const vec3 fragPos = PerspectiveTransformUvDepth(vec3(uv, depth * 2.0 - 1.0), u_InverseViewProjection);

    const vec3 normal = texture(g_Normal, uv).rgb;
    const float roughness = texture(g_Roughness_Metallic_Specular_AO, uv).r;
    const float metallic = texture(g_Roughness_Metallic_Specular_AO, uv).g;

    const vec3 viewDir = fragPos - u_ViewPosition.xyz;

    vec3 indirectLight = CalculateIndirectLighting(gl_GlobalInvocationID.xy, fragPos, viewDir, normal, roughness, metallic);

    imageStore(u_Output, outputCoord, vec4(indirectLight, 1.0));
}