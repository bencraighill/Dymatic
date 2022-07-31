#type vertex
#vertex 450 core

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

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

uniform vec3 u_Direction;
uniform vec3 camera_pos;
uniform int sky_model;

void main()
{
	vec3 dir = normalize(PS_IN_TexCoord);

	float sun = step(cos(M_PI / 360.0), dot(dir, SUN_DIR));
				
	vec3 sunColor = vec3(sun,sun,sun) * SUN_INTENSITY;
	vec3 extinction;
	vec3 inscatter = SkyRadiance(camera_pos, dir, extinction);
	vec3 col = sunColor * extinction + inscatter;
	o_Color = vec4(col, 1.0);

    o_EntityID = -1;
}