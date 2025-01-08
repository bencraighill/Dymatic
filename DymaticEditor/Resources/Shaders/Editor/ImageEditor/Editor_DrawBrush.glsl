// Dymatic Image Editor Draw Brush Compute Shader

#type compute
#version 450 core
#include EditorBuffer.glslh

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (r8, binding = 0) restrict uniform image2D u_BrushBuffer;

float DistancePointToLine(vec2 p1, vec2 p2, vec2 p)
{
    const vec2 lineDir = p2 - p1;    
    const float t = clamp(dot(p - p1, lineDir) / dot(lineDir, lineDir), 0.0, 1.0);
    
    // Compute the closest point on the line segment
    const vec2 closestPoint = p1 + t * lineDir;
    
    // Return the distance from p to the closest point
    return length(p - closestPoint);
}

float GetBrushPattern(vec2 a, vec2 b)
{
    const float patternDistance =  DistancePointToLine(a, b, vec2(gl_GlobalInvocationID.xy));
    return max(1.0 - patternDistance / u_BrushRadius, 0.0);
}

void main()
{
    if (any(greaterThanEqual(gl_GlobalInvocationID.xy, u_CanvasSize)))
        return;

    float brush = imageLoad(u_BrushBuffer, ivec2(gl_GlobalInvocationID.xy)).r;

    const float currentLength = distance(u_MousePosition, u_PreviousDrawPosition);
    const float currentLengthInverse = 1.0 / currentLength;
    for (float dist = 0.0; dist <= currentLength; dist += u_BrushRadius)
    {
        brush += GetBrushPattern(mix(u_PreviousDrawPosition, u_MousePosition, dist * currentLengthInverse), mix(u_PreviousDrawPosition, u_MousePosition, min((dist + u_BrushRadius) * currentLengthInverse, 1.0)));
    }

    imageStore(u_BrushBuffer, ivec2(gl_GlobalInvocationID.xy), vec4(brush, 0.0, 0.0, 1.0));
}