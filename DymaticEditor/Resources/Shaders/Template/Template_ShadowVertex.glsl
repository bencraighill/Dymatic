CompilerInclude(Template_ConstantDefinitions)
CompilerDefine(World Position, worldPosition);
CompilerDefine(World Normal, v_Normal);
CompilerDefine(Texture Coordinates, v_TexCoord);
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

layout (location = 0) out vec2 o_TexCoord;

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

    vec3 worldPosition = vec3(u_Model * vec4(position, 1.0));

    Compiler(Displacement)

    o_TexCoord = v_TexCoord;
    gl_Position = vec4(worldPosition, 1.0);
}