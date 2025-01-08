// Basic Texture Shader

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

void main()
{
	vec3 position = a_Position + BlendShapeOffset(a_GlobalIndex);

	if (u_Animated == 1)
	{
		// Animation Calculation
    	vec4 totalPosition = vec4(0.0);
    	for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    	{
    	    if(a_BoneIDs[i] == -1)
    	        continue;
    	    if(a_BoneIDs[i] >= MAX_BONES) 
    	    {
    	        totalPosition = vec4(position, 1.0f);
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];
    	}

		position = vec3(u_Model * totalPosition);
	}
	else
	{
		position = vec3(u_Model * vec4(position, 1.0));
	}

	gl_Position = u_ViewProjection * vec4(position, 1.0);
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

void main()
{
    o_Color = vec4(SELECTION_COLOR, 1.0);
    o_EntityID = u_EntityID;
}