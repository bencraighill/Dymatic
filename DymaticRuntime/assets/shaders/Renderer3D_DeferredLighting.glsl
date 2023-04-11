// Deferred Lighting

#type vertex
#version 450 core
#include assets/shaders/Buffers.glslh

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
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D g_Albedo;
layout (binding = 1) uniform sampler2D g_Depth;
layout (binding = 2) uniform sampler2D g_Normal;
layout (binding = 3) uniform sampler2D g_Emissive;
layout (binding = 4) uniform sampler2D g_Roughness_Metallic_Specular_AO;

layout (set = 0, binding = 6) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 7) uniform samplerCubeArray u_ShadowMapArray;

layout (set = 0, binding = 8) uniform samplerCube u_EnvironmentMap;
layout (set = 0, binding = 9) uniform samplerCube u_IrradianceMap;
layout (set = 0, binding = 10) uniform samplerCube u_PrefilterMap;
layout (set = 0, binding = 11) uniform sampler2D u_brdfLUT;

layout (set = 0, binding = 12) uniform samplerCube u_FlowMap;

layout(set = 0, binding = 13) uniform sampler3D u_LightVoxelTexture;

//TODO:: Probably should be a buffer...
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

vec3 calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedo, float rough, float metal, float shadow, vec3 F0);
float calcDirShadow(vec4 fragPosLightSpace);
float ShadowCalculation(vec3 position, vec3 normal);
float PointShadowCalculation(vec3 position, vec3 normal, vec3 lightPos, float lightRange, int shadowIndex);
vec3 calcPointLight(uint index, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float rough, float metal, vec3 F0,  float viewDistance);
//float calcPointLightShadows(samplerCube depthMap, vec3 fragPos, float viewDistance);
float LinearDepth(float depthSample);

vec3 WorldPosFromDepth(float depth);

//PBR Functions
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float distributionGGX(vec3 N, vec3 H, float rough);
float geometrySchlickGGX(float nDotV, float rough);
float geometrySmith(float nDotV, float nDotL, float rough);

// Volumetric Fog
const int NUM_VOLUMETRIC_SAMPLES = 500;
const int NUM_VOLUMETRIC_BOUNCES = 1;

// Fog density (controls how thick the fog is)
const float uFogDensity = 10.0;

float ComputeScattering(float lightDotView, float scatteringDistribution);
bool IsInBounds(vec3 point, vec3 min, vec3 max);

void main()
{
	float depth = texture(g_Depth, v_TexCoord).r;
    float linearDepth = LinearDepth(depth);
	vec3 position = WorldPosFromDepth(depth);

	vec3 albedo = texture(g_Albedo, v_TexCoord).xyz;
	vec3 normal = texture(g_Normal, v_TexCoord).xyz;
    vec3 emissive = vec3(texture(g_Emissive, v_TexCoord).xyz);
	float roughness = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).x;
	float metallic = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).y;
	vec3 specular = vec3(texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).z);
	float ao = texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).w;

    if (u_VisualizationMode == VISUALIZATION_MODE_LIGHTING_ONLY)
    {
        albedo = vec3(1.0);
        roughness = 0.5;
        metallic = 0.0;
        specular = vec3(0.5);
    }

    //Components common to all light types
    vec3 viewDir = normalize(vec3(u_ViewPosition) - position);
    vec3 R = reflect(-viewDir, normal);

    //Correcting zero incidence reflection
    vec3 F0   = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    //Locating which cluster you are a part of
    uint zTile     = uint(max(log2(linearDepth) * u_Scale + u_Bias, 0.0));
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

    // Calculate Fog Lighting
    //{
    //    const float stepSize = 0.05;
    //    vec3 p = u_ViewPosition.xyz;
    //    for (float i = 0; i < 4096; i++)
    //    {
    //        if (length(u_ViewPosition.xyz - p) > linearDepth)
    //        {
    //            break;
    //        }
    //    
    //        vec3 gridPos = (p / vec3(VOXEL_SIZE * GRID_SIZE_X)) - vec3((0.5 * VOXEL_SIZE) / GRID_SIZE_X);
    //        
    //        if (gridPos.x > 0.0 && gridPos.y > 0.0 && gridPos.z > 0.0 && gridPos.x < GRID_SIZE_X && gridPos.y < GRID_SIZE_X &&  gridPos.z < GRID_SIZE_X)
    //        {
    //            radianceOut += texture(u_LightVoxelTexture, gridPos).rgb * stepSize;
    //        }
    //    
    //        p += (-viewDir) * stepSize;
    //    }
    //
    //    //radianceOut = vec3(0.0);
    //    //for (float currentDepth = 0.0; currentDepth < 1.0; currentDepth += 0.005)
    //    //{
    //    //    radianceOut += texture(u_LightVoxelTexture, vec3(v_TexCoord, currentDepth)).rgb * 0.005 * 160.0;
    //    //}
    //    //o_Color = vec4(radianceOut, 1.0);
    //    //return;
    //
    //    //radianceOut = vec3(0.0);
    //    //for (float currentDepth = 0.0; currentDepth < 1.0; currentDepth+=0.005)
    //    //{
    //    //    radianceOut += texture(u_LightVoxelTexture, vec3(v_TexCoord, currentDepth)).rgb * 0.005 * (u_ZFar - u_ZNear);
    //    //}
    //    //o_Color = vec4(radianceOut, 1.0);
    //    //return;
    //
    //    //o_Color = vec4(0.0, 0.0, 0.0, 1.0);
    //    //o_Color = vec4(texture(u_LightVoxelTexture, vec3(v_TexCoord, log2(linearDepth))).rgb, 1.0);
    //    //return;
    //}

    // Directional Lighting Volumetrics
    if (u_UsingDirectionalLight == 1)
    {
        vec3 rayVector = position.xyz - u_ViewPosition.xyz;
        float rayLength = length(rayVector);
        vec3 rayDirection = rayVector / rayLength;

        float stepLength = 0.025;

        vec3 currentPosition = u_ViewPosition.xyz;
        vec3 accumFog = vec3(0.0);

        while (length(u_ViewPosition.xyz - currentPosition) < linearDepth)
        {            
            bool inside = false;
            float scatteringDistribution = 0.0;

            for (uint i = 0; i < u_VolumeCount; i++)
            {
                if (IsInBounds(currentPosition, u_Volumes[i].min.xyz, u_Volumes[i].max.xyz))
                {
                    inside = true;

                    if (u_Volumes[i].blend == 0)
                    {
                        // Set Mode
                        scatteringDistribution = u_Volumes[i].scatteringDistribution;
                    }
                    else
                    {
                        // Additive Mode
                        scatteringDistribution += u_Volumes[i].scatteringDistribution;
                    }
                }
            }

            if (inside)
            {
                accumFog += ComputeScattering(dot(rayDirection, u_DirectionalLight.direction.xyz), scatteringDistribution).xxx * (u_DirectionalLight.color.xyz * u_DirectionalLight.color.w)
                    * (1.0 - ShadowCalculation(currentPosition, vec3(0.0, 0.0, 0.0))) * stepLength;
            }

            currentPosition += rayDirection * stepLength;
            stepLength *= 1.01;
        }

        radianceOut += accumFog;
    }

    //If IBL is enabled use environment map for rough incoming light approximation for the fragment otherwise use constant fallback.
    vec3 ambient = vec3(/*0.025*/0.0)* albedo;
    if (u_UsingSkyLight == 1) // IBL Enabled
    {
        if (linearDepth >= u_ZFar - 1)
        {
            // Background Skybox
            const float mult = 0.25;
            vec3 flowSample = ((texture(u_FlowMap, -viewDir).rgb - 0.5) * 2.0);
            vec3 skyboxA = texture(u_EnvironmentMap, (-viewDir + ((fract((u_Time * mult))) * flowSample))).rgb;
            vec3 skyboxB = texture(u_EnvironmentMap, (-viewDir + ((fract((u_Time * mult + 0.5)) ) * flowSample))).rgb;

            ambient = mix(skyboxA, skyboxB, ((asin(cos(u_Time * mult * PI * 2.0))) + (PI * 0.5)) / PI);
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
	
    radianceOut += ambient;

	// Only really applies with AO map
	radianceOut *= ao;

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

    vec3 radiance = (kD * (albedo / PI) + specular ) * radianceIn * nDotL;
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

// Note this code is used elsewhere in other shaders and all instances of it will need to be updated if modified
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

    vec3 radiance = (kD * (albedo / PI) + specular) * radianceIn * nDotL;

    if (shadowIndex != -1)
    {
        float shadow = PointShadowCalculation(fragPos, normal, position, radius, shadowIndex);
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

// This is used externally and will need to be updated if changed. TODO: move this to a glsl header.
float PointShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightPos, float lightRange, int shadowIndex)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = -max(0.0001 * (1.0 - dot(normal, fragToLight)), 0.0001);
    int samples = 20;
    float viewDistance = length(vec3(u_ViewPosition) - fragPos);
    float diskRadius = (1.0 + (viewDistance / (/*Depth Far Plane*/u_ZFar))) / u_ZFar;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(u_ShadowMapArray, vec4(vec3(fragToLight + gridSamplingDisk[i] * diskRadius), shadowIndex)).r;
        closestDepth *= /*Depth Far Plane*/lightRange;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    shadow = clamp(shadow, 0.0, 1.0);
        
    return shadow;
}

float LinearDepth(float depthSample)
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
    denom = 1 / (PI * denom * denom);

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

// Mie scaterring approximated with Henyey-Greenstein phase function.
float ComputeScattering(float lightDotView, float scatteringDistribution)
{
    float result = 1.0 - scatteringDistribution * scatteringDistribution;
    result /= (4.0 * PI * pow(1.0f + scatteringDistribution * scatteringDistribution - (2.0 * scatteringDistribution) * lightDotView, 1.5));
    return result;
}

bool IsInBounds(vec3 point, vec3 min, vec3 max)
{
    return point.x > min.x && point.y > min.y && point.z > min.z && point.x < max.x && point.y < max.y && point.z < max.z;
}