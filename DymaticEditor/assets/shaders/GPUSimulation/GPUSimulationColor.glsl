#type compute
#version 450 core

#define MAX_AGENTS 819200
#define NUM_SPECIES 4

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (binding = 0, rgba8) uniform readonly image2D baseImage;
layout (binding = 1, rgba8) uniform image2D resultImage;

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

void main() 
{
    vec4 map = imageLoad(baseImage, ivec2(gl_GlobalInvocationID.xy));

	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	for (uint i = 0; i < NUM_SPECIES; i ++) {
		vec4 mask = vec4(i==0, i==1, i==2,i==3);
		color += u_SpeciesSettings[i].color * dot(map, mask);
	}

    imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), color);
}