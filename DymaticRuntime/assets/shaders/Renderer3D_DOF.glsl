// Depth of Field (DOF) Post Processing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_SceneTexture;
layout (binding = 1) uniform sampler2D u_DepthTexture;

const float GOLDEN_ANGLE = 2.39996323;
const float RAD_SCALE = 0.5; // Smaller = nicer blur, larger = faster

float getBlurSize(float depth)
{
	if (depth >= u_FocusNearStart && depth < u_FocusNearEnd)
	{
		return ((u_FocusNearEnd - depth)/(u_FocusNearEnd - u_FocusNearStart)) * u_FocusScale;
	}
	else if (depth >= u_FocusNearEnd && depth < u_FocusFarStart)
	{
		return 0.0;
	}
	else if (depth >= u_FocusFarStart && depth < u_FocusFarEnd)
	{
		return (1.0 - (u_FocusFarEnd - depth) / (u_FocusFarEnd - u_FocusFarStart)) * u_FocusScale;
	}
	else
	{
		return u_FocusScale;
	}
}

float LinearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
}

void main()
{
	float centerDepth = LinearDepth(texture(u_DepthTexture, v_TexCoord).r);
 	float centerSize = getBlurSize(centerDepth);
 	vec3 color = texture(u_SceneTexture, v_TexCoord).rgb;
 	float tot = 1.0;

 	float radius = RAD_SCALE;
 	for (float ang = 0.0; radius < u_FocusScale; ang += GOLDEN_ANGLE)
 	{
 	 	vec2 tc = v_TexCoord + vec2(cos(ang), sin(ang)) * u_PixelSize * radius;

 	 	vec3 sampleColor = texture(u_SceneTexture, tc).rgb;
 	 	float sampleDepth = LinearDepth(texture(u_DepthTexture, tc).r);
 	 	float sampleSize = getBlurSize(sampleDepth);
 	 	if (sampleDepth > centerDepth)
 	 	 	sampleSize = clamp(sampleSize, 0.0, centerSize*2.0);

 	 	float m = smoothstep(radius-0.5, radius+0.5, sampleSize);
 	 	color += mix(color/tot, sampleColor, m);
 	 	tot += 1.0;
 	 	radius += RAD_SCALE/radius;
 	}
 	o_Color = vec4(color /= tot, 1.0);
}