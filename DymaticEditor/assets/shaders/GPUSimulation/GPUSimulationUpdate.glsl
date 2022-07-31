#type compute
#version 450 core

#define NUM_SPECIES 4
#define MAX_AGENTS 819200
#define WIDTH 4480
#define HEIGHT 1080
#define PI 3.1415926535897932384626433832795

struct Agent
{
    vec2 position;
    float angle;
    float BUFF_DISCARD;
    ivec4 speciesMask;

};

struct SpeciesSettings
{
    vec4 color;
};

layout(std430, binding = 9) buffer AgentBindings
{
	Agent u_Agents[MAX_AGENTS];
    SpeciesSettings u_SpeciesSettings[NUM_SPECIES];
};

layout(std140, binding = 10) uniform FrameInfo
{
	float u_DeltaTime;
    float u_MoveSpeed;
    float u_EvaporateSpeed;
    float u_DiffuseSpeed;
    float u_TrailWeight;
    int u_SensorOffsetDst;
    int u_SensorSize;
    float u_SensorAngleSpacing;
    float u_TurnSpeed;
};

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0, rgba8) uniform image2D resultImage;

int hash(int state)
{
    uint value = uint(state);
	value ^= 2747636419u;
	value *= 2654435769u;
	value ^= state >> 16;
	value *= 2654435769u;
	value ^= state >> 16;
	value *= 2654435769u;
	return int(value);
}

float scaleToRange01(int value)
{
    return value / 4294967295.0;
}

float sense(Agent agent, float sensorAngleOffset)
{
    float sensorAngle = agent.angle + sensorAngleOffset;
    vec2 sensorDir = vec2(cos(sensorAngle), sin(sensorAngle));
    ivec2 sensorCentre = ivec2(agent.position + sensorDir * u_SensorOffsetDst);
    float sum = 0;

    for (int offsetX = -u_SensorSize; offsetX <= u_SensorSize; offsetX++)
    {
        for (int offsetY = -u_SensorSize; offsetY <= u_SensorSize; offsetY++)
        {
            ivec2 pos = sensorCentre + ivec2(offsetX, offsetY);

            if (pos.x >= 0 && pos.x < WIDTH && pos.y >= 0 && pos.y < HEIGHT)
            {
                sum += dot(imageLoad(resultImage, pos), agent.speciesMask * 2 - 1);
            }     
        }
    }

    return sum;
}

void main() 
{
    Agent agent = u_Agents[gl_GlobalInvocationID.x];
    int random = hash(int(agent.position.y * WIDTH + agent.position.x + hash(int(gl_GlobalInvocationID.x))));



     // Steer based on sensory data
    float weightForward = sense(agent, 0);
    float weightLeft = sense(agent, u_SensorAngleSpacing);
    float weightRight = sense(agent, -u_SensorAngleSpacing);

    float randomSteerStrength = scaleToRange01(random);

    // Continue in same direction
    if (weightForward > weightLeft && weightForward > weightRight)
    {
        // Do nothing
    }
    // Turn randomly
    else if (weightForward < weightLeft && weightForward < weightRight)
    {
        u_Agents[gl_GlobalInvocationID.x].angle += (randomSteerStrength - 0.5) * 2.0 * u_TurnSpeed * u_DeltaTime;
    }
    // Turn right
    else if (weightRight > weightLeft)
    {
        u_Agents[gl_GlobalInvocationID.x].angle -= randomSteerStrength * u_TurnSpeed * u_DeltaTime;
    }
    // Turn left
    else if (weightLeft > weightRight)
    {
        u_Agents[gl_GlobalInvocationID.x].angle += randomSteerStrength * u_TurnSpeed * u_DeltaTime;
    }



    // Move Agent
    vec2 direction = vec2(cos(agent.angle), sin(agent.angle));
    vec2 newPos = agent.position + (direction * u_MoveSpeed * u_DeltaTime);

    // Clamp Position
    if (newPos.x < 0 || newPos.x >= WIDTH || newPos.y < 0 || newPos.y >= HEIGHT)
    {
        newPos.x = min(WIDTH - 2.0, max(2, newPos.x));
        newPos.y = min(HEIGHT - 2.0, max(2, newPos.y));
        u_Agents[gl_GlobalInvocationID.x].angle = scaleToRange01(random) * 2.0 * PI;
    }

    u_Agents[gl_GlobalInvocationID.x].position = newPos;

    imageStore(resultImage, ivec2(u_Agents[gl_GlobalInvocationID.x].position), u_TrailWeight * u_DeltaTime * agent.speciesMask);
}