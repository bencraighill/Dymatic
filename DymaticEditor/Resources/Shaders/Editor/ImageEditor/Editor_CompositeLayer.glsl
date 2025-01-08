// Dymatic Image Editor Grid Compositing Compute Shader

#type compute
#version 450 core
#include EditorBuffer.glslh

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (set = 0, binding = 0) uniform sampler2D u_Layer;
layout (rgba8, binding = 0) restrict uniform image2D u_Result;

void main()
{
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, u_CanvasSize)))
        return;

    const ivec2 imageCoord = ivec2(gl_GlobalInvocationID.xy);

    vec4 color = imageLoad(u_Result, imageCoord);

    if (color.a == 1.0)
        return;

    const vec4 layerColor = texelFetch(u_Layer, imageCoord, 0);
    color.rgb = mix(color.rgb, layerColor.rgb, layerColor.a);
    color.a = max(color.a, layerColor.a);
    
    imageStore(u_Result, imageCoord, color);
}