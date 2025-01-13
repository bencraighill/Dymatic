// Grass Shader

#type vertex
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec3 a_Position;

struct VertexOutput
{
	vec3 Normal;
    float Height;
};

layout (location = 0) out VertexOutput Output;

layout (set = 0, binding = 0) uniform sampler2D u_PerlinNoise;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float calculateRotationAngle(vec3 bladeDirection, vec3 toCamera)
{
    bladeDirection.y = 0.0; // Project onto XZ plane
    toCamera.y = 0.0;       // Project onto XZ plane
    bladeDirection = normalize(bladeDirection);
    toCamera = normalize(toCamera);
    
    float cosTheta = dot(bladeDirection, toCamera);
    return acos(cosTheta); // This is the angle in radians
}

mat3 rotateY(float angle)
{
    float cosTheta = cos(angle);
    float sinTheta = sin(angle);
    
    return mat3(
        cosTheta, 0.0, sinTheta,
        0.0,     1.0, 0.0,
        -sinTheta, 0.0, cosTheta
    );
}

void main()
{
    const float height = a_Position.y;
    Output.Height = height;

    const vec2 position = vec2(0.25 * (gl_InstanceIndex % 100), 0.25 * (-gl_InstanceIndex / 100));

    const float noise = texture(u_PerlinNoise, vec2(sin(u_Time * 0.05 + gl_InstanceIndex * 0.01), cos(u_Time * 0.05 + gl_InstanceIndex * 0.01))).r;
    const vec3 vertexOffset = vec3(2.0 * rand(position) - 1.0, rand(position), 2.0 * rand(position) - 1.0) * height * exp(height * noise) * sin(noise) * vec3(0.6, 0.3, 0.6);
    const vec3 worldPos = vec3(position.x, 0.0, position.y);

    vec3 toCamera = normalize(u_ViewPosition.xyz - worldPos);
    vec3 bladeForward = (u_Model * vec4(0.0, 0.0, 1.0, 0.0)).xyz;
    mat3 rotationMatrix = rotateY(calculateRotationAngle(bladeForward, toCamera));

	gl_Position = u_ViewProjection * vec4(vec3(mat4(1.0) * vec4(rotationMatrix * a_Position + worldPos + vertexOffset, 1.0)), 1.0);
	Output.Normal = vec3(a_Position.x * 10.0, 0.0, 1.0);
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out int o_EntityID;
layout(location = 2) out vec4 o_Normal;
layout(location = 3) out vec4 o_Emissive;
layout(location = 4) out vec4 o_Roughness_Metallic_Specular_AO;

struct VertexOutput
{
	vec3 Normal;
    float Height;
};

layout (location = 0) in VertexOutput Input;

void main()
{
	o_Albedo = vec4(mix(vec3(0.2, 0.9, 0.1), vec3(0.4, 0.65, 0.1), Input.Height), 1.0);
    o_EntityID = -1;
    o_Normal = vec4(Input.Normal, 1.0);
	o_Emissive = vec4(0.0, 0.0, 0.0, 1.0);
    o_Roughness_Metallic_Specular_AO = vec4(1.0, 0.0, 1.0, 0.0);
}