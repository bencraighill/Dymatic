// VXGI Clear Voxel Compute Shader

#type compute
#version 450 core

layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout (rgba16f, binding = 0) restrict uniform image3D u_VoxelGrid;

#if !TAKE_ATOMIC_FP16_PATH
layout(binding = 1) restrict writeonly uniform uimage3D u_VoxelGridR;
layout(binding = 2) restrict writeonly uniform uimage3D u_VoxelGridG;
layout(binding = 3) restrict writeonly uniform uimage3D u_VoxelGridB;
#endif

void main()
{
    ivec3 voxelPosition = ivec3(gl_GlobalInvocationID);

#if TAKE_ATOMIC_FP16_PATH

    imageStore(u_VoxelGrid, voxelPosition, vec4(0.0));

#else

    bool isNotEmpty = imageLoad(u_VoxelGrid, voxelPosition).a > 0.0;
    if (isNotEmpty)
    {
        imageStore(u_VoxelGridR, voxelPosition, uvec4(0));
        imageStore(u_VoxelGridG, voxelPosition, uvec4(0));
        imageStore(u_VoxelGridB, voxelPosition, uvec4(0));
        imageStore(u_VoxelGrid, voxelPosition, vec4(0.0));
    }

#endif
}