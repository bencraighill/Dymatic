// Pre Depth Shader

#type vertex
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

layout (location = 0) out vec2 o_TexCoord;

void main()
{
	vec3 position;

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
    	        totalPosition = vec4(a_Position, 1.0f);
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(a_Position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];
    	}

		position = vec3(u_Model * totalPosition);
	}
	else
	{
		position = vec3(u_Model * vec4(a_Position, 1.0));
	}

	o_TexCoord = a_TexCoord;
	gl_Position = u_ViewProjection * vec4(position, 1.0);
}

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (set = 0, binding = 6) uniform sampler2D u_AlphaMap;

float noise(vec2 co) { return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453); }

layout (location = 0) in vec2 v_TexCoord;

void main()
{
	if (u_AlphaBlendMode == 3)
		if ((u_UsingAlphaMap == 1.0 ? (texture(u_AlphaMap, v_TexCoord).r * u_Alpha) : u_Alpha) <= noise(v_TexCoord))
			discard;

    if (u_AlphaBlendMode == 2)
        discard;

    if (u_AlphaBlendMode == 1)
    	if ((u_UsingAlphaMap == 1.0 ? (texture(u_AlphaMap, v_TexCoord).r * u_Alpha) : u_Alpha) < 0.5)
    		discard;
}