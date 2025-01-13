// Dymatic Image Editor Clear Texture Compute Shader

#type compute
#version 450 core
#include EditorBuffer.glslh

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) restrict writeonly uniform image2D u_Image;

void main()
{
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, u_CanvasSize)))
        return;

    imageStore(u_Image, ivec2(gl_GlobalInvocationID.xy),  vec4(0.0));
}