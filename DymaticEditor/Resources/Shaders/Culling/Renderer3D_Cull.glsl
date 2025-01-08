// Frustum and Occlusion Culling Compute Shader

#type compute
#version 450 core

#include Include/Buffers.glslh
#include Include/Box.glslh
#include Include/Frustum.glslh

#define IS_HI_Z_CULLING 1

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

void main ()
{
    uint meshIndex = gl_GlobalInvocationID.x;

    if (meshIndex >= u_MeshInstanceCount)
        return;

    MeshInstance meshInstance = u_MeshInstances[meshIndex];

    bool isVisible = true;

    Box meshLocalBounds = Box(meshInstance.Min.xyz, meshInstance.Max.xyz);

    Frustum frustum = GetFrustum(u_ViewProjection * meshInstance.ModelMatrix);
    isVisible = FrustumBoxIntersect(frustum, meshLocalBounds);

    // TODO: Hi-Z Culling? Maybe don't need with the current pre-depth test?
    
    if (isVisible)
    {
        uint index = atomicAdd(u_VisibleMeshCount, 1);
        u_VisibleMeshIndices[index] = meshIndex;
    }
}
