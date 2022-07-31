#type compute
#version 450 core

#define WIDTH 2560
#define HEIGHT 1440

layout(std140, binding = 10) uniform FrameInfo
{
	float u_DeltaTime;
    float u_MoveSpeed;
    float u_EvaporateSpeed;
    float u_DiffuseSpeed;
};

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (binding = 0, rgba8) uniform readonly image2D baseImage;
layout (binding = 1, rgba8) uniform image2D resultImage;

void main() 
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec4 originalValue = imageLoad(baseImage, pixelCoord);

    vec4 sum = vec4(0.0);
    for (int offsetX = -1; offsetX <= 1; offsetX++)
    {
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            int sampleX = int(gl_GlobalInvocationID.x) + offsetX;
            int sampleY = int(gl_GlobalInvocationID.y) + offsetY;

            if (sampleX >= 0 && sampleX < WIDTH && sampleY >= 0 && sampleY < HEIGHT)
            {
                sum += imageLoad(baseImage, ivec2(sampleX, sampleY));
            }
        }
    }

    vec4 blurResult = sum / 9.0;

    vec4 diffusedValue = mix(originalValue, blurResult, u_DiffuseSpeed * u_DeltaTime);

    vec4 diffusedAndEvaporatedValue = max(vec4(0.0), diffusedValue - u_EvaporateSpeed * u_DeltaTime);

    imageStore(resultImage, pixelCoord, diffusedAndEvaporatedValue);
}