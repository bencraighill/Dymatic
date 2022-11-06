// DOF Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_SceneTexture;
layout (binding = 1) uniform sampler2D u_Depth;

const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 20.0;
const float RAD_SCALE = 0.5; // Smaller = nicer blur, larger = faster

float getBlurSize(float depth, float focusPoint, float focusScale)
{
 float coc = clamp((1.0 / focusPoint - 1.0 / depth)*focusScale, -1.0, 1.0);
 return abs(coc) * MAX_BLUR_SIZE;
}

layout(std140, binding = 4) uniform PostProcessing
{
    vec4 u_SSAOSamples[64];
    float u_Exposure;
    int u_BlurHorizontal;
    float u_Time;
};

void main()
{
	const vec2 PixelSize = 1.0 / textureSize(u_SceneTexture, 0);

   	float u_Far = 1000.0;
  	float focusPoint = u_Time;
   	float focusScale = 20.0;

	float centerDepth = texture(u_Depth, v_TexCoord).r * u_Far;
 	float centerSize = getBlurSize(centerDepth, focusPoint, focusScale);
 	vec3 color = texture(u_SceneTexture, v_TexCoord).rgb;
 	float tot = 1.0;

 	float radius = RAD_SCALE;
 	for (float ang = 0.0; radius<MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
 	{
 	 	vec2 tc = v_TexCoord + vec2(cos(ang), sin(ang)) * PixelSize * radius;

 	 	vec3 sampleColor = texture(u_SceneTexture, tc).rgb;
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