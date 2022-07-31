// Shadow Depth Shader

#type vertex
#version 450 core

// Animation Data
#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_TexCoord;
layout(location = 5) in ivec4 a_BoneIDs;
layout(location = 6) in vec4 a_Weights;

layout (location = 0) out vec2 o_TexCoord;

layout(std140, binding = 2) uniform Object
{
	mat4 u_Model;
	mat4 u_ModelInverse;
	mat4 lightSpaceMatrix;
	mat4 u_FinalBonesMatrices[MAX_BONES];
	int u_EntityID;
	bool u_Animated;
};

layout(std140, binding = 3) uniform Material
{
	vec4 u_Albedo;
	vec4 u_Specular;
	float u_Metalness;
	float u_Shininess;
	float u_Rougness;
	float u_Alpha;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRougnessMap;
	float u_UsingAlphaMap;

	int  u_AlphaBlendMode;
};

void main()
{
	o_TexCoord = a_TexCoord;

	if (u_Animated)
	{
		// Animation Calculation
    	vec4 totalPosition = vec4(0.0);
    	for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    	{
    	    if(a_BoneIDs[i] == -1)
    	        continue;
    	    if(a_BoneIDs[i] >= MAX_BONES) 
    	    {
    	        totalPosition = vec4(a_Position, 1.0f);
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(a_Position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];
    	}

		gl_Position = lightSpaceMatrix * u_Model * totalPosition;
	}
	else
		gl_Position = lightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout (location = 0) in vec2 a_TexCoord;

layout(std140, binding = 3) uniform Material
{
	vec4 u_Albedo;
	vec4 u_Specular;
	float u_Metalness;
	float u_Shininess;
	float u_Rougness;
	float u_Alpha;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRougnessMap;
	float u_UsingAlphaMap;

	int  u_AlphaBlendMode;
};

layout (set = 0, binding = 5) uniform sampler2D u_AlphaMap;

void main()
{
    if (u_AlphaBlendMode == 2)
        discard;

    if (u_AlphaBlendMode == 1)
    		if ((u_UsingAlphaMap == 1.0 ? (texture(u_AlphaMap, a_TexCoord).r * u_Alpha) : u_Alpha) < 0.5)
    			discard;
}