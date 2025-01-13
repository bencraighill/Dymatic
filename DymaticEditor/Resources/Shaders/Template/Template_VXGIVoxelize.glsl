Compiler(Comment)
// Usage: Dymatic VXGI Voxelization Shader

CompilerInclude(Template_SurfaceDefinitions)

#type vertex
#version 450 core
#include Include/Buffers.glslh
#include Include/Transformations.glslh
CompilerIf(Normal)
#include Include/TBN.glslh
CompilerEndIf()

layout (location = 0) in uint a_GlobalIndex;
layout (location = 1) in vec3 a_Position;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in vec3 a_Tangent;
layout (location = 5) in vec3 a_Bitangent;
layout (location = 6) in vec4 a_Color;
layout (location = 7) in ivec4 a_BoneIDs;
layout (location = 8) in vec4 a_Weights;

#define SURFACE_POSITION Input.Position
#define SURFACE_NORMAL a_Normal
#define SURFACE_TEXTURE_COORD a_TexCoord
#define SURFACE_COLOR a_Color

out InOutData
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;

CompilerIf(VertexIndex)
	float GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    vec4 Color;
CompilerEndIf()

CompilerIf(Normal)
	mat3 TBN;
CompilerEndIf()
} Input;

Compiler(ExpressionHeader)

Compiler(Buffers)

void main()
{
	// Note: VXGI does not account for blend shapes

	if (u_Animated == 1)
	{
		// Animation Calculation
    	vec4 totalPosition = vec4(0.0);
		vec3 totalNormal = vec3(0.0);
    	for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    	{
    	    if(a_BoneIDs[i] == -1)
    	        continue;
    	    if(a_BoneIDs[i] >= MAX_BONES) 
    	    {
    	        totalPosition = vec4(a_Position, 1.0f);
				totalNormal = a_Normal;
    	        break;
    	    }

			// Position
    	    vec4 localPosition = u_FinalBonesMatrices[a_BoneIDs[i]] * vec4(a_Position, 1.0f);
    	    totalPosition += localPosition * a_Weights[i];

			// Normal
    	    vec3 localNormal = mat3(u_FinalBonesMatrices[a_BoneIDs[i]]) * a_Normal;
    	    totalNormal += localNormal * a_Weights[i];
    	}

        Input.Position = vec3(u_Model * totalPosition);
		Input.Normal = mat3(u_ModelInverse) * totalNormal;
	}
	else
	{
        Input.Position = vec3(u_Model * vec4(a_Position, 1.0));
		Input.Normal = mat3(u_ModelInverse) * a_Normal;
	}

	Compiler(Displacement)

	Input.TexCoord = a_TexCoord;

CompilerIf(VertexIndex)
    Input.GlobalIndex = a_GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    Input.Color = a_Color;
CompilerEndIf()

CompilerIf(Normal)
    Input.TBN = CalculateTBN(a_Normal, a_Tangent);
CompilerEndIf()

    // Transform fragPos from [GridMin, GridMax] to [-1, 1]
    const vec3 ndc = MapToZeroOne(Input.Position, u_GridMin[u_RenderCascade].xyz, u_GridMax[u_RenderCascade].xyz);
    gl_Position = vec4(ndc, 1.0);

#if !TAKE_FAST_GEOMETRY_SHADER_PATH
    if (u_RenderAxis == 0) gl_Position = gl_Position.zyxw;
    if (u_RenderAxis == 1) gl_Position = gl_Position.xzyw;
#endif
}

#type geometry
#require TAKE_FAST_GEOMETRY_SHADER_PATH
#version 450 core

#extension GL_NV_geometry_shader_passthrough: enable
#if !GL_NV_geometry_shader_passthrough
#error "Cannot compile voxelize geometry shader stage as GL_NV_geometry_shader_passthrough is not supported. Use the fallback path!"
#endif

layout(triangles) in;

layout(passthrough) in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

