CompilerInclude(Template_Surface)
CompilerInclude(Template_SceneDefinitions)

layout (location = 0) out vec4 o_Color;
layout (set = 0, binding = 2) uniform sampler2D u_SceneColor;

layout (set = 0, binding = 13) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 14) uniform samplerCubeArray u_ShadowMapArray;

// Additional Includes
#define USE_LIGHT_GRID
#include Include/Lighting.glslh
#include Include/IndirectLighting.glslh

void main()
{
    Compiler(Properties)

CompilerIf(Normal)
    normal = (2.0 * normal - 1.0);
    normal = normalize(Input.TBN * normal); //going -1 to 1
CompilerElse()
    vec3 normal = normalize(Input.Normal);
CompilerEndIf()

    const float linearDepth = length(u_ViewPosition.xyz - Input.Position);

    // Unlike the main deferred pre pass shader we now forward draw the object

    // Calculate the refracted color
    vec2 screenTexCoord = vec2(gl_FragCoord.xy) / vec2(u_ScreenDimensions.xy);
    vec4 clipNormal = u_ViewProjection * vec4(normal, 1.0);
    vec3 ndcNormal = clipNormal.xyz / clipNormal.w;
    vec3 refractedColor = texture(u_SceneColor, (screenTexCoord + ndcNormal.xy * 0.1 * (1.0 - ior))).rgb;

    // Calculate the surface color
    const vec3 directLighting = CalculateLighting(albedo, normal, emissive, roughness, metallic, vec3(specular), ao, Input.Position, linearDepth);
    vec3 surfaceColor = directLighting;

#if TRANSLUCENT_VXGI
    if (u_UsingVXGI == 1)
    {
        const uvec2 pixel = uvec2(gl_FragCoord.xy);
        const vec3 viewDir = normalize(Input.Position - u_ViewPosition.xyz);
        const vec3 indirectLighting = CalculateIndirectLighting(pixel, Input.Position, viewDir, normal, roughness, metallic);
        surfaceColor += indirectLighting;
    }
#endif
    
    // Blend refracted and surface color by alpha
    o_Color = vec4(mix(refractedColor, surfaceColor, alpha), 1.0);
}

// End of Generated File