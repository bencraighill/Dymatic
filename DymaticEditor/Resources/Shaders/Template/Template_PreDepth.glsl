Compiler(Comment)
// Usage: Dymatic Pre Depth Shader

CompilerInclude(Template_ConstantDefinitions)
CompilerDefine(World Position, Input.Position);
CompilerDefine(World Normal, Input.Normal);
CompilerDefine(Texture Coordinates, Input.TexCoord);
CompilerDefine(Pixel Position, vec2(gl_FragCoord.xy) / vec2(u_ScreenDimensions));

#type vertex
#version 450 core
#include Include/Buffers.glslh
#include Include/BlendShapes.glslh

layout (location = 0) in uint v_GlobalIndex;
layout (location = 1) in vec3 v_Position;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec2 v_TexCoord;
layout (location = 4) in vec3 v_Tangent;
layout (location = 5) in vec3 v_Bitangent;
layout (location = 6) in vec4 v_Color;
layout (location = 7) in ivec4 v_BoneIDs;
layout (location = 8) in vec4 v_Weights;

struct VertexOutput
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
};

layout (location = 0) out VertexOutput Input;

Compiler(ExpressionHeader)

Compiler(Buffers)

void main()
{
    vec3 position = v_Position + BlendShapeOffset(v_GlobalIndex);

    if (u_Animated == 1)
    {
        // Animation Calculation
        vec4 totalPosition = vec4(0.0);
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (v_BoneIDs[i] == -1)
                continue;

            if (v_BoneIDs[i] >= MAX_BONES)
            {
                totalPosition = vec4(position, 1.0);
                break;
            }

            // Position
            vec4 localPosition = u_FinalBonesMatrices[v_BoneIDs[i]] * vec4(position, 1.0f);
            totalPosition += localPosition * v_Weights[i];
        }

        position = totalPosition.xyz;
    }

    Input.Position = vec3(u_Model * vec4(position, 1.0));
    Input.Normal = v_Normal;
    Input.TexCoord = v_TexCoord;


CompilerIf(!Tessellation)
    Compiler(Displacement)
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

struct VertexOutput
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
};

in VertexOutput Input[];
out VertexOutput TessOutput[];

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

struct VertexOutput
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
};

in VertexOutput TessOutput[];
out VertexOutput Input;

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

    Compiler(Displacement)

    gl_Position = u_ViewProjection * vec4(Input.Position, 1.0);
}

CompilerEndIf()

#type fragment
#version 450 core
#extension GL_ARB_bindless_texture: require
#include Include/Buffers.glslh
#include Include/Math.glslh

Compiler(ExpressionHeader)

struct VertexOutput
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
};

layout (location = 0) in VertexOutput Input;

Compiler(Buffers)

float noise() { return fract(sin(dot(Input.TexCoord, vec2(12.9898, 78.233))) * 43758.5453); }

void main()
{
    Compiler(Alpha)
    Compiler(PreDepth)

    Compiler(DepthOffset)

CompilerIf(DepthOffset)
    const float linearDepth = LinearDepth(gl_FragCoord.z) + depthOffset;
    gl_FragDepth = NonLinearDepth(linearDepth);
CompilerEndIf()
}