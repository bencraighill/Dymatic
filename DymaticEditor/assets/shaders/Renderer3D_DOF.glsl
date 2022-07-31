// DOF Shader

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec2 o_TexCoord;

const vec2 madd = vec2(0.5,0.5);

void main() 
{
   o_TexCoord = a_Position.xy*madd+madd; // scale vertex attribute to [0-1] range
   gl_Position = vec4(a_Position.xy,0.0,1.0);
}

#type fragment
#version 450 core

layout(std140, binding = 3) uniform Material
{
	vec4 u_Albedo;
	float u_Normal;
	vec4 u_Specular;
	float u_Metalness;
	float u_Shininess;
	float u_Rougness;
	float u_Alpha;

	float u_UsingAlbedoMap;
	float u_UsingNormalMap;
	float u_UsingSpecularMap;
	float u_UsingMetalnessMap;
	float u_UsingRougnessMap;
	float u_UsingAlphaMap;
	
	int  u_AlphaBlendMode;
};

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_Texture;
layout (binding = 1) uniform sampler2D u_Depth;
const vec2 u_PixelSize = vec2(1.0 / 1920, 1.0 / 1080);

const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 20.0;
const float RAD_SCALE = 0.5; // Smaller = nicer blur, larger = faster

float getBlurSize(float depth, float focusPoint, float focusScale)
{
 float coc = clamp((1.0 / focusPoint - 1.0 / depth)*focusScale, -1.0, 1.0);
 return abs(coc) * MAX_BLUR_SIZE;
}

void main()
{
   float u_Far = 1000.0;
   float focusPoint = 150.0;
   float focusScale = 20.0;

 float centerDepth = texture(u_Depth, v_TexCoord).r * u_Far;
 float centerSize = getBlurSize(centerDepth, focusPoint, focusScale);
 vec3 color = texture(u_Texture, v_TexCoord).rgb;
 float tot = 1.0;

 float radius = RAD_SCALE;
 for (float ang = 0.0; radius<MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
 {
  vec2 tc = v_TexCoord + vec2(cos(ang), sin(ang)) * u_PixelSize * radius;

  vec3 sampleColor = texture(u_Texture, tc).rgb;
  float sampleDepth = texture(u_Depth, tc).r * u_Far;
  float sampleSize = getBlurSize(sampleDepth, focusPoint, focusScale);
  if (sampleDepth > centerDepth)
   sampleSize = clamp(sampleSize, 0.0, centerSize*2.0);

  float m = smoothstep(radius-0.5, radius+0.5, sampleSize);
  color += mix(color/tot, sampleColor, m);
  tot += 1.0;
  radius += RAD_SCALE/radius;
 }
 o_Color = vec4(color /= tot, 1.0);
}