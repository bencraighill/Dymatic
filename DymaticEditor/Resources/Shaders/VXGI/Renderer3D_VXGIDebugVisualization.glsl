// VXGI Debug Visualization Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh
#include Include/Intersection.glslh
#include Include/TraceCone.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

void main()
{
    const Box gridBounds = Box(u_GridMin[0].xyz, u_GridMax[0].xyz);

    Ray worldRay;
    worldRay.Origin = u_ViewPosition.xyz;
    
    const vec3 focusPointLocal = vec3(v_TexCoord * 2.0 - 1.0, 1.0);
    const vec4 focusPointWorld = u_InverseViewProjection * vec4(focusPointLocal, 1.0);
    const vec3 focusPoint = focusPointWorld.xyz / focusPointWorld.w;

    worldRay.Direction = normalize(focusPoint - worldRay.Origin);

    float t1, t2;
    if (!(RayBoxIntersect(worldRay, gridBounds, t1, t2) && t2 > 0.0))
    {
        vec4 skyColor = texture(u_EnvironmentMap, worldRay.Direction);
        o_Color = skyColor;
        return;
    }

    bool isInsideGrid = t1 < 0.0 && t2 > 0.0;

    if (isInsideGrid)
        worldRay.Origin = u_ViewPosition.xyz;
    else
        worldRay.Origin = worldRay.Origin + worldRay.Direction * t1;

    vec4 color = TraceCone(worldRay, u_ConeAngle, u_StepMultiplier);
    color += (1.0 - color.a) * texture(u_EnvironmentMap, worldRay.Direction);

    o_Color = color;
}