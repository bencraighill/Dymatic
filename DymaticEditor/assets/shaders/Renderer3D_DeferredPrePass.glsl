// Deferred Pre-Pass Shader

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

// Animation Data
#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

struct DirectionalLight
{
	vec4 direction;

	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct PointLight
{
	vec4 position; // w = radius

	vec4 ambient; // w = constant
	vec4 diffuse; // w = linear
	vec4 specular; // w = quadratic
};

struct SpotLight {
    vec4 position; // w = cutOff
    vec4 direction; // w = outerCutOff
  
    vec4 ambient; // w = constant
    vec4 diffuse; // w = linear
    vec4 specular; // w = quadratic
};

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
};

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
	float u_Normal;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRougnessMap;
	float u_UsingAlphaMap;
	
	int  u_AlphaBlendMode;
};

struct VertexOutput
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
	vec4 FragPosLightSpace;

    vec3 TangentViewPos;
	vec3 TangentFragPos;
    mat3 TBN;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	if (u_Animated)
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
	Output.FragPosLightSpace = lightSpaceMatrix * vec4(Output.Position, 1.0);

	if (u_UsingNormalMap == 1.0)
	{
		mat3 normalMatrix = transpose(mat3(u_ModelInverse));
   		vec3 T = normalize(normalMatrix * a_Tangent);
   		vec3 N = normalize(normalMatrix * a_Normal);
   		T = normalize(T - dot(T, N) * N);
   		vec3 B = cross(N, T);
		
   		mat3 TBN = transpose(mat3(T, B, N));    
   		Output.TangentViewPos = TBN * vec3(u_ViewPosition);
   		Output.TangentFragPos = TBN * Output.Position;
   		Output.TBN = TBN;
	}

	gl_Position = u_ViewProjection * vec4(Output.Position, 1.0);
}

#type fragment
#version 450 core

// Output to textures
layout(location = 1) out int o_EntityID;
layout(location = 2) out vec4 o_Position;
layout(location = 3) out vec4 o_Albedo;
layout(location = 4) out vec4 o_Normal;
layout(location = 5) out vec4 o_Roughness_Alpha_Specular;

// Animation Data
#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
};

layout(std140, binding = 2) uniform Object
{
	mat4 u_Model;
	mat4 u_ModelInverse;
	mat4 lightSpaceMatrix;
	mat4 u_FinalBonesMatrices[MAX_BONES];
	int u_EntityID;
	bool u_Animated;
};

struct VertexOutput
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
	vec4 FragPosLightSpace;
	
    vec3 TangentViewPos;
	vec3 TangentFragPos;
    mat3 TBN;
};

// Material bindings

layout(std140, binding = 3) uniform Material
{
	vec4 u_Albedo;
	vec4 u_Specular;
	float u_Metalness;
	float u_Shininess;
	float u_Rougness;
	float u_Alpha;
	float u_Normal;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRougnessMap;
	float u_UsingAlphaMap;

	int  u_AlphaBlendMode;
};

layout (set = 0, binding = 0) uniform sampler2D u_AlbedoMap;
layout (set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout (set = 0, binding = 2) uniform sampler2D u_SpecularMap;
layout (set = 0, binding = 3) uniform sampler2D u_MetalnessMap;
layout (set = 0, binding = 4) uniform sampler2D u_RougnessMap;
layout (set = 0, binding = 5) uniform sampler2D u_AlphaMap;

layout (location = 0) in VertexOutput Input;

void main()
{
	vec3 albedo =  vec3(u_UsingAlbedoMap == 1.0 ? (texture(u_AlbedoMap, Input.TexCoord) * u_Albedo) : u_Albedo);
	float specular = vec3(u_UsingSpecularMap == 1.0 ? (texture(u_SpecularMap, Input.TexCoord) * u_Specular) : u_Specular).r;
	float rougness = (u_UsingRougnessMap == 1.0 ? (texture(u_RougnessMap, Input.TexCoord).r * u_Rougness) : u_Rougness);
	float alpha = (u_UsingAlphaMap == 1.0 ? (texture(u_AlphaMap, Input.TexCoord).r * u_Alpha) : u_Alpha);

	if (u_AlphaBlendMode == 1 && alpha < 0.5)
		discard;
    alpha = u_AlphaBlendMode == 2 ? alpha : 1.0;
	
	// Properties
	vec3 normal;
	if (u_UsingNormalMap == 1.0)
	{
		normal = texture(u_NormalMap, Input.TexCoord).rgb * u_Normal;
		normal = normalize(normal);
		normal = normalize(normal * 2.0 - 1.0);
	}
	else
	{
		normal = normalize(Input.Normal);
	}

    o_Position = vec4(Input.Position, 1.0);
    o_Albedo = vec4(albedo, 1.0);
    o_Normal = vec4(normal, 1.0);
    o_Roughness_Alpha_Specular = vec4(rougness, alpha, specular, 1.0);
    o_EntityID = u_EntityID;
}