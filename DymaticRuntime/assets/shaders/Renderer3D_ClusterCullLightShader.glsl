#type compute
#version 430 core
#include assets/shaders/Buffers.glslh

layout(local_size_x = LIGHT_CULL_SIZE_X, local_size_y = LIGHT_CULL_SIZE_Y, local_size_z = LIGHT_CULL_SIZE_Z) in;

//Shared variables
shared PointLight sharedLights[LIGHT_CULL_SIZE_X*LIGHT_CULL_SIZE_Y*LIGHT_CULL_SIZE_Z];

bool testSphereAABB(uint light, uint tile);
float sqDistPointAABB(vec3 point, uint tile);

void main()
{
    u_GlobalIndexCount = 0;
    uint threadCount = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
    uint numBatches = (u_PointLightCount + threadCount -1) / threadCount;

    uint tileIndex = gl_LocalInvocationIndex + gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z * gl_WorkGroupID.z;
    
    uint visibleLightCount = 0;
    uint visibleLightIndices[100];

    for( uint batch = 0; batch < numBatches; batch++)
    {
        uint lightIndex = batch * threadCount + gl_LocalInvocationIndex;

        //Prevent overflow by clamping to last light which is always null
        lightIndex = min(lightIndex, u_PointLightCount);

        //Populating shared light array
        sharedLights[gl_LocalInvocationIndex] = u_PointLights[lightIndex];
        barrier();

        //Iterating within the current batch of lights
        for( uint light = 0; light < threadCount; light++)
        {
            if(sharedLights[light].enabled  == 1)
            {
                if(testSphereAABB(light, tileIndex))
                {
                    visibleLightIndices[visibleLightCount] = batch * threadCount + light;
                    visibleLightCount += 1;
                }
            }
        }
    }

    //We want all thread groups to have completed the light tests before continuing
    barrier();

    uint offset = atomicAdd(u_GlobalIndexCount, visibleLightCount);

    for(uint i = 0; i < visibleLightCount; i++)
    {
        u_GlobalLightIndexList[offset + i] = visibleLightIndices[i];
    }

    u_LightGrid[tileIndex].offset = offset;
    u_LightGrid[tileIndex].count = visibleLightCount;
}

bool testSphereAABB(uint light, uint tile)
{
    float radius = sharedLights[light].range;
    vec3 center  = vec3(u_View * sharedLights[light].position);
    float squaredDistance = sqDistPointAABB(center, tile);

    return squaredDistance <= (radius * radius);
}

float sqDistPointAABB(vec3 point, uint tile)
{
    float sqDist = 0.0;
    VolumeTileAABB currentCell = u_Cluster[tile];
    u_Cluster[tile].maxPoint[3] = tile;
    for(int i = 0; i < 3; ++i)
    {
        float v = point[i];
        if(v < currentCell.minPoint[i])
        {
            sqDist += (currentCell.minPoint[i] - v) * (currentCell.minPoint[i] - v);
        }
        if(v > currentCell.maxPoint[i])
        {
            sqDist += (v - currentCell.maxPoint[i]) * (v - currentCell.maxPoint[i]);
        }
    }

    return sqDist;
}