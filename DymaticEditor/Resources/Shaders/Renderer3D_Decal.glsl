// Dymatic Decal Shader

#type vertex
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec3 a_Position;

void main()
{
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

layout (location = 0) out vec4 o_Albedo;
layout (location = 1) out int o_EntityID;

layout (set = 0, binding = 0) uniform sampler2D u_Albedo;

void main()
{
    const vec2 uv = gl_FragCoord.xy / u_ScreenDimensions.xy;
    const float depth = texture(g_Depth, uv).r;
    const vec3 position = WorldPosFromDepth(depth, uv);

    // Calculate local world position relative to the decal fragment position
    const vec3 localPos = (u_ModelInverse * vec4(position, 1.0)).xyz;
    const vec2 decalUV = localPos.xz * 0.5 + 0.5;
    
    const vec3 normal = vec3(0.0, 1.0, 0.0);
    const vec3 projectionDirectionWorldSpace = mat3(u_Model) * normal;

    // Note: Here we reuse u_Animated to represent u_ConstrainAngle
    if (u_Animated == 1)
    {
        // Discard pixels where angle between axis of projection and normal from GBuffer is greater than 0
        if (dot(projectionDirectionWorldSpace, normalize(texture(g_Normal, uv).xyz)) < 0.9)
            discard;
    }

    // Discard pixels that lie outside of projection box
	if (abs(localPos).x > 1.0 || abs(localPos).z > 1.0 || abs(localPos).y > 1.0)
		discard;

    const vec4 albedo = texture(u_Albedo, decalUV);

    if (albedo.a == 0.0)
        discard;

    o_Albedo = albedo;
    o_EntityID = u_EntityID;
}