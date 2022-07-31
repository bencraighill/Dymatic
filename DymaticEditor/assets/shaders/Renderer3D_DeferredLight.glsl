// Deferred Lighting Shader

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

// Quad TexCoord
layout(location = 0) in vec2 a_TexCoord;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out float o_EntityID;

layout  (binding = 0) uniform sampler2D u_InputTexture; // Input from previous light pass

layout  (binding = 1) uniform sampler2D u_EntityID;
layout  (binding = 2) uniform sampler2D u_Position;
layout  (binding = 3) uniform sampler2D u_Albedo;
layout  (binding = 4) uniform sampler2D u_Normal; //Normal.z = sqrt(1.0f - Normal.x2 - Normal.y2) !Could help pack down the framebuffer data
layout  (binding = 5) uniform sampler2D u_Roughness_Alpha_Specular;

// Main Lighting Shader

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
	PointLight u_PointLight;
	SpotLight u_SpotLight;
    int u_LightType;
};

layout(std140, binding = 2) uniform Object
{
	mat4 u_Model;
	mat4 u_ModelInverse;
	mat4 lightSpaceMatrix;
};

// Material bindings
const float u_Shininess = 32.0;
layout (binding = 7) uniform samplerCube u_EnvironmentMap;
layout (binding = 8) uniform sampler2D u_ShadowMap;

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
    vec3 normal = vec3(texture(u_Normal, a_TexCoord));
    vec3 lightDir = normalize(/*light position*/vec3(1.0, 1.0, 1.0) - texture(u_Position, a_TexCoord).xyz);
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
	vec3 albedo = vec3(texture(u_Albedo, a_TexCoord));
	float rougness = texture(u_Roughness_Alpha_Specular, a_TexCoord).x;
	float alpha = texture(u_Roughness_Alpha_Specular, a_TexCoord).y;
	vec3 specular = vec3(texture(u_Roughness_Alpha_Specular, a_TexCoord).z);
    vec3 normal = texture(u_Normal, a_TexCoord).xyz;
    vec3 position = texture(u_Position, a_TexCoord).xyz;
	
	// Properties
	vec3 viewDir = normalize(vec3(u_ViewPosition) - position);

	vec3 I = normalize(position - vec3(u_ViewPosition));
    vec3 R = reflect(I, normal);

    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(position, 1.0); //!! Expensive - SHOULD BE PRECOMPUTED
	float shadow = ShadowCalculation(fragPosLightSpace);

    vec3 result = vec3(0.0, 0.0, 0.0);
    switch (u_LightType)
    {
        case 0: break;
        case 1:
            result = CalculateDirectionalLight(u_DirectionalLight, normal, viewDir, albedo, specular, shadow);
            break;
        case 2:
            result = CalculatePointLight(u_PointLight, normal, position, viewDir, albedo, specular, shadow);
            break;
        case 3:
            result = CalculateSpotLight(u_SpotLight, normal, position, viewDir, albedo, specular, shadow);
            break;
    }

	//vec4 FragColor = vec4(result + texture(u_EnvironmentMap, R).rgb * (1.0 - rougness), alpha);
    vec4 FragColor = texture(u_InputTexture, a_TexCoord) + vec4(result, alpha);

	o_Color = FragColor;
	o_EntityID = texture(u_EntityID, a_TexCoord).r;
}