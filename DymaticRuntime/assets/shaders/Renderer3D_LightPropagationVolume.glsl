#type compute
#version 450 core
#include assets/shaders/Buffers.glslh

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (rgba32f, binding = 0) writeonly uniform image3D u_VoxelGrid;

layout (set = 0, binding = 7) uniform samplerCubeArray u_ShadowMapArray;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

// This is used externally and will need to be updated if changed. TODO: move this to a glsl header.
float PointShadowCalculation(vec3 worldPos, vec3 lightPos, int shadowIndex)
{
    vec3 fragToLight = worldPos - lightPos;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = -0.1;
    int samples = 20;
    float viewDistance = length(vec3(u_ViewPosition) - worldPos);
    float diskRadius = 1 / 25.0; //(1.0 + (viewDistance / (/*Depth Far Plane*/25.0))) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(u_ShadowMapArray, vec4(vec3(fragToLight + gridSamplingDisk[i] * diskRadius), shadowIndex)).r;
        closestDepth *= /*Depth Far Plane*/25.0;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    return shadow;
}

vec3 calcPointLight(uint index, vec3 worldPos)
{
    //Point light basics
    vec3 position = u_PointLights[index].position.xyz;
    vec3 color    = 100.0 * u_PointLights[index].color.rgb * u_PointLights[index].intensity;
    float radius  = u_PointLights[index].range;

    //Attenuation calculation that is applied to all
    float distance    = length(position - worldPos);
    float attenuation = pow(clamp(1 - pow((distance / radius), 4.0), 0.0, 1.0), 2.0)/(1.0  + (distance * distance) );
    vec3 radianceIn   = color * attenuation;

    vec3 radiance = radianceIn;

    if (u_PointLights[index].shadowIndex != -1)
    {
        float shadow = PointShadowCalculation(worldPos, position, u_PointLights[index].shadowIndex);
        radiance *= (1.0 - shadow);
    }

    return radiance;
}

vec3 rand(vec3 vector) {
    // Use the input vec3 value to generate three random scalar values
    float r1 = fract(sin(dot(vector, vec3(12.9898, 78.233, 151.7182))) * 43758.5453);
    float r2 = fract(sin(dot(vector, vec3(89.123, 256.234, 36.7182))) * 53845.2345);
    float r3 = fract(sin(dot(vector, vec3(34.1234, 45.2345, 67.3456))) * 65432.8765);

    // Return a vec3 value composed of the three random scalar values
    return vec3(r1, r2, r3);
}

void main()
{
    ivec3 voxelIndex = ivec3(gl_GlobalInvocationID);
    vec3 worldPos = (vec3(voxelIndex) + vec3(0.5)) * VOXEL_SIZE;
    
    vec3 directLighting = vec3(0.0);
    
    for (uint index = 0; index < u_GlobalIndexCount; index++)
    {
        directLighting += calcPointLight(index, worldPos);
    }
    
    imageStore(u_VoxelGrid, voxelIndex, vec4(directLighting, 1.0));

    //ivec3 voxelIndex = ivec3(gl_GlobalInvocationID);
//
    //float voxelSizeZ = 25.0 / (float(GRID_SIZE_Z) + 0.5);
    //float clipSpaceSizeZ = voxelSizeZ * (u_ZFar - u_ZNear) / u_ZNear;
    //float clipSpacePosZ = (voxelIndex.z + 0.5) * clipSpaceSizeZ - (u_ZFar - u_ZNear) / 2.0;
//
    //vec2 rangedPos = (voxelIndex.xy + vec2(0.5)) / GRID_SIZE.xy;
    //vec4 clipSpacePos = vec4(((rangedPos) * 2.0) - vec2(1.0), clipSpacePosZ, 1.0);
//
    //vec4 worldPos = u_InverseProjection * clipSpacePos;
    //worldPos /= worldPos.w;
    //worldPos = u_InverseView * worldPos;
    //
    //vec3 directLighting = vec3(0.0);
    //
    //for (uint index = 0; index < u_GlobalIndexCount; index++)
    //{
    //    directLighting += calcPointLight(index, worldPos.xyz);
    //}
    //
    //imageStore(u_VoxelGrid, voxelIndex, vec4(directLighting, 1.0));

    //ivec3 voxelIndex = ivec3(gl_GlobalInvocationID);
    //vec4 clipSpacePos = vec4((((vec3(voxelIndex) + vec3(0.5)) / vec3(GRID_SIZE)) * 2.0) - vec3(1.0), 1.0);
//
    //mat4 inverseProjection = u_InverseProjection;
    //inverseProjection[2][2] = 160.0;
    //inverseProjection[2][3] = -80.0;
//
    //vec4 worldPos = inverseProjection * clipSpacePos;
    //worldPos /= worldPos.w;
    //worldPos = u_InverseView * worldPos;
    //
    //vec3 directLighting = vec3(0.0);
    //
    //for (uint index = 0; index < u_GlobalIndexCount; index++)
    //{
    //    directLighting += calcPointLight(index, worldPos.xyz);
    //}
//
    //imageStore(u_VoxelGrid, voxelIndex, vec4(directLighting, 1.0));
    
    
    //imageStore(u_VoxelGrid, voxelIndex, vec4(rand(voxelIndex), 1.0));
}