// Dymatic Image Editor Grid Compositing Compute Shader

#type compute
#version 450 core
#include EditorBuffer.glslh

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (rgba8, binding = 0) restrict uniform image2D u_Image;

void main()
{
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, u_CanvasSize)))
        return;

    const ivec2 imageCoord = ivec2(gl_GlobalInvocationID.xy);

    vec4 color = imageLoad(u_Image, imageCoord);

    if (color.a == 1.0)
        return;

    // Generate grid pattern
    const vec3 gridColor = ((int(imageCoord.x * u_Zoom) / 7 + int(imageCoord.y * u_Zoom) / 7) % 2 == 0) ? vec3(0.8) : vec3(0.2);
    color.rgb = mix(gridColor, color.rgb, color.a);
    color.a = 1.0;

    
    imageStore(u_Image, imageCoord, color);
}