layout(passthrough) in InOutData
{
    vec3 Position;
	vec3 Normal;
	vec2 TexCoord;

CompilerIf(VertexIndex)
	float GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    vec4 Color;
CompilerEndIf()

CompilerIf(Normal)
	mat3 TBN;
CompilerEndIf()
} Input[];

void main()
{
    vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 normalWeights = abs(cross(p1, p2));

    int dominantAxis = normalWeights.y > normalWeights.x ? 1 : 0;
    dominantAxis = normalWeights.z > normalWeights[dominantAxis] ? 2 : dominantAxis;

    // Swizzle is applied by selecting a viewport
    // This works using the GL_NV_viewport_swizzle extension
    gl_ViewportIndex = 2 - dominantAxis;
}

#type fragment
#version 450 core
#extension GL_ARB_bindless_texture: require

#if TAKE_ATOMIC_FP16_PATH
    #extension GL_NV_shader_atomic_fp16_vector: require
    #extension GL_NV_gpu_shader5: require
#endif

#include Include/Buffers.glslh
#include Include/Transformations.glslh
#include Include/Math.glslh

// Lighting Resources
layout (set = 0, binding = 13) uniform sampler2DArray u_ShadowMap;
layout (set = 0, binding = 14) uniform samplerCubeArray u_ShadowMapArray;

// Additional includes
#include Include/Lighting.glslh

#define SURFACE_POSITION Input.Position
#define SURFACE_NORMAL Input.Normal
#define SURFACE_TEXTURE_COORD Input.TexCoord
#define SURFACE_COLOR Input.Color

Compiler(ExpressionHeader)

in InOutData
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;

CompilerIf(VertexIndex)
	float GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    vec4 Color;
CompilerEndIf()

CompilerIf(Normal)
	mat3 TBN;
CompilerEndIf()
} Input;

layout (rgba16f, binding = 0) restrict uniform image3D u_VoxelGrid;

#if !TAKE_ATOMIC_FP16_PATH
layout(binding = 1, r32ui) restrict uniform uimage3D u_VoxelGridR;
layout(binding = 2, r32ui) restrict uniform uimage3D u_VoxelGridG;
layout(binding = 3, r32ui) restrict uniform uimage3D u_VoxelGridB;
#endif

ivec3 WorlSpaceToVoxelImageSpace(vec3 worldPos);

Compiler(Buffers)

void main()
{
    Compiler(Properties)

CompilerIf(Normal)
    normal = (2.0 * normal - 1.0);
    normal = normalize(Input.TBN * normal); //going -1 to 1
CompilerElse()
    vec3 normal = normalize(Input.Normal);
CompilerEndIf()

	const vec3 radianceOut = CalculateLighting(albedo, normal, emissive, roughness, metallic, vec3(specular), ao, Input.Position, 0.0) * alpha;
    const ivec3 voxelPos = WorlSpaceToVoxelImageSpace(Input.Position);

#if TAKE_ATOMIC_FP16_PATH

    imageAtomicMax(u_VoxelGrid, voxelPos, f16vec4(vec4(radianceOut, alpha)));

#else

	imageAtomicMax(u_VoxelGridR, voxelPos, floatBitsToUint(radianceOut.r));
    imageAtomicMax(u_VoxelGridG, voxelPos, floatBitsToUint(radianceOut.g));
    imageAtomicMax(u_VoxelGridB, voxelPos, floatBitsToUint(radianceOut.b));
    imageStore(u_VoxelGrid, voxelPos, vec4(0.0, 0.0, 0.0, 1.0));

#endif
}

ivec3 WorlSpaceToVoxelImageSpace(vec3 worldPos)
{
    vec3 uvw = MapToZeroOne(worldPos, u_GridMin[u_RenderCascade].xyz, u_GridMax[u_RenderCascade].xyz);
    ivec3 voxelPos = ivec3(uvw * u_GridResolution[u_RenderCascade]);
    return voxelPos;
}

// End of Generated File