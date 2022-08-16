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

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
    
    mat4 u_Projection;
    mat4 u_InverseProjection;
    mat4 u_View;
    mat4 u_InverseView;

	uvec4 u_TileSizes;
    uvec2 u_ScreenDimensions;
    float u_Scale;
    float u_Bias;
	float u_ZNear;
	float u_ZFar;
	float BUFF1[2];
};

layout(std140, binding = 2) uniform Object
{
	mat4 u_Model;
	mat4 u_ModelInverse;
	mat4 lightSpaceMatrix;
	mat4 u_FinalBonesMatrices[MAX_BONES];
	int u_EntityID;
	bool u_Animated;
	float BUFF2[2];
};

layout(std140, binding = 3) uniform Material
{
	vec4 u_Albedo;
	vec4 u_Specular;
	float u_Metalness;
	float u_Shininess;
	float u_Roughness;
	float u_Alpha;
	float u_Normal;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRoughnessMap;
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

	vec4 position = u_ViewProjection * vec4(Output.Position, 1.0);
	gl_Position = position;
}

#type fragment
#version 450 core

// Animation Data
#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;
layout(location = 2) out vec4 o_Normal;
layout(location = 3) out vec4 o_Metallic;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
    
    mat4 u_Projection;
    mat4 u_InverseProjection;
    mat4 u_View;
    mat4 u_InverseView;

	uvec4 u_TileSizes;
    uvec2 u_ScreenDimensions;
    float u_Scale;
    float u_Bias;
	float u_ZNear;
	float u_ZFar;
	float BUFF1[2];
};

const float LightConstant = 1.0;
const float LightLinear = 0.09;
const float LightQuadratic = 0.032;
struct DirectionalLight
{
    vec4 direction;
    vec4 color; // w = intensity
};
struct PointLight
{
    vec4 position;
    vec4 color;
    uint enabled;
    float intensity;
    float range;
	float BUFF;
};

layout(std140, binding = 1) uniform Lighting
{
	DirectionalLight u_DirectionalLight;
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
	float BUFF2[2];
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
	float u_Roughness;
	float u_Alpha;
	float u_Normal;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRoughnessMap;
	float u_UsingAlphaMap;

	int  u_AlphaBlendMode;
};

//Cluster shading structs and buffers
struct LightGrid
{
    uint offset;
    uint count;
};

layout (std430, binding = 6) buffer lightSSBO
{
    PointLight u_PointLights[];
};
layout (std430, binding = 7) buffer lightIndexSSBO
{
    uint u_GlobalLightIndexList[];
};
layout (std430, binding = 8) buffer lightGridSSBO
{
    LightGrid u_LightGrid[];
};

// Material Maps
layout (set = 0, binding = 0) uniform sampler2D u_AlbedoMap;
layout (set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout (set = 0, binding = 2) uniform sampler2D u_SpecularMap;
layout (set = 0, binding = 3) uniform sampler2D u_MetalnessMap;
layout (set = 0, binding = 4) uniform sampler2D u_RoughnessMap;
layout (set = 0, binding = 5) uniform sampler2D u_AlphaMap;

layout (set = 0, binding = 6) uniform sampler2D u_ShadowMap;
layout (set = 0, binding = 7) uniform samplerCube u_EnvironmentMap;

layout (location = 0) in VertexOutput Input;

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

//Dir light uniform
//struct DirLight{
//    vec3 direction;
//    vec3 color;
//};
//uniform DirLight dirLight;

//IBL textures to sample, all pre-computed
//uniform samplerCube irradianceMap;
//uniform samplerCube prefilterMap;
//uniform sampler2D brdfLUT;

#define M_PI 3.1415926535897932384626433832795

//TODO:: Probably should be a buffer...
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

//Function prototypes
vec3 calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedo, float rough, float metal, float shadow, vec3 F0);
float calcDirShadow(vec4 fragPosLightSpace);
vec3 calcPointLight(uint index, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float rough, float metal, vec3 F0,  float viewDistance);
//float calcPointLightShadows(samplerCube depthMap, vec3 fragPos, float viewDistance);
float linearDepth(float depthSample);

//PBR Functions
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float distributionGGX(vec3 N, vec3 H, float rough);
float geometrySchlickGGX(float nDotV, float rough);
float geometrySmith(float nDotV, float nDotL, float rough);

void main()
{

	vec3 albedo =  vec3(u_UsingAlbedoMap == 1.0 ? (texture(u_AlbedoMap, Input.TexCoord) * u_Albedo) : u_Albedo);
	vec3 specular = vec3(u_UsingSpecularMap == 1.0 ? (texture(u_SpecularMap, Input.TexCoord) * u_Specular) : u_Specular);
	float roughness = (u_UsingRoughnessMap == 1.0 ? (texture(u_RoughnessMap, Input.TexCoord).r * u_Roughness) : u_Roughness);
	float alpha = (u_UsingAlphaMap == 1.0 ? (texture(u_AlphaMap, Input.TexCoord).r * u_Alpha) : u_Alpha);

	vec3 emissive = vec3(0.0);
	int u_UsingAOMap = 0;
	float ao = (u_UsingAOMap == 1 ? (/*AO MAP GOES HERE*/0.0) : 1.0);
	float metallic = alpha;

	//if (u_AlphaBlendMode == 1 && alpha < 0.5)
	//	discard;
    
    //Normal mapping
    vec3 norm = vec3(0.0);
    if(u_UsingNormalMap == 1){
        vec3 normal = normalize(2.0 * texture(u_NormalMap, Input.TexCoord).rgb - 1.0);
        norm = normalize(Input.TBN * normal ); //going -1 to 1
    }
    else{
        //default to using the vertex normal if no normal map is used
        norm = normalize(Input.Normal);
    }

    //Components common to all light types
    vec3 viewDir = normalize(vec3(u_ViewPosition) - Input.Position);
    vec3 R = reflect(-viewDir, norm);

    //Correcting zero incidence reflection
    vec3 F0   = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    //Locating which cluster you are a part of
    uint zTile     = uint(max(log2(linearDepth(gl_FragCoord.z)) * u_Scale + u_Bias, 0.0));
    uvec3 tiles    = uvec3( uvec2( gl_FragCoord.xy / u_TileSizes[3] ), zTile);
    uint tileIndex = tiles.x +
                     u_TileSizes.x * tiles.y +
                     (u_TileSizes.x * u_TileSizes.y) * tiles.z;  

    //Solving outgoing reflectance of fragment
    vec3 radianceOut = vec3(0.0);

    // shadow calcs
    float shadow = calcDirShadow(Input.FragPosLightSpace);
    float viewDistance = length(vec3(u_ViewPosition) - vec3(Input.Position));

    //Directional light 
    radianceOut = calcDirLight(u_DirectionalLight, norm, viewDir, albedo, roughness, metallic, shadow, F0);

    // Point lights
    uint lightCount       = u_LightGrid[tileIndex].count;
    uint lightIndexOffset = u_LightGrid[tileIndex].offset;

    //Reading from the global light list and calculating the radiance contribution of each light.
    for(uint i = 0; i < lightCount; i++){
        uint lightVectorIndex = u_GlobalLightIndexList[lightIndexOffset + i];
        radianceOut += calcPointLight(lightVectorIndex, norm, Input.Position, viewDir, albedo, roughness, metallic, F0, viewDistance);
    }

    //Treating the ambient light term as the incoming indirect light affecting the fragment
    //We have two options, if IBL is not enabled for hte given object, we use a flat ambient term
    //which generally looks terrible but it's an okay fallback
    //If IBL is enabled it will use an environment map to do a very rough incoming light approximation from it
    vec3 ambient = vec3(0.025)* albedo;
//    if(IBL){
//        vec3  kS = fresnelSchlickRoughness(max(dot(norm, viewDir), 0.0), F0, roughness);
//        vec3  kD = 1.0 - kS;
//        kD *= 1.0 - metallic;
//        vec3 irradiance = texture(irradianceMap, norm).rgb;
//        vec3 diffuse    = irradiance * albedo;
//
//        const float MAX_REFLECTION_LOD = 4.0;
//        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
//        vec2 envBRDF = texture(brdfLUT, vec2(max(dot(norm, viewDir), 0.0), roughness)).rg;
//        vec3 specular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);
//        ambient = (kD * diffuse + specular);
//    }
	
	// Only really applies with AO map
	ambient *= ao;
	
    radianceOut += ambient;

    //Adding any emissive if there is an assigned map
    radianceOut += emissive;

	o_Color = vec4(radianceOut, 1.0);
	o_EntityID = u_EntityID;
	o_Normal = vec4(norm, 1.0);
	o_Metallic = vec4(metallic, specular.r, 0.0, 1.0);
}

vec3 calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedo, float rough, float metal, float shadow, vec3 F0){
    //Variables common to BRDFs
    vec3 lightDir = normalize(-vec3(light.direction));
    vec3 halfway  = normalize(lightDir + viewDir);
    float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);
    vec3 radianceIn = vec3(light.color) * light.color.w;

    //Cook-Torrance BRDF
    float NDF = distributionGGX(normal, halfway, rough);
    float G   = geometrySmith(nDotV, nDotL, rough);
    vec3  F   = fresnelSchlick(max(dot(halfway,viewDir), 0.0), F0);

    //Finding specular and diffuse component
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max (denominator, 0.0001);

    vec3 radiance = (kD * (albedo / M_PI) + specular ) * radianceIn * nDotL;
    radiance *= (1.0 - shadow);

    return radiance;
}

//Sample offsets for the pcf are the same for both dir and point shadows
float calcDirShadow(vec4 fragPosLightSpace){
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float bias = 0.0;
    int   samples = 9;
    float shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);

    for(int i = 0; i < samples; ++i){
        float pcfDepth = texture(u_ShadowMap, projCoords.xy + sampleOffsetDirections[i].xy * texelSize).r;
        shadow += projCoords.z - bias > pcfDepth ? 0.111111 : 0.0;
    }

    return shadow;
}

vec3 calcPointLight(uint index, vec3 normal, vec3 fragPos,
                    vec3 viewDir, vec3 albedo, float rough,
                    float metal, vec3 F0,  float viewDistance){
    //Point light basics
    vec3 position = u_PointLights[index].position.xyz;
    vec3 color    = 100.0 * u_PointLights[index].color.rgb * u_PointLights[index].intensity;
    float radius  = u_PointLights[index].range;

    //Stuff common to the BRDF subfunctions 
    vec3 lightDir = normalize(position - fragPos);
    vec3 halfway  = normalize(lightDir + viewDir);
    float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);

    //Attenuation calculation that is applied to all
    float distance    = length(position - fragPos);
    float attenuation = pow(clamp(1 - pow((distance / radius), 4.0), 0.0, 1.0), 2.0)/(1.0  + (distance * distance) );
    vec3 radianceIn   = color * attenuation;

    //Cook-Torrance BRDF
    float NDF = distributionGGX(normal, halfway, rough);
    float G   = geometrySmith(nDotV, nDotL, rough);
    vec3  F   = fresnelSchlick(max(dot(halfway,viewDir), 0.0), F0);

    //Finding specular and diffuse component
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specular = numerator / max(denominator, 0.0000001);
    // vec3 specular = numerator / denominator;

    vec3 radiance = (kD * (albedo / M_PI) + specular ) * radianceIn * nDotL;

    //shadow stuff
    //vec3 fragToLight = fragPos - position;
    //float shadow = calcPointLightShadows(depthMaps[index], fragToLight, viewDistance);
    //
    //radiance *= (1.0 - shadow);

    return radiance;
}

//sample amount is small but this was killing perf
//This will probably be re-written as soon as the shadow mapping update comes in
//float calcPointLightShadows(samplerCube depthMap, vec3 fragToLight, float viewDistance){
//    float shadow      = 0.0;
//    float bias        = 0.0;
//    int   samples     = 8;
//    float fraction    = 1.0/float(samples);
//    float diskRadius  = (1.0 + (viewDistance / far_plane)) / 25.0;
//    float currentDepth = (length(fragToLight) - bias);
//
//    for(int i = 0; i < samples; ++i){
//        float closestDepth = texture(depthMap, fragToLight + sampleOffsetDirections[i], diskRadius).r;
//        closestDepth *= far_plane;
//        if(currentDepth > closestDepth){
//            shadow += fraction;
//        }
//    }
//    return shadow;
//}

float linearDepth(float depthSample){
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
}

// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0){
    float val = 1.0 - cosTheta;
    return F0 + (1.0 - F0) * (val*val*val*val*val); //Faster than pow
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    float val = 1.0 - cosTheta;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (val*val*val*val*val); //Faster than pow
}

float distributionGGX(vec3 N, vec3 H, float rough){
    float a  = rough * rough;
    float a2 = a * a;

    float nDotH  = max(dot(N, H), 0.0);
    float nDotH2 = nDotH * nDotH;

    float num = a2; 
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    denom = 1 / (M_PI * denom * denom);

    return num * denom;
}

float geometrySchlickGGX(float nDotV, float rough){
    float r = (rough + 1.0);
    float k = r*r / 8.0;

    float num = nDotV;
    float denom = 1 / (nDotV * (1.0 - k) + k);

    return num * denom;
}

float geometrySmith(float nDotV, float nDotL, float rough){
    float ggx2  = geometrySchlickGGX(nDotV, rough);
    float ggx1  = geometrySchlickGGX(nDotL, rough);

    return ggx1 * ggx2;
}