CompilerInclude(Template_Surface)
CompilerInclude(Template_SceneDefinitions)

layout (location = 0) out vec4 o_Albedo;
layout (location = 1) out int o_EntityID;
layout (location = 2) out vec4 o_Normal;
layout (location = 3) out vec4 o_Emissive;
layout (location = 4) out vec4 o_Roughness_Metallic_Specular_AO;
layout (location = 5) out int o_SubmeshIndex;
layout (location = 6) out vec2 o_Velocity;

void main()
{
    Compiler(Properties)

    o_Albedo = vec4(albedo, 1.0);
    o_EntityID = u_EntityID;
    o_Emissive = vec4(emissive, 1.0);
    o_Roughness_Metallic_Specular_AO = vec4(roughness, metallic, specular, ao);
    o_SubmeshIndex = u_SubmeshIndex;

CompilerIf(Normal)
    normal = (2.0 * normal - 1.0);
    o_Normal = vec4(normalize(Input.TBN * normal), 1.0); //going -1 to 1
CompilerElse()
    o_Normal = vec4(normalize(Input.Normal), 1.0);
CompilerEndIf()

    Compiler(DepthOffset)

CompilerIf(DepthOffset)
    const float linearDepth = LinearDepth(gl_FragCoord.z) + depthOffset;
    gl_FragDepth = NonLinearDepth(linearDepth);
CompilerEndIf()

    // Velocity
    const vec2 uv = gl_FragCoord.xy / vec2(u_ScreenDimensions);
    // TODO: Include TAA Jitter here --- (uv * 2.0 - 1.0) - taaDataUBO.Jitter;
    const vec2 currentNdc = (uv * 2.0 - 1.0);
    const vec2 previousNdc = Input.PreviousClipPosition.xy / Input.PreviousClipPosition.w;
    o_Velocity = (currentNdc - previousNdc) * 0.5;
}

// End of Generated File