// Dymatic Image Editor Apply Brush Compute Shader

#type compute
#version 450 core
#include EditorBuffer.glslh

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (set = 0, binding = 0) uniform sampler2D u_BrushBuffer;
layout (rgba8, binding = 0) restrict uniform image2D u_LayerBuffer;

void main()
{
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, u_CanvasSize)))
        return;

    const ivec2 imageCoord = ivec2(gl_GlobalInvocationID.xy);

    const float brush = texelFetch(u_BrushBuffer, imageCoord, 0).r * u_BrushColor.a;
    
    vec4 color = imageLoad(u_LayerBuffer, imageCoord);
    color.rgb = mix(color.rgb, u_BrushColor.rgb, brush);
    color.a = max(color.a, brush);

    imageStore(u_LayerBuffer, imageCoord, color);
}