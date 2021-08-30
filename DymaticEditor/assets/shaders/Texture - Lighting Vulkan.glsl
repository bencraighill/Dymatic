#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;
layout(location = 5) in int a_EntityID;
layout(location = 6) in vec3 a_Normal;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec3 Position;
	vec4 Color;
	vec2 TexCoord;
	float TexIndex;
	float TilingFactor;
	vec3 Normal;
};

layout (location = 0) out VertexOutput Output;
layout (location = 6) out flat int v_EntityID;

void main()
{
	Output.Position = a_Position;
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	Output.TexIndex = a_TexIndex;
	Output.TilingFactor = a_TilingFactor;
	//Output.Normal = vec3(1.0, 1.0, 1.0);
	Output.Normal = a_Normal;
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);

	
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out int color2;

struct VertexOutput
{
	vec3 Position;
	vec4 Color;
	vec2 TexCoord;
	float TexIndex;
	float TilingFactor;
	vec3 Normal;
};

layout (location = 0) in VertexOutput Input;
layout (location = 6) in flat int v_EntityID;

struct Material {
	vec3 ambient;
	vec3 specular;
	float shininess;
};

//uniform Material material;

struct Light {
	vec3 position;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

//uniform Light light;

layout (binding = 0) uniform sampler2D u_Textures[32];
layout(std140, binding = 0) uniform LightData
{
	vec3 viewPos;
	vec3 lightPos;
};

void main()
{
	vec4 texColor = Input.Color;
	switch(int(Input.TexIndex))
	{
		case  0: texColor *= texture(u_Textures[ 0], Input.TexCoord * Input.TilingFactor); break;
		case  1: texColor *= texture(u_Textures[ 1], Input.TexCoord * Input.TilingFactor); break;
		case  2: texColor *= texture(u_Textures[ 2], Input.TexCoord * Input.TilingFactor); break;
		case  3: texColor *= texture(u_Textures[ 3], Input.TexCoord * Input.TilingFactor); break;
		case  4: texColor *= texture(u_Textures[ 4], Input.TexCoord * Input.TilingFactor); break;
		case  5: texColor *= texture(u_Textures[ 5], Input.TexCoord * Input.TilingFactor); break;
		case  6: texColor *= texture(u_Textures[ 6], Input.TexCoord * Input.TilingFactor); break;
		case  7: texColor *= texture(u_Textures[ 7], Input.TexCoord * Input.TilingFactor); break;
		case  8: texColor *= texture(u_Textures[ 8], Input.TexCoord * Input.TilingFactor); break;
		case  9: texColor *= texture(u_Textures[ 9], Input.TexCoord * Input.TilingFactor); break;
		case 10: texColor *= texture(u_Textures[10], Input.TexCoord * Input.TilingFactor); break;
		case 11: texColor *= texture(u_Textures[11], Input.TexCoord * Input.TilingFactor); break;
		case 12: texColor *= texture(u_Textures[12], Input.TexCoord * Input.TilingFactor); break;
		case 13: texColor *= texture(u_Textures[13], Input.TexCoord * Input.TilingFactor); break;
		case 14: texColor *= texture(u_Textures[14], Input.TexCoord * Input.TilingFactor); break;
		case 15: texColor *= texture(u_Textures[15], Input.TexCoord * Input.TilingFactor); break;
		case 16: texColor *= texture(u_Textures[16], Input.TexCoord * Input.TilingFactor); break;
		case 17: texColor *= texture(u_Textures[17], Input.TexCoord * Input.TilingFactor); break;
		case 18: texColor *= texture(u_Textures[18], Input.TexCoord * Input.TilingFactor); break;
		case 19: texColor *= texture(u_Textures[19], Input.TexCoord * Input.TilingFactor); break;
		case 20: texColor *= texture(u_Textures[20], Input.TexCoord * Input.TilingFactor); break;
		case 21: texColor *= texture(u_Textures[21], Input.TexCoord * Input.TilingFactor); break;
		case 22: texColor *= texture(u_Textures[22], Input.TexCoord * Input.TilingFactor); break;
		case 23: texColor *= texture(u_Textures[23], Input.TexCoord * Input.TilingFactor); break;
		case 24: texColor *= texture(u_Textures[24], Input.TexCoord * Input.TilingFactor); break;
		case 25: texColor *= texture(u_Textures[25], Input.TexCoord * Input.TilingFactor); break;
		case 26: texColor *= texture(u_Textures[26], Input.TexCoord * Input.TilingFactor); break;
		case 27: texColor *= texture(u_Textures[27], Input.TexCoord * Input.TilingFactor); break;
		case 28: texColor *= texture(u_Textures[28], Input.TexCoord * Input.TilingFactor); break;
		case 29: texColor *= texture(u_Textures[29], Input.TexCoord * Input.TilingFactor); break;
		case 30: texColor *= texture(u_Textures[30], Input.TexCoord * Input.TilingFactor); break;
		case 31: texColor *= texture(u_Textures[31], Input.TexCoord * Input.TilingFactor); break;
	}

	Light light; light.position = vec3(0.0, 0.0, 10.0); light.ambient = vec3(0.2, 0.2, 0.2); light.diffuse = vec3(0.2, 0.2, 0.2); light.specular = vec3(0.8, 0.8, 0.8);
	Material material; material.ambient = vec3(0.2, 0.2, 0.2); material.specular = vec3(0.8, 0.8, 0.8); material.shininess = 1.0;

	vec3 objectColor = vec3(texColor.x, texColor.y, texColor.z);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	//vec3 lightPos = light.position;

	// ambient
    vec3 ambient  = light.ambient * objectColor;
  	
    // diffuse 
    vec3 norm = normalize(Input.Normal);
    vec3 lightDir = normalize(lightPos - Input.Position);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * objectColor;
	
	//Specular
	vec3 viewDir = normalize(viewPos - Input.Position);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * (spec * material.specular);	

	// Fade Out
	float distance = sqrt(pow(Input.Position.x - lightPos.x, 2) + pow(Input.Position.y - lightPos.y, 2) + pow(Input.Position.z - lightPos.z, 2));
	float FadeOut = -distance + 100.0f;
	if (FadeOut < 0.0f) { FadeOut = 0.0f; }
	FadeOut /= 100.0f;
            
	//result
    vec3 result = FadeOut * (ambient + diffuse + specular);
    color = vec4(result.x, result.y, result.z, 1.0);

	color2 = v_EntityID;
}