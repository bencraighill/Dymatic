// VXGI Mipmap Generation Shader

#type compute
#version 450 core
#include Include/Constants.glslh

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0) restrict writeonly uniform image3D u_Output;
layout(binding = 0) uniform sampler3D u_SamplerDownsample;

// Reuse submesh uniform buffer here
layout(std140, binding = SUBMESH_BUFFER_BINDING) uniform Submesh
{
    int u_Lod;
};

void main()
{
    const ivec3 imgSize = imageSize(u_Output);
    const ivec3 imgCoord = ivec3(gl_GlobalInvocationID);

    if (imgCoord.x > imgSize.x || imgCoord.y > imgSize.y)
        return;
    
    vec3 uvw = (imgCoord + 0.5) / imgSize;

    // Note: LOD provided is the LOD of the previously computed level.
    // We use the previous level to compute the next lower level mipmap
    vec4 result = textureLod(u_SamplerDownsample, uvw, u_Lod);

    result += textureLodOffset(u_SamplerDownsample, uvw, u_Lod, ivec3(-1,  0,  0));
    result += textureLodOffset(u_SamplerDownsample, uvw, u_Lod, ivec3( 1,  0,  0));

    result += textureLodOffset(u_SamplerDownsample, uvw, u_Lod, ivec3( 0, -1,  0));
    result += textureLodOffset(u_SamplerDownsample, uvw, u_Lod, ivec3( 0,  1,  0));

    result += textureLodOffset(u_SamplerDownsample, uvw, u_Lod, ivec3( 0,  0, -1));
    result += textureLodOffset(u_SamplerDownsample, uvw, u_Lod, ivec3( 0,  0,  1));

    result /= 7.0;

    imageStore(u_Output, imgCoord, result);
}