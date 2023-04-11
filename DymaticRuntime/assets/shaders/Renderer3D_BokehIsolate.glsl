// Compute shader to find all bokeh points

#type compute
#version 450 core
#include assets/shaders/Buffers.glslh

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (set = 0, binding = 0) uniform sampler2D u_ColorTexture;
layout (set = 0, binding = 1) uniform sampler2D u_DepthTexture;

float GetFocusScale(float depth)
{
    if (depth >= u_FocusNearStart && depth < u_FocusNearEnd)
	{
		return ((u_FocusNearEnd - depth)/(u_FocusNearEnd - u_FocusNearStart));
	}
	else if (depth >= u_FocusNearEnd && depth < u_FocusFarStart)
	{
		return 0.0;
	}
	else if (depth >= u_FocusFarStart && depth < u_FocusFarEnd)
	{
		return (1.0 - (u_FocusFarEnd - depth) / (u_FocusFarEnd - u_FocusFarStart));
	}
	else
	{
		return 1.0;
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
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    // Ensure we are inside the image dimensions
    if (pixelCoord.x >= u_ScreenDimensions.x && pixelCoord.y >= u_ScreenDimensions.y)
    {
        return;
    }

    // Ensure that the counter is reset to 0 before proceeding
    if (pixelCoord.x == 0 && pixelCoord.y == 0)
    {
        atomicExchange(u_GlobalBokehIndex, 0);
    }

    if (u_GlobalBokehIndex >= MAX_BOKEH)
    {
        return;
    }

    barrier();

    vec2 texCoord = vec2(pixelCoord) / vec2(u_ScreenDimensions);

    float focus = GetFocusScale(LinearDepth(texture(u_DepthTexture, texCoord).r));

    vec3 avgColor = vec3(0.0);
    for (int x = -2; x < 2; x++)
    {
        for (int y = -2; y < 2; y++)
        {
            avgColor += texture(u_ColorTexture, texCoord + (u_PixelSize * vec2(x, y))).xyz;
        }
    }
    avgColor /= 25.0;

    if (dot(avgColor - texture(u_ColorTexture, texCoord).xyz, vec3(0.2126, 0.7152, 0.0722)) > u_BokehThreshold)
    {
        uint index = atomicAdd(u_GlobalBokehIndex, 1);
        u_BokehList[index].position = texCoord;
        u_BokehList[index].size = focus * u_BokehSize;
        u_BokehList[index].color = avgColor;
    }
}