// Editor Arrow Material Shader

#type vertex
#version 450 core
#include Include/Buffers.glslh
#include Include/BlendShapes.glslh

layout (location = 0) in uint a_GlobalIndex;
layout (location = 1) in vec3 a_Position;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in vec3 a_Tangent;
layout (location = 5) in vec3 a_Bitangent;
layout (location = 6) in vec4 a_Color;
layout (location = 7) in ivec4 a_BoneIDs;
layout (location = 8) in vec4 a_Weights;

struct VertexOutput
{
	vec3 Normal;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	vec3 position = a_Position + BlendShapeOffset(a_GlobalIndex);

	if (u_Animated == 1)
	{
		// Animation Calculation
    	vec4 totalPosition = vec4(0.0);
		vec3 totalNormal = vec3(0.0);
    	for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    	{
    	    if(a_BoneIDs[i] == -1)
    	        continue;
    	    if(a_BoneIDs[i] >= MAX_BONES) 
    	    {
    	        totalPosition = vec4(position, 1.0f);
				totalNormal = a_Normal;
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];

			// Normal
    	    vec3 localNormal = mat3(u_FinalBonesMatrices[a_BoneIDs[i]]) * a_Normal;
    	    totalNormal += localNormal * a_Weights[i];
    	}

        gl_Position = u_ViewProjection * vec4(vec3(u_Model * totalPosition), 1.0);
		Output.Normal = mat3(u_ModelInverse) * totalNormal;
	}
	else
	{
        gl_Position = u_ViewProjection * vec4(vec3(u_Model * vec4(position, 1.0)), 1.0);
		Output.Normal = mat3(u_ModelInverse) * a_Normal;
	}
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out int o_EntityID;
layout(location = 2) out vec4 o_Normal;
layout(location = 3) out vec4 o_Emissive;
layout(location = 4) out vec4 o_Roughness_Metallic_Specular_AO;

struct VertexOutput
{
	vec3 Normal;
};

layout (location = 0) in VertexOutput Input;

void main()
{
	o_Albedo = vec4(0.0, 0.0, 0.0, 1.0);
    o_EntityID = u_EntityID;
    o_Normal = vec4(normalize(Input.Normal), 1.0);
	o_Emissive = vec4(1.0, 1.0, 1.0, 1.0);
    o_Roughness_Metallic_Specular_AO = vec4(0.0, 0.0, 0.0, 0.0);
}