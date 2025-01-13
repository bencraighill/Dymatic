#type compute
#version 450 core
#include Include/Buffers.glslh

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (rgba32f, binding = 0) writeonly uniform image3D u_VoxelGrid;

layout (set = 0, binding = 14) uniform samplerCubeArray u_ShadowMapArray;

// Additional Includes
#define POINT_SHADOW_NO_NORMAL
#include Include/PointLighting.glslh

#define USE_RANDOM_NOISE 1
#include Include/IndirectLighting.glslh

vec3 rand(vec3 vector) {
    // Use the input vec3 value to generate three random scalar values
    float r1 = fract(sin(dot(vector, vec3(12.9898, 78.233, 151.7182))) * 43758.5453);
    float r2 = fract(sin(dot(vector, vec3(89.123, 256.234, 36.7182))) * 53845.2345);
    float r3 = fract(sin(dot(vector, vec3(34.1234, 45.2345, 67.3456))) * 65432.8765);

    // Return a vec3 value composed of the three random scalar values
    return vec3(r1, r2, r3);
}

vec3 PointLighting(uint index, vec3 worldPos)
{
    // Point light basics
    vec3 position = u_PointLights[index].position.xyz;
    vec3 color    = u_PointLights[index].color.rgb * u_PointLights[index].intensity;
    float radius  = u_PointLights[index].range;

    // Attenuation calculation that is applied to all
    float distance    = length(position - worldPos);
    float attenuation = pow(clamp(1 - pow((distance / radius), 4.0), 0.0, 1.0), 2.0)/(1.0  + (distance * distance) );
    vec3 radianceIn   = color * attenuation;

    vec3 radiance = (1.0 / PI) * radianceIn;

    if (u_PointLights[index].shadowIndex != -1)
    {
        float shadow = PointShadow(worldPos, vec3(0.0), position, radius, u_PointLights[index].shadowIndex);
        radiance *= (1.0 - shadow);
    }

    return radiance;
}

#ifdef LIGHT_VOLUME_VXGI
const vec3 sampleNormals[] = {
    vec3( 0,  0,  1),
    vec3( 0,  0, -1),
    vec3( 0,  1,  0),
    vec3( 0, -1,  0),
    vec3( 1,  0,  0),
    vec3(-1,  0,  0),
};
#endif

void main()
{
    const vec3 gridSize = vec3(u_VolumetricGridSize);

    const float voxelSize = 200.0 / gridSize.z;
    const float voxelArea = voxelSize * voxelSize * voxelSize;

    ivec3 voxelIndex = ivec3(gl_GlobalInvocationID);

    vec2 rangedPos = (vec2(voxelIndex.xy) + vec2(0.5)) / gridSize.xy;
    vec2 ndc = (rangedPos - 0.5) * 2.0;
    vec4 clip = vec4(ndc, 1.0, 1.0);
    vec4 view = u_InverseProjection * clip;
    view /= view.w;
    vec3 worldDir = normalize((u_InverseView * view).xyz - u_ViewPosition.xyz);

    vec3 worldPos = worldDir * 200.0 * pow(float(voxelIndex.z) / gridSize.z, 1.0) + u_ViewPosition.xyz;

    vec3 directLighting = vec3(0.0);

    // TODO: Do we need to search all these lights? Can we use computed light culling grid?
    for (uint index = 0; index < u_PointLightCount; index++)
        directLighting += PointLighting(index, worldPos.xyz);

#ifdef LIGHT_VOLUME_VXGI
    for (uint i = 0; i < sampleNormals.length(); i++)
        directLighting += CalculateIndirectLighting(uvec2(voxelIndex.xy), worldPos, worldDir, sampleNormals[i], 1.0, 0.0);
#endif
    
    directLighting /= voxelArea;

    imageStore(u_VoxelGrid, voxelIndex, vec4(directLighting, 1.0));
}