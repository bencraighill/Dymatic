Compiler(Comment)
// Usage: Dymatic Default Lighting Shader

CompilerInclude(Template_SurfaceDefinitions)

// Begin Surface Template

#type vertex
#version 450 core
#include Include/Buffers.glslh
#include Include/BlendShapes.glslh
CompilerIf(Normal)
#include Include/TBN.glslh
CompilerEndIf()

layout (location = 0) in uint a_GlobalIndex;
layout (location = 1) in vec3 a_Position;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in vec3 a_Tangent;
layout (location = 5) in vec3 a_Bitangent;
layout (location = 6) in vec4 a_Color;
layout (location = 7) in ivec4 a_BoneIDs;
layout (location = 8) in vec4 a_Weights;

#define SURFACE_POSITION Input.Position
#define SURFACE_NORMAL a_Normal
#define SURFACE_TEXTURE_COORD a_TexCoord
#define SURFACE_COLOR a_Color

CompilerInclude(Template_VertexOutput)

// Note: This would typically be called 'Output' in Dymatic vertex shaders but
// has been conveniently called 'Input' for consistency with the fragment shader.
layout (location = 0) out VertexOutput Input;

Compiler(ExpressionHeader)

Compiler(Buffers)

void main()
{
    vec3 position = a_Position + BlendShapeOffset(a_GlobalIndex);

    if (u_Animated == 1)
    {
        // Animation Calculation
        vec4 totalPosition = vec4(0.0);
        vec3 totalNormal = vec3(0.0);
        for (uint i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (a_BoneIDs[i] == -1)
                continue;

            if (a_BoneIDs[i] >= MAX_BONES)
            {
                totalPosition = vec4(position, 1.0);
                totalNormal = a_Normal;
                break;
            }

            // Position
            vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(position, 1.0);
            totalPosition += localPosition * a_Weights[i];

            // Normal
            vec3 localNormal = mat3(u_FinalBonesMatrices[a_BoneIDs[i]]) * a_Normal;
            totalNormal += localNormal * a_Weights[i];
        }

        position = totalPosition.xyz;
        Input.Normal = mat3(u_ModelInverse) * totalNormal;
    }
    else
    {
        Input.Normal = mat3(u_ModelInverse) * a_Normal;
    }

    // Calculate and output world space position
    Input.Position = vec3(u_Model * vec4(position, 1.0));

CompilerIf(!Tessellation)
    Compiler(Displacement)
CompilerEndIf()

    // Final vertex position and texture coordinates
    Input.TexCoord = a_TexCoord;

CompilerIf(VertexIndex)
    Input.GlobalIndex = a_GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    Input.Color = a_Color;
CompilerEndIf()

CompilerIf(Normal)
    Input.TBN = CalculateTBN(a_Normal, a_Tangent);
    Input.TangentViewPos = Input.TBN * vec3(u_ViewPosition);
    Input.TangentFragPos = Input.TBN * Input.Position;
CompilerEndIf()

    // Calculate the previous world clip space position for usage with velocity buffer
    Input.PreviousClipPosition = u_PreviousViewProjection * u_PreviousModel * vec4(position, 1.0);
    
CompilerIf(!Tessellation)
    gl_Position = u_ViewProjection * vec4(Input.Position, 1.0);
CompilerEndIf()
}

CompilerIf(Tessellation)

#type control
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

layout (vertices = 3) out;

Compiler(ExpressionHeader)
CompilerInclude(Template_VertexOutput)

in VertexOutput Input[];
out VertexOutput TessOutput[];

#define SURFACE_POSITION Input[gl_InvocationID].Position
#define SURFACE_NORMAL Input[gl_InvocationID].Normal
#define SURFACE_TEXTURE_COORD Input[gl_InvocationID].TexCoord
#define SURFACE_COLOR Input[gl_InvocationID].Color

Compiler(Buffers)

void main()
{
    Compiler(TessellationMultiplier)

    gl_TessLevelOuter[0] = tessellationMultiplier;
    gl_TessLevelOuter[1] = tessellationMultiplier;
    gl_TessLevelOuter[2] = tessellationMultiplier;
    gl_TessLevelInner[0] = tessellationMultiplier;
    
    TessOutput[gl_InvocationID] = Input[gl_InvocationID];
}

#type evaluation
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

layout (triangles, Compiler(TessellationSpacing), ccw) in;

Compiler(ExpressionHeader)
CompilerInclude(Template_VertexOutput)

in VertexOutput TessOutput[];
out VertexOutput Input;

#define SURFACE_POSITION Input.Position
#define SURFACE_NORMAL Input.Normal
#define SURFACE_TEXTURE_COORD Input.TexCoord
#define SURFACE_COLOR Input.Color

Compiler(Buffers)

void main()
{
    Input.Position = gl_TessCoord.x * TessOutput[0].Position +
                     gl_TessCoord.y * TessOutput[1].Position +
                     gl_TessCoord.z * TessOutput[2].Position;

    Input.Normal = normalize(
        gl_TessCoord.x * TessOutput[0].Normal +
        gl_TessCoord.y * TessOutput[1].Normal +
        gl_TessCoord.z * TessOutput[2].Normal
    );

    Input.TexCoord = gl_TessCoord.x * TessOutput[0].TexCoord +
                     gl_TessCoord.y * TessOutput[1].TexCoord +
                     gl_TessCoord.z * TessOutput[2].TexCoord;

    Input.PreviousClipPosition = gl_TessCoord.x * TessOutput[0].PreviousClipPosition +
                                gl_TessCoord.y * TessOutput[1].PreviousClipPosition +
                                gl_TessCoord.z * TessOutput[2].PreviousClipPosition;

CompilerIf(VertexIndex)
    Input.GlobalIndex = TessOutput[0].GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    Input.Color = gl_TessCoord.x * TessOutput[0].Color +
                gl_TessCoord.y * TessOutput[1].Color +
                gl_TessCoord.z * TessOutput[2].Color;
CompilerEndIf()

CompilerIf(Normal)
    Input.TangentViewPos = gl_TessCoord.x * TessOutput[0].TangentViewPos +
                        gl_TessCoord.y * TessOutput[1].TangentViewPos +
                        gl_TessCoord.z * TessOutput[2].TangentViewPos;

    Input.TangentFragPos = gl_TessCoord.x * TessOutput[0].TangentFragPos +
                        gl_TessCoord.y * TessOutput[1].TangentFragPos +
                        gl_TessCoord.z * TessOutput[2].TangentFragPos;

    Input.TBN = gl_TessCoord.x * TessOutput[0].TBN +
                gl_TessCoord.y * TessOutput[1].TBN +
                gl_TessCoord.z * TessOutput[2].TBN;
CompilerEndIf()

    Compiler(Displacement)

    gl_Position = u_ViewProjection * vec4(Input.Position, 1.0);
}

CompilerEndIf()

#type fragment
#version 450 core
#extension GL_ARB_bindless_texture: require
#include Include/Buffers.glslh
#include Include/Math.glslh

#define SURFACE_POSITION Input.Position
#define SURFACE_NORMAL Input.Normal
#define SURFACE_TEXTURE_COORD Input.TexCoord
#define SURFACE_COLOR Input.Color

Compiler(ExpressionHeader)

CompilerInclude(Template_VertexOutput)

layout (location = 0) in VertexOutput Input;

Compiler(Buffers)

// End Surface Template