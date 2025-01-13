// Lens Flare Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_ColorTexture;
layout (binding = 1) uniform sampler2D u_NoiseTexture;

float LinearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
}

float noise(float t)
{
	return texture(u_NoiseTexture, vec2(t / 64.0 + fract(u_Time / 360.0), 0.0)).x;
}
float noise(vec2 t)
{
	return texture(u_NoiseTexture, t / 64.0 + vec2(fract(u_Time / 360.0))).x;
}

vec3 lensflare(vec2 uv, vec2 pos)
{
	vec2 main = uv-pos;
	vec2 uvd = uv*(length(uv));
	
	float ang = atan(main.x,main.y);
	float dist=length(main); dist = pow(dist,.1);
	float n = noise(vec2(ang*16.0,dist*32.0));
	
	float f0 = 1.0/(length(uv-pos)*16.0+1.0);
	
	f0 = f0 + f0*(sin(noise(sin(ang*2.+pos.x)*4.0 - cos(ang*3.+pos.y))*16.)*.1 + dist*.1 + .8);
	
	float f1 = max(0.01-pow(length(uv+1.2*pos),1.9),.0)*7.0;

	float f2 = max(1.0/(1.0+32.0*pow(length(uvd+0.8*pos),2.0)),.0)*00.25;
	float f22 = max(1.0/(1.0+32.0*pow(length(uvd+0.85*pos),2.0)),.0)*00.23;
	float f23 = max(1.0/(1.0+32.0*pow(length(uvd+0.9*pos),2.0)),.0)*00.21;
	
	vec2 uvx = mix(uv,uvd,-0.5);
	
	float f4 = max(0.01-pow(length(uvx+0.4*pos),2.4),.0)*6.0;
	float f42 = max(0.01-pow(length(uvx+0.45*pos),2.4),.0)*5.0;
	float f43 = max(0.01-pow(length(uvx+0.5*pos),2.4),.0)*3.0;
	
	uvx = mix(uv,uvd,-.4);
	
	float f5 = max(0.01-pow(length(uvx+0.2*pos),5.5),.0)*2.0;
	float f52 = max(0.01-pow(length(uvx+0.4*pos),5.5),.0)*2.0;
	float f53 = max(0.01-pow(length(uvx+0.6*pos),5.5),.0)*2.0;
	
	uvx = mix(uv,uvd,-0.5);
	
	float f6 = max(0.01-pow(length(uvx-0.3*pos),1.6),.0)*6.0;
	float f62 = max(0.01-pow(length(uvx-0.325*pos),1.6),.0)*3.0;
	float f63 = max(0.01-pow(length(uvx-0.35*pos),1.6),.0)*5.0;
	
	vec3 c = vec3(.0);
	
	c.r+=f2+f4+f5+f6; c.g+=f22+f42+f52+f62; c.b+=f23+f43+f53+f63;
	c = c*1.3 - vec3(length(uvd)*.05);
	c+=vec3(f0);
	
	return c;
}

vec3 cc(vec3 color, float factor,float factor2) // color modifier
{
	float w = color.x + color.y + color.z;
	return mix(color, vec3(w) * factor, w * factor2);
}

void main()
{
	vec3 color = texture(u_ColorTexture, v_TexCoord).rgb;
	const float linearDepth = LinearDepth(texture(g_Depth, v_TexCoord).r);

	vec2 texCoord = v_TexCoord - vec2(0.5);

	// Directional Light
	if (u_UsingDirectionalLight == 1 && (linearDepth >= u_ZFar - 1))
	{
		float dotProduct = -dot(-u_View[2].xyz, normalize(-u_DirectionalLight.direction.xyz));

		if (dotProduct >= 0.0)
		{
			vec4 lightDirClip = u_ViewProjection * vec4(-u_DirectionalLight.direction.xyz, 0.0);
			// The following causes an infinte number leading to undefined behaviour. Unsure if its removal will be a major issue
			//lightDirClip /= lightDirClip.w;
			vec2 lightDirScreen = vec2(lightDirClip);

			color += vec3(u_DirectionalLight.color.rgb * sqrt(u_DirectionalLight.color.w)) * lensflare(texCoord, lightDirScreen) * (dotProduct * dotProduct);
		}
	}

	texCoord.x *= float(u_ScreenDimensions.x) / float(u_ScreenDimensions.y);

	// Point Lights
	for (uint i = 0; i < u_PointLightCount; i++)
	{
		vec3 position = u_PointLights[i].position.xyz;

		vec4 viewPos = u_View * vec4(position - u_ViewPosition.xyz, 1.0);
		bool visible = viewPos.z < 0.0;

		if (visible)
		{
			vec4 clip = u_ViewProjection * vec4(position, 1.0);
			vec3 ndc = clip.xyz / clip.w;
			vec2 uv = (vec2(ndc) + vec2(1.0)) / 2.0;

			float worldDistance = distance(u_ViewPosition.xyz, position);

			if (LinearDepth(texture(g_Depth, uv).r) < worldDistance - 1)
				continue;

			color += vec3(u_PointLights[i].color.rgb * sqrt(u_PointLights[i].intensity * 0.001)) * lensflare(v_TexCoord, uv) * (1.0 / worldDistance) * sqrt(1.0 - min(length(ndc.xy), 1.0));
		}
	}

	o_Color = vec4(cc(color, 0.5, 0.1), 1.0);
}
