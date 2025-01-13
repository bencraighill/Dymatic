Compiler(Comment)
// Usage: Dymatic Shadow Depth Shader

CompilerInclude(Template_ShadowVertex)

#type geometry
#version 450 core
#include Include/Buffers.glslh

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout (location = 0) in vec2 v_TexCoord[];
layout (location = 0) out vec2 o_TexCoord;

layout (location = 1) out vec4 o_Position;

void main()
{
    for(int face = 0; face < 6; face++)
    {
		// u_CascadeCount reused for current shadow index

        gl_Layer = (u_CascadeCount * 6 + face);
		
	    for (int i = 0; i < 3; i++)
	    {
	    	o_TexCoord = v_TexCoord[i];
            o_Position = gl_in[i].gl_Position;
            gl_Position = u_LightSpaceMatrices[face] * o_Position;
            EmitVertex();
	    }

	    EndPrimitive();
    }
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

Compiler(ExpressionHeader)

layout (location = 0) in vec2 v_TexCoord;
layout (location = 1) in vec4 v_Position;

Compiler(Buffers)

float noise() { return fract(sin(dot(v_TexCoord, vec2(12.9898, 78.233))) * 43758.5453); }

void main()
{
	Compiler(Alpha)
	Compiler(PreDepth)

    // Get current light being rendered

    // Calcualte Depth Output
    float lightDistance = length(v_Position.xyz - u_PointLights[u_CascadeCount].position.xyz); // u_CascadeCount reused for current shadow index
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / u_CascadePlaneDistances[0][0]; // u_CascadePlaneDistances[0][0] reused for far plane
    // write this as modified depth
    gl_FragDepth = lightDistance;
}