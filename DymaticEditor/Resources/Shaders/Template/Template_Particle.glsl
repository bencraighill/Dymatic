Compiler(Comment)
// Usage: Dymatic Particle Shader

CompilerInclude(Template_ConstantDefinitions)

CompilerDefine(World Position, v_Position);
CompilerDefine(World Normal, -u_Forward);
CompilerDefine(Texture Coordinates, v_TexCoord);
CompilerDefine(Object Position, u_Particles[PARTICLE_INDEX].position.xyz);
CompilerDefine(Pixel Position, vec2(gl_FragCoord.xy) / vec2(u_ScreenDimensions));
CompilerDefine(Entity ID, float(u_EntityID));
CompilerDefine(Submesh Index, float(PARTICLE_INDEX));

CompilerDefine(Particle Lifetime, u_Particles[PARTICLE_INDEX].lifeTime);
CompilerDefine(Particle Life Remaining, u_Particles[PARTICLE_INDEX].lifeRemaining);
CompilerDefine(Particle Life Weight, (u_Particles[PARTICLE_INDEX].lifeRemaining / u_Particles[PARTICLE_INDEX].lifeTime));
CompilerDefine(Particle Velocity, u_Particles[PARTICLE_INDEX].velocity.xyz);

#type vertex
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

Compiler(ExpressionHeader)

layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_Index;

layout (location = 0) out vec3 v_Position;
layout (location = 1) out vec2 v_TexCoord;
layout (location = 2) out flat int v_Index;

CompilerIf(DeferredLit)
layout (location = 3) out vec2 v_Velocity;
CompilerEndIf()

#define PARTICLE_POSITION a_Position
#define PARTICLE_INDEX a_Index

Compiler(Buffers)

void main()
{
    // Attempt to performantly discard the vertex if it is being drawn but has no life remaining
    // This is quite experimental (Try disabling this if issues occur)
    if (u_Particles[a_Index].lifeRemaining == 0.0)
    {
        gl_CullDistance[0] = -1.0;
        return;
    }

    v_Index = a_Index;
    v_TexCoord = (a_Position.xy + vec2(1.0)) * 0.5;

    v_Position = u_Particles[a_Index].position.xyz;

    Compiler(Displacement)

    Compiler(ParticleSize)

    v_Position += (u_Right.xyz * a_Position.x * particleSize.x) + (u_Up.xyz * a_Position.y * particleSize.y);

    const vec4 currentClipSpacePos = u_ViewProjection * vec4(v_Position, 1.0);

CompilerIf(DeferredLit)
    // Compute screenspace velocity
    const vec3 currentNDCPosition = currentClipSpacePos.xyz / currentClipSpacePos.w;

    const vec3 velocity = u_Particles[a_Index].velocity.xyz;
    const vec4 futureClipSpacePos = u_ViewProjection * vec4(v_Position + velocity, 1.0);
    const vec3 futureNDCPosition = futureClipSpacePos.xyz / futureClipSpacePos.w;

    v_Velocity = futureNDCPosition.xy - currentNDCPosition.xy;
    v_Velocity = vec2(1000.0, 1000.0);
CompilerEndIf()

    gl_Position = currentClipSpacePos;
}

#type fragment
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

CompilerIf(TranslucentLit)
layout (set = 0, binding = 13) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 14) uniform samplerCubeArray u_ShadowMapArray;

#include Include/Lighting.glslh
#include Include/IndirectLighting.glslh
CompilerEndIf()

Compiler(ExpressionHeader)

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec2 v_TexCoord;
layout (location = 2) in flat int v_Index;

CompilerIf(DeferredLit)
layout (location = 3) in vec2 v_Velocity;
CompilerEndIf()

layout (location = 0) out vec4 o_Albedo;
layout (location = 1) out int o_EntityID;

CompilerIf(DeferredLit)
layout (location = 2) out vec4 o_Normal;
layout (location = 3) out vec4 o_Emissive;
layout (location = 4) out vec4 o_Roughness_Metallic_Specular_AO;
layout (location = 5) out int o_SubmeshIndex;
layout (location = 6) out vec2 o_Velocity;
CompilerEndIf()

#define PARTICLE_POSITION v_Position
#define PARTICLE_INDEX v_Index

Compiler(Buffers)

void main()
{
    Compiler(Properties)

    // Particles manually define normal as forward vector if not provided
CompilerIf(Normal)
CompilerElse()
    vec3 normal = u_Forward.xyz;
CompilerEndIf()

    // Note: If we ever have a pre-depth pass for particle systems, this should be moved there!
    Compiler(PreDepth)

CompilerIf(TranslucentLit)
    const float linearDepth = length(u_ViewPosition.xyz - v_Position);
    const vec3 directLighting = CalculateLighting(albedo, normal, emissive, roughness, metallic, vec3(specular), ao, v_Position, linearDepth);
    vec3 surfaceColor = directLighting;

#if TRANSLUCENT_VXGI
    if (u_UsingVXGI == 1)
    {
        const uvec2 pixel = uvec2(gl_FragCoord.xy);
        const vec3 viewDir = normalize(v_Position - u_ViewPosition.xyz);
        const vec3 indirectLighting = CalculateIndirectLighting(pixel, v_Position, viewDir, normal, roughness, metallic);
        surfaceColor += indirectLighting;
    }
#endif

    o_Albedo = vec4(surfaceColor, alpha);
CompilerElse()
    o_Albedo = vec4(albedo, alpha);
CompilerEndIf()

    o_EntityID = u_EntityID;

    // Unlit and Transparent do not write to deferred buffers
CompilerIf(DeferredLit)
    o_Normal = vec4(normal, 1.0);
    o_Emissive = vec4(emissive, 1.0);
    o_Roughness_Metallic_Specular_AO = vec4(roughness, metallic, specular, ao);
    o_SubmeshIndex = v_Index;
    o_Velocity = v_Velocity;
CompilerEndIf()

}