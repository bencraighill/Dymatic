#type vertex
#version 450 core

// Animation Data
#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

layout (location = 0) in vec3 a_Position;
layout(location = 5) in ivec4 a_BoneIDs;
layout(location = 6) in vec4 a_Weights;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
};

layout(std140, binding = 2) uniform Object
{
	mat4 u_Model;
	mat4 u_ModelInverse;
	mat4 lightSpaceMatrix;
	mat4 u_FinalBonesMatrices[MAX_BONES];
	int u_EntityID;
	bool u_Animated;
};

void main()
{
	if (u_Animated)
	{
		// Animation Calculation
    	vec4 totalPosition = vec4(0.0);
    	for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    	{
    	    if(a_BoneIDs[i] == -1)
    	        continue;
    	    if(a_BoneIDs[i] >= MAX_BONES) 
    	    {
    	        totalPosition = vec4(a_Position, 1.0f);
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(a_Position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];
    	}

		gl_Position =  u_ViewProjection * u_Model * totalPosition;
	}
	else
		gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type geometry
#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) out vec3 GEdgeDistance;

void main()
{
    float a = length(gl_in[1].gl_Position.xyz - gl_in[2].gl_Position.xyz);
    float b = length(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz);
    float c = length(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz);

    float alpha = acos( (b*b + c*c - a*a) / (2.0*b*c) );
    float beta = acos( (a*a + c*c - b*b) / (2.0*a*c) );
    float ha = abs( c * sin( beta ) );
    float hb = abs( c * sin( alpha ) );
    float hc = abs( b * sin( alpha ) );

    GEdgeDistance = vec3( ha, 0, 0 );
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    GEdgeDistance = vec3( 0, hb, 0 );
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    GEdgeDistance = vec3( 0, 0, hc );
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}

#type fragment
#version 450 core

const float wire_thickness = 0.005;
layout (location = 0) out vec4 o_Color;

layout (location = 0) in vec3 GEdgeDistance;

void main()
{
    //base fragment color off of which edge is closest
    float distance = min(GEdgeDistance[0], min(GEdgeDistance[1], GEdgeDistance[2]));

    if (distance<wire_thickness)
        o_Color = vec4(1.0); //draw fragment if close to edge
    else if (distance>=wire_thickness)
        discard; //discard if not
}