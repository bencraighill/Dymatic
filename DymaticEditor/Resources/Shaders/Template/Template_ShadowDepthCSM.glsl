Compiler(Comment)
// Usage: Dymatic CSM Shadow Depth Shader

CompilerInclude(Template_ShadowVertex)

#type geometry
#version 450 core
#include Include/Buffers.glslh

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 v_TexCoord[];
layout (location = 0) out vec2 o_TexCoord;

void main()
{
	for (int i = 0; i < 3; i++)
	{
		gl_Position = u_LightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		o_TexCoord = v_TexCoord[i];
		
		EmitVertex();
	}

	EndPrimitive();
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

Compiler(ExpressionHeader)

layout (location = 0) in vec2 v_TexCoord;
float noise() { return fract(sin(dot(v_TexCoord, vec2(12.9898, 78.233))) * 43758.5453); }

Compiler(Buffers)

void main()
{
	Compiler(Alpha)
	Compiler(PreDepth)
}