// Deferred Lighting

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

// Inputs/Outputs
layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

// Lighting Resources
layout (set = 0, binding = 13) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 14) uniform samplerCubeArray u_ShadowMapArray;

layout (set = 0, binding = 6) uniform sampler2D u_IndirectVoxelGrid;
layout (set = 0, binding = 7) uniform sampler3D u_PerlinWorley;

// Additional Includes
#define USE_LIGHT_GRID
#include Include/Lighting.glslh
#include Include/VolumetricClouds.glslh
#include Include/Math.glslh

void main()
{
	float depth = texture(g_Depth, v_TexCoord).r;
    float linearDepth = LinearDepth(depth);
	vec3 position = WorldPosFromDepth(depth, v_TexCoord);

	vec3 albedo = texture(sampler2D(g_Albedo), v_TexCoord).xyz;
	vec3 normal = texture(g_Normal, v_TexCoord).xyz;
    vec3 emissive = vec3(texture(g_Emissive, v_TexCoord).xyz);
	float roughness = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).x;
	float metallic = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).y;
	vec3 specular = vec3(texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).z);
	float ao = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).w;

    if (u_VisualizationMode == VISUALIZATION_MODE_LIGHTING_ONLY)
    {
        albedo = vec3(1.0);
        roughness = 0.5;
        metallic = 0.0;
        specular = vec3(0.5);
    }
    
    vec3 directLighting = CalculateLighting(albedo, normal, emissive, roughness, metallic, specular, ao, position, linearDepth);
    const vec3 indirectLighting = (u_UsingVXGI == 1 ? texture(u_IndirectVoxelGrid, v_TexCoord).rgb : vec3(u_Ambient)) * albedo;

    // Clouds
    if (u_UsingVolumetricClouds == 1)
        directLighting = CalculateVolumetricClouds(directLighting, position, linearDepth);

	o_Color = vec4(directLighting + indirectLighting, 1.0);
}