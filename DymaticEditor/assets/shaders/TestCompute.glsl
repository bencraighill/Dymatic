#type compute
#version 450 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0, rgba8) uniform readonly image2D inputImage;
layout (binding = 1, rgba8) uniform image2D resultImage;

void main(void)
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = imageLoad(inputImage, pixelCoord);
    imageStore(resultImage, pixelCoord, vec4(color.r * -1.0 + 1.0, color.g * -1.0 + 1.0, color.b * -1.0 + 1.0, 1.0));
}