// Deferred Pre-Pass Shader

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

struct VertexOutput
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;

    vec3 TangentViewPos;
	vec3 TangentFragPos;
    mat3 TBN;
};

layout (location = 0) out VertexOutput Output;

void main()
{
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
    	        totalPosition = vec4(a_Position, 1.0f);
				totalNormal = a_Normal;
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(a_Position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];

			// Normal
    	    vec3 localNormal = mat3(u_FinalBonesMatrices[a_BoneIDs[i]]) * a_Normal;
    	    totalNormal += localNormal * a_Weights[i];
    	}

		Output.Position = vec3(u_Model * totalPosition);
		Output.Normal = mat3(u_ModelInverse) * totalNormal;
	}
	else
	{
		Output.Position = vec3(u_Model * vec4(a_Position, 1.0));
		Output.Normal = mat3(u_ModelInverse) * a_Normal;
	}

	Output.TexCoord = a_TexCoord;

	if (u_UsingNormalMap == 1.0)
	{
		// Should be precomputed
		mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    	vec3 T = normalize(normalMatrix * a_Tangent);
    	vec3 N = normalize(normalMatrix * a_Normal);
    	T = normalize(T - dot(T, N) * N);
    	vec3 B = cross(N, T);
		
   		Output.TBN = mat3(T, B, N);

   		Output.TangentViewPos = Output.TBN * vec3(u_ViewPosition);
   		Output.TangentFragPos = Output.TBN * Output.Position;
	}

	gl_Position = u_ViewProjection * vec4(Output.Position, 1.0);
}

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out int o_EntityID;
layout(location = 2) out vec4 o_Normal;
layout(location = 3) out vec4 o_Emissive;
layout(location = 4) out vec4 o_Roughness_Metallic_Specular_AO;

struct VertexOutput
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
	
    vec3 TangentViewPos;
	vec3 TangentFragPos;
    mat3 TBN;
};

// Material Maps
layout (set = 0, binding = 0) uniform sampler2D u_AlbedoMap;
layout (set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout (set = 0, binding = 2) uniform sampler2D u_EmissiveMap;
layout (set = 0, binding = 3) uniform sampler2D u_SpecularMap;
layout (set = 0, binding = 4) uniform sampler2D u_MetalnessMap;
layout (set = 0, binding = 5) uniform sampler2D u_RoughnessMap;
layout (set = 0, binding = 6) uniform sampler2D u_AlphaMap;
layout (set = 0, binding = 7) uniform sampler2D u_AmbientOcclusionMap;

layout (location = 0) in VertexOutput Input;

void main()
{
	vec3 albedo =  u_UsingAlbedoMap == 1.0 ? (texture(u_AlbedoMap, Input.TexCoord).xyz * u_Albedo.xyz) : u_Albedo.xyz;
	vec3 emissive =  (u_UsingEmissiveMap == 1.0 ? (texture(u_EmissiveMap, Input.TexCoord).xyz * u_Emissive.xyz) : u_Emissive.xyz) * u_Emissive.w;
	vec3 specular = u_UsingSpecularMap == 1.0 ? (texture(u_SpecularMap, Input.TexCoord).xyz * u_Specular.xyz) : u_Specular.xyz;
	float roughness = u_UsingRoughnessMap == 1.0 ? (texture(u_RoughnessMap, Input.TexCoord).r * u_Roughness) : u_Roughness;
	float metallic = u_UsingMetalnessMap == 1.0 ? (texture(u_MetalnessMap, Input.TexCoord).r * u_Metalness) : u_Metalness;
	float ao = u_UsingAmbientOcclusionMap == 1.0 ? (texture(u_AmbientOcclusionMap, Input.TexCoord).r * u_AmbientOcclusion) : u_AmbientOcclusion;
    
    //Normal mapping
    vec3 normal = vec3(0.0);
    if(u_UsingNormalMap == 1)
	{
        normal = (2.0 * texture(u_NormalMap, Input.TexCoord).rgb - 1.0);
        normal = normalize(Input.TBN * normal); //going -1 to 1
    }
    else
	{
        //default to using the vertex normal if no normal map is used
        normal = normalize(Input.Normal);
    }

	o_Albedo = vec4(albedo, 1.0);
    o_EntityID = u_EntityID;
    o_Normal = vec4(normal, 1.0);
	o_Emissive = vec4(emissive, 1.0);
    o_Roughness_Metallic_Specular_AO = vec4(roughness, metallic, specular.x, ao);
}