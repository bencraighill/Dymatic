// Basic Texture Shader

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

// Lights
#define NUM_POINT_LIGHTS 32
#define NUM_SPOT_LIGHTS 32

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

layout(std140, binding = 1) uniform Lighting
{
	DirectionalLight u_DirectionalLight;
	PointLight u_PointLights[NUM_POINT_LIGHTS];
	SpotLight u_SpotLights[NUM_SPOT_LIGHTS];
	int u_NumPointLights;
	int u_NumSpotLights;
	bool u_UsingDirectionalLight;
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

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

// Animation Data
#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

// Lights
#define NUM_POINT_LIGHTS 32
#define NUM_SPOT_LIGHTS 32

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

layout(std140, binding = 1) uniform Lighting
{
	DirectionalLight u_DirectionalLight;
	PointLight u_PointLights[NUM_POINT_LIGHTS];
	SpotLight u_SpotLights[NUM_SPOT_LIGHTS];
	int u_NumPointLights;
	int u_NumSpotLights;
	bool u_UsingDirectionalLight;
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

layout (set = 0, binding = 6) uniform sampler2D u_ShadowMap;
layout (set = 0, binding = 7) uniform samplerCube u_EnvironmentMap;


layout (location = 0) in VertexOutput Input;

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedoValue, vec3 specularValue, float shadow)
{
	vec3 lightDir = normalize(-vec3(light.direction));
	vec3 halfwayDir = normalize(lightDir + viewDir);

	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// specular phong
	//vec3 reflectDir = reflect(-lightDir, normal);
	//float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);

	// specular blinn phong
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, halfwayDir), 0.0), u_Shininess);
	spec = diff !=0 ? spec : 0.0;

	// result
	vec3 ambient = vec3(light.ambient) * albedoValue;
	vec3 diffuse = vec3(light.diffuse) * diff * albedoValue;
	vec3 specular = vec3(light.specular) * spec * specularValue;
	return (ambient + diffuse + specular) * (1.0 - (shadow * 0.5));
	//return ((ambient) + (diffuse + specular)) * (1.0 - (shadow * 0.5));
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedoValue, vec3 specularValue, float shadow)
{
	vec3 lightDir;
	if (u_UsingNormalMap == 1.0)
	{
		lightDir = normalize(Input.TBN * vec3(light.position) - Input.TangentFragPos);
	}
	else
	{
		lightDir = normalize(vec3(light.position) - fragPos);
	}

	vec3 halfwayDir = normalize(lightDir + viewDir);

	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// specular phong
	//vec3 reflectDir = reflect(-lightDir, normal);
	//float spec = pow(max(dot(viewDir, lightDir), 0.0), u_Shininess);

	// specular blinn phong
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, halfwayDir), 0.0), u_Shininess);
	spec = diff !=0 ? spec : 0.0;

	// attenuation
	float distance = length(vec3(light.position) - fragPos);
	float attenuation = light.position.w /*1.0*/ / (light.ambient.w + light.diffuse.w * distance + light.specular.w * (distance * distance));

	//result
	vec3 ambient  = vec3(light.ambient) * albedoValue;
	vec3 diffuse  = vec3(light.diffuse) * diff * albedoValue;
	vec3 specular = vec3(light.specular) * spec * specularValue;
	ambient  *= attenuation;
	diffuse  *= attenuation;
	specular *= attenuation;
	return (ambient + diffuse + specular) * (1.0 - (shadow * 0.5));
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedoValue, vec3 specularValue, float shadow)
{
	vec3 lightDir;
	if (u_UsingNormalMap == 1.0)
	{
		lightDir = normalize(Input.TBN * vec3(light.position) - Input.TangentFragPos);
	}
	else
	{
		lightDir = normalize(vec3(light.position) - fragPos);
	}
    
	vec3 halfwayDir = normalize(lightDir + viewDir);

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // specular phong
    //vec3 reflectDir = reflect(-lightDir, normal);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);

	// specular blinn phong
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, halfwayDir), 0.0), u_Shininess);
	spec = diff !=0 ? spec : 0.0;

    // attenuation
    float distance = length(vec3(light.position) - fragPos);
    float attenuation = 1.0 / (light.ambient.w + light.diffuse.w * distance + light.specular.w * (distance * distance));   

    // intensity
    float theta = dot(lightDir, normalize(-vec3(light.direction))); 
    float epsilon = light.position.w - light.direction.w;
    float intensity = clamp((theta - light.direction.w) / epsilon, 0.0, 1.0);

    // result
    vec3 ambient  = vec3(light.ambient) * albedoValue;
	vec3 diffuse  = vec3(light.diffuse) * diff * albedoValue;
	vec3 specular = vec3(light.specular) * spec * specularValue;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular) * (1.0 - shadow);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r; 

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Input.Normal);
    vec3 lightDir = normalize(/*light position*/vec3(1.0, 1.0, 1.0) - Input.Position);
    float bias = 0.0;// max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	// PCF: percentage-closer filtering
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
	const int halfkernelWidth = 3;
	for(int x = -halfkernelWidth; x <= halfkernelWidth; ++x)
	{
		for(int y = -halfkernelWidth; y <= halfkernelWidth; ++y)
		{
			float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= ((halfkernelWidth*2+1)*(halfkernelWidth*2+1));
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
	vec3 result = vec3(0.0, 0.0, 0.0);

	vec3 albedo =  vec3(u_UsingAlbedoMap == 1.0 ? (texture(u_AlbedoMap, Input.TexCoord) * u_Albedo) : u_Albedo);
	vec3 specular = vec3(u_UsingSpecularMap == 1.0 ? (texture(u_SpecularMap, Input.TexCoord) * u_Specular) : u_Specular);
	float rougness = (u_UsingRougnessMap == 1.0 ? (texture(u_RougnessMap, Input.TexCoord).r * u_Rougness) : u_Rougness);
	float alpha = (u_UsingAlphaMap == 1.0 ? (texture(u_AlphaMap, Input.TexCoord).r * u_Alpha) : u_Alpha);

	if (u_AlphaBlendMode == 1 && alpha < 0.5)
		discard;
	
	// Properties
	vec3 norm;
	vec3 normal;
	vec3 viewDir;
	if (u_UsingNormalMap == 1.0)
	{
		norm = texture(u_NormalMap, Input.TexCoord).rgb * u_Normal;
		normal = normalize(norm);
		norm = normalize(normal * 2.0 - 1.0);
		viewDir = normalize(Input.TangentViewPos - Input.TangentFragPos);
	}
	else
	{
		norm = normalize(Input.Normal);
		normal = norm;
		viewDir = normalize(vec3(u_ViewPosition) - Input.Position);
	}

	vec3 I = normalize(Input.Position - vec3(u_ViewPosition));
    vec3 R = reflect(I, normal);

	float shadow = ShadowCalculation(Input.FragPosLightSpace);

	// Directional Lighting
	if (u_UsingDirectionalLight)
		result += CalculateDirectionalLight(u_DirectionalLight, norm, viewDir, albedo, specular, shadow);

	//Point Lights
	for (int i = 0; i < u_NumPointLights; i++)
		result += CalculatePointLight(u_PointLights[i], norm, Input.Position, viewDir, albedo, specular, shadow);

	// Spot Lights
	for (int i = 0; i < u_NumSpotLights; i++)
		result += CalculateSpotLight(u_SpotLights[i], norm, Input.Position, viewDir, albedo, specular, shadow);

	vec4 FragColor = vec4(result + texture(u_EnvironmentMap, R).rgb * (1.0 - rougness), u_AlphaBlendMode == 2 ? alpha : 1.0);

	o_Color = FragColor;
	o_EntityID = u_EntityID;
}