// Basic Text Shader

#type vertex
#version 450 core
#include Include/Buffers.glslh

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in int a_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) out flat float v_TexIndex;
layout (location = 4) out flat int v_EntityID;

void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat float v_TexIndex;
layout (location = 4) in flat int v_EntityID;

layout (binding = 0) uniform sampler2D u_Textures[32];

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

// TODO: Should be precomputed for better performance
float ScreenPixelRange(vec2 texSize, vec2 fwidth_texCoord)
{
	// Value must be consistant with pxRange of atlas
    vec2 unitRange = vec2(2.0)/texSize;
    vec2 screenTexSize = vec2(1.0)/fwidth_texCoord;
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main()
{
	vec4 texColor = Input.Color;

	switch(int(v_TexIndex))
	{
		case  0: texColor = texture(u_Textures[ 0], Input.TexCoord); break;
		case  1: texColor = texture(u_Textures[ 1], Input.TexCoord); break;
		case  2: texColor = texture(u_Textures[ 2], Input.TexCoord); break;
		case  3: texColor = texture(u_Textures[ 3], Input.TexCoord); break;
		case  4: texColor = texture(u_Textures[ 4], Input.TexCoord); break;
		case  5: texColor = texture(u_Textures[ 5], Input.TexCoord); break;
		case  6: texColor = texture(u_Textures[ 6], Input.TexCoord); break;
		case  7: texColor = texture(u_Textures[ 7], Input.TexCoord); break;
		case  8: texColor = texture(u_Textures[ 8], Input.TexCoord); break;
		case  9: texColor = texture(u_Textures[ 9], Input.TexCoord); break;
		case 10: texColor = texture(u_Textures[10], Input.TexCoord); break;
		case 11: texColor = texture(u_Textures[11], Input.TexCoord); break;
		case 12: texColor = texture(u_Textures[12], Input.TexCoord); break;
		case 13: texColor = texture(u_Textures[13], Input.TexCoord); break;
		case 14: texColor = texture(u_Textures[14], Input.TexCoord); break;
		case 15: texColor = texture(u_Textures[15], Input.TexCoord); break;
		case 16: texColor = texture(u_Textures[16], Input.TexCoord); break;
		case 17: texColor = texture(u_Textures[17], Input.TexCoord); break;
		case 18: texColor = texture(u_Textures[18], Input.TexCoord); break;
		case 19: texColor = texture(u_Textures[19], Input.TexCoord); break;
		case 20: texColor = texture(u_Textures[20], Input.TexCoord); break;
		case 21: texColor = texture(u_Textures[21], Input.TexCoord); break;
		case 22: texColor = texture(u_Textures[22], Input.TexCoord); break;
		case 23: texColor = texture(u_Textures[23], Input.TexCoord); break;
		case 24: texColor = texture(u_Textures[24], Input.TexCoord); break;
		case 25: texColor = texture(u_Textures[25], Input.TexCoord); break;
		case 26: texColor = texture(u_Textures[26], Input.TexCoord); break;
		case 27: texColor = texture(u_Textures[27], Input.TexCoord); break;
		case 28: texColor = texture(u_Textures[28], Input.TexCoord); break;
		case 29: texColor = texture(u_Textures[29], Input.TexCoord); break;
		case 30: texColor = texture(u_Textures[30], Input.TexCoord); break;
		case 31: texColor = texture(u_Textures[31], Input.TexCoord); break;
	}

	vec2 texSize;

	switch(int(v_TexIndex))
	{
		case  0: texSize = vec2(textureSize(u_Textures[ 0], 0)); break;
		case  1: texSize = vec2(textureSize(u_Textures[ 1], 0)); break;
		case  2: texSize = vec2(textureSize(u_Textures[ 2], 0)); break;
		case  3: texSize = vec2(textureSize(u_Textures[ 3], 0)); break;
		case  4: texSize = vec2(textureSize(u_Textures[ 4], 0)); break;
		case  5: texSize = vec2(textureSize(u_Textures[ 5], 0)); break;
		case  6: texSize = vec2(textureSize(u_Textures[ 6], 0)); break;
		case  7: texSize = vec2(textureSize(u_Textures[ 7], 0)); break;
		case  8: texSize = vec2(textureSize(u_Textures[ 8], 0)); break;
		case  9: texSize = vec2(textureSize(u_Textures[ 9], 0)); break;
		case 10: texSize = vec2(textureSize(u_Textures[10], 0)); break;
		case 11: texSize = vec2(textureSize(u_Textures[11], 0)); break;
		case 12: texSize = vec2(textureSize(u_Textures[12], 0)); break;
		case 13: texSize = vec2(textureSize(u_Textures[13], 0)); break;
		case 14: texSize = vec2(textureSize(u_Textures[14], 0)); break;
		case 15: texSize = vec2(textureSize(u_Textures[15], 0)); break;
		case 16: texSize = vec2(textureSize(u_Textures[16], 0)); break;
		case 17: texSize = vec2(textureSize(u_Textures[17], 0)); break;
		case 18: texSize = vec2(textureSize(u_Textures[18], 0)); break;
		case 19: texSize = vec2(textureSize(u_Textures[19], 0)); break;
		case 20: texSize = vec2(textureSize(u_Textures[20], 0)); break;
		case 21: texSize = vec2(textureSize(u_Textures[21], 0)); break;
		case 22: texSize = vec2(textureSize(u_Textures[22], 0)); break;
		case 23: texSize = vec2(textureSize(u_Textures[23], 0)); break;
		case 24: texSize = vec2(textureSize(u_Textures[24], 0)); break;
		case 25: texSize = vec2(textureSize(u_Textures[25], 0)); break;
		case 26: texSize = vec2(textureSize(u_Textures[26], 0)); break;
		case 27: texSize = vec2(textureSize(u_Textures[27], 0)); break;
		case 28: texSize = vec2(textureSize(u_Textures[28], 0)); break;
		case 29: texSize = vec2(textureSize(u_Textures[29], 0)); break;
		case 30: texSize = vec2(textureSize(u_Textures[30], 0)); break;
		case 31: texSize = vec2(textureSize(u_Textures[31], 0)); break;
	}

	vec2 fwidth_texCoord = fwidth(Input.TexCoord);
	vec3 msd = texColor.rgb;
    float sd = Median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPixelRange(texSize, fwidth_texCoord)*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

	if (opacity == 0.0)
		discard;

	//vec4 bgColor = vec4(0.0);
    //o_Color = mix(bgColor, Input.Color, opacity);
	//
	//if (o_Color.a == 0.0)
	//	discard;

	o_Color = vec4(Input.Color.rgb, opacity);
	
	o_EntityID = v_EntityID;
}