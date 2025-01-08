// VXGI Merge Voxel Compute Shader

// Note: This is only used if the TAKE_ATOMIC_FP16_PATH has not been set in VXGI
// shaders (i.e. the GL_NV_shader_atomic_fp16_vector is not supported).
// Merge all r32ui channel staging images into the main grid image.

#type compute
#version 450 core

layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout (rgba16f, binding = 0) restrict uniform image3D u_VoxelGrid;

layout(binding = 0) uniform sampler3D u_VoxelGridR;
layout(binding = 1) uniform sampler3D u_VoxelGridG;
layout(binding = 2) uniform sampler3D u_VoxelGridB;

void main()
{
    ivec3 voxelPosition = ivec3(gl_GlobalInvocationID);

    float a = imageLoad(u_VoxelGrid, voxelPosition).a;
    if (a > 0.0)
    {
        float r = texelFetch(u_VoxelGridR, voxelPosition, 0).r;
        float g = texelFetch(u_VoxelGridG, voxelPosition, 0).r;
        float b = texelFetch(u_VoxelGridB, voxelPosition, 0).r;
        imageStore(u_VoxelGrid, voxelPosition, vec4(r, g, b, a));
    }
}