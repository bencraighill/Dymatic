// Deferred Lighting

#type vertex
#version 450 core

const vec2 madd = vec2(0.5,0.5);

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec2 o_TexCoord;

void main()
{
   o_TexCoord = a_Position.xy*madd+madd; // scale vertex attribute to [0-1] range
   gl_Position = vec4(a_Position.xy,0.0,1.0);
}

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D g_Albedo;
layout (binding = 1) uniform sampler2D g_Depth;
layout (binding = 2) uniform sampler2D g_Normal;
layout (binding = 3) uniform sampler2D g_Roughness_Metallic_Specular;

layout (set = 0, binding = 6) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 7) uniform samplerCubeArray u_ShadowMapArray;

layout (set = 0, binding = 8) uniform samplerCube u_EnvironmentMap;
layout (set = 0, binding = 9) uniform samplerCube u_IrradianceMap;
layout (set = 0, binding = 10) uniform samplerCube u_PrefilterMap;
layout (set = 0, binding = 11) uniform sampler2D u_brdfLUT;


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
	int shadowIndex;
};

const int MaxCascadeCount = 16;
layout(std140, binding = 1) uniform Lighting
{
	DirectionalLight u_DirectionalLight;
    mat4 u_LightSpaceMatrices[MaxCascadeCount];
    vec4 u_CascadePlaneDistances[MaxCascadeCount/4]; // Represents and array of floats - use [index/4][index%4] to get.
	int u_UsingDirectionalLight;
    int u_CascadeCount;
    int u_UsingSkyLight;
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
float ShadowCalculation(vec3 position, vec3 normal);
float PointShadowCalculation(vec3 position, vec3 lightPos, int shadowIndex);
vec3 calcPointLight(uint index, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float rough, float metal, vec3 F0,  float viewDistance);
//float calcPointLightShadows(samplerCube depthMap, vec3 fragPos, float viewDistance);
float linearDepth(float depthSample);

vec3 WorldPosFromDepth(float depth);

//PBR Functions
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float distributionGGX(vec3 N, vec3 H, float rough);
float geometrySchlickGGX(float nDotV, float rough);
float geometrySmith(float nDotV, float nDotL, float rough);

void main()
{
	float depth = texture(g_Depth, v_TexCoord).r;
	vec3 position = WorldPosFromDepth(depth);

	vec3 albedo = texture(g_Albedo, v_TexCoord).xyz;
	vec3 normal = texture(g_Normal, v_TexCoord).xyz;
	float roughness = texture(g_Roughness_Metallic_Specular, v_TexCoord).x;
	float metallic = texture(g_Roughness_Metallic_Specular, v_TexCoord).y;
	vec3 specular = vec3(texture(g_Roughness_Metallic_Specular, v_TexCoord).z);

	float ao = 1.0;
	float alpha = 0.0;
	vec3 emissive = vec3(0.0);

	//if (u_AlphaBlendMode == 1 && alpha < 0.5)
	//	discard;

    //Components common to all light types
    vec3 viewDir = normalize(vec3(u_ViewPosition) - position);
    vec3 R = reflect(-viewDir, normal);

    //Correcting zero incidence reflection
    vec3 F0   = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    //Locating which cluster you are a part of
    uint zTile     = uint(max(log2(linearDepth(depth)) * u_Scale + u_Bias, 0.0));
    uvec3 tiles    = uvec3( uvec2( gl_FragCoord.xy / u_TileSizes[3] ), zTile);
    uint tileIndex = tiles.x +
                     u_TileSizes.x * tiles.y +
                     (u_TileSizes.x * u_TileSizes.y) * tiles.z;  

    //Solving outgoing reflectance of fragment
    vec3 radianceOut = vec3(0.0);

    // shadow calcs
    //vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(position, 1.0);
    //float shadow = calcDirShadow(fragPosLightSpace);
    float shadow = ShadowCalculation(position, normal);
    float viewDistance = length(vec3(u_ViewPosition) - position);

    //Directional light
    if (u_UsingDirectionalLight == 1)
        radianceOut = calcDirLight(u_DirectionalLight, normal, viewDir, albedo, roughness, metallic, shadow, F0);

    // Point lights
    uint lightCount       = u_LightGrid[tileIndex].count;
    uint lightIndexOffset = u_LightGrid[tileIndex].offset;

    //Reading from the global light list and calculating the radiance contribution of each light.
    for (uint i = 0; i < lightCount; i++)
    {
        uint lightVectorIndex = u_GlobalLightIndexList[lightIndexOffset + i];
        radianceOut += calcPointLight(lightVectorIndex, normal, position, viewDir, albedo, roughness, metallic, F0, viewDistance);
    }

    //If IBL is enabled use environment map for rough incoming light approximation for the fragment otherwise use constant fallback.
    vec3 ambient = vec3(/*0.025*/0.0)* albedo;
    if (u_UsingSkyLight == 1) // IBL Enabled
    {
        if (linearDepth(depth) >= u_ZFar - 1)
        {
            // Background Skybox
            ambient = texture(u_EnvironmentMap, -viewDir).rgb;
        }
        else
        {
            vec3  kS = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
            vec3  kD = 1.0 - kS;
            kD *= 1.0 - metallic;
            vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
            vec3 diffuse    = irradiance * albedo;

            const float MAX_REFLECTION_LOD = 4.0;
            vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
            vec2 envBRDF = texture(u_brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;
            vec3 specular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);
            ambient = (kD * diffuse + specular);
        }
    }
	
	// Only really applies with AO map
	ambient *= ao;
	
    radianceOut += ambient;

    //Adding any emissive if there is an assigned map
    radianceOut += emissive;

	o_Color = vec4(radianceOut, 1.0);
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
//float calcDirShadow(vec4 fragPosLightSpace)
//{
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//    projCoords = projCoords * 0.5 + 0.5;
//    float bias = 0.0;
//    int   samples = 9;
//    float shadow = 0.0;
//
//    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
//
//    for(int i = 0; i < samples; ++i){
//        float pcfDepth = texture(u_ShadowMap, projCoords.xy + sampleOffsetDirections[i].xy * texelSize).r;
//        shadow += projCoords.z - bias > pcfDepth ? 0.111111 : 0.0;
//    }
//
//    return shadow;
//}

float ShadowCalculation(vec3 position, vec3 normal)
{
    // select cascade layer
    vec4 fragPosViewSpace = u_View * vec4(position, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < u_CascadeCount; i++)
    {
        if (depthValue < u_CascadePlaneDistances[i/4][i%4])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = u_CascadeCount;
    }

    vec4 fragPosLightSpace = u_LightSpaceMatrices[layer] * vec4(position, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    float bias = max(0.05 * (1.0 - dot(normal, vec3(-u_DirectionalLight.direction))), 0.005);
    const float biasModifier = 0.5f;
    if (layer == u_CascadeCount)
    {
        bias *= 1 / (u_ZFar * biasModifier);
    }
    else
    {
        bias *= 1 / (u_CascadePlaneDistances[layer/4][layer%4] * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0));
    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(u_ShadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}

vec3 calcPointLight(uint index, vec3 normal, vec3 fragPos,
                    vec3 viewDir, vec3 albedo, float rough,
                    float metal, vec3 F0,  float viewDistance)
{
    //Point light basics
    vec3 position = u_PointLights[index].position.xyz;
    vec3 color    = 100.0 * u_PointLights[index].color.rgb * u_PointLights[index].intensity;
    float radius  = u_PointLights[index].range;
    int shadowIndex = u_PointLights[index].shadowIndex;

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

    vec3 radiance = (kD * (albedo / M_PI) + specular) * radianceIn * nDotL;

    if (shadowIndex != -1)
    {
        float shadow = PointShadowCalculation(fragPos, position, shadowIndex);
        radiance *= (1.0 - shadow);
    }

    return radiance;
}

//sample amount is small but this was killing perf
//This will probably be re-written as soon as the shadow mapping update comes in
//float calcPointLightShadows(samplerCube depthMap, vec3 fragToLight, float viewDistance)
//{
//    float shadow      = 0.0;
//    float bias        = 0.0;
//    int   samples     = 8;
//    float fraction    = 1.0/float(samples);
//    float diskRadius  = (1.0 + (viewDistance / /*Light Far Plane*/25.0)) / 25.0;
//    float currentDepth = (length(fragToLight) - bias);
//
//    for(int i = 0; i < samples; i++)
//    {
//        float closestDepth = texture(depthMap, fragToLight + sampleOffsetDirections[i], diskRadius).r;
//        closestDepth *= /*Light Far Plane*/25.0;
//        if(currentDepth > closestDepth)
//        {
//            shadow += fraction;
//        }
//    }
//    return shadow;
//}

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float PointShadowCalculation(vec3 fragPos, vec3 lightPos, int shadowIndex)
{
    //// get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    //// use the fragment to light vector to sample from the depth map    
    //float closestDepth = texture(u_ShadowMapArray, fragToLight).r;
    //// it is currently in linear range between [0,1], let's re-transform it back to original depth value
    //closestDepth *= u_ZFar;
    //// now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    //// test for shadows
    //float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    //float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
    //// PCF
    //float samples = 4.0;
    //float offset = 0.1;
    //for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    //{
    //    for(float y = -offset; y < offset; y += offset / (samples * 0.5))
    //    {
    //        for(float z = -offset; z < offset; z += offset / (samples * 0.5))
    //        {
    //            float closestDepth = texture(u_ShadowMapArray, fragToLight + vec3(x, y, z)).r; // use lightdir to lookup cubemap
    //            closestDepth *= u_ZFar;   // Undo mapping [0;1]
    //            if(currentDepth - bias > closestDepth)
    //                shadow += 1.0;
    //        }
    //    }
    //}
    //shadow /= (samples * samples * samples);

    float shadow = 0.0;
    float bias = -0.1;
    int samples = 20;
    float viewDistance = length(vec3(u_ViewPosition) - fragPos);
    float diskRadius = (1.0 + (viewDistance / (/*Depth Far Plane*/25.0))) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(u_ShadowMapArray, vec4(vec3(fragToLight + gridSamplingDisk[i] * diskRadius), shadowIndex)).r;
        closestDepth *= /*Depth Far Plane*/25.0;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    return shadow;
}

float linearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
}

// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    float val = 1.0 - cosTheta;
    return F0 + (1.0 - F0) * (val*val*val*val*val); //Faster than pow
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    float val = 1.0 - cosTheta;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (val*val*val*val*val); //Faster than pow
}

float distributionGGX(vec3 N, vec3 H, float rough)
{
    float a  = rough * rough;
    float a2 = a * a;

    float nDotH  = max(dot(N, H), 0.0);
    float nDotH2 = nDotH * nDotH;

    float num = a2; 
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    denom = 1 / (M_PI * denom * denom);

    return num * denom;
}

float geometrySchlickGGX(float nDotV, float rough)
{
    float r = (rough + 1.0);
    float k = r*r / 8.0;

    float num = nDotV;
    float denom = 1 / (nDotV * (1.0 - k) + k);

    return num * denom;
}

float geometrySmith(float nDotV, float nDotL, float rough)
{
    float ggx2  = geometrySchlickGGX(nDotV, rough);
    float ggx1  = geometrySchlickGGX(nDotL, rough);

    return ggx1 * ggx2;
}

vec3 WorldPosFromDepth(float depth)
{
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(v_TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_InverseProjection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = u_InverseView * viewSpacePosition;

    return worldSpacePosition.xyz;
}