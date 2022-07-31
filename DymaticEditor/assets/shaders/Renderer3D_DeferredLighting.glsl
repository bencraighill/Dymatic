#type compute
#version 450 core

// Setup compute
#define GRID_SIZE 16 
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0, rgba8) uniform readonly image2D OutputImage;

// Input Textures
layout(location = 0) out vec4 o_Position;
layout(location = 1) out vec4 o_TexCoord;
layout(location = 2) out vec4 o_Albedo;
layout(location = 3) out vec4 o_Normal;
layout(location = 4) out vec4 o_Specular;
layout(location = 5) out vec4 o_Roughness_Alpha;
layout(location = 6) out int o_EntityID;

void main()
{
}