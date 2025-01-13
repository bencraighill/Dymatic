// G Buffer Visualization Shader

// Include Fullscreen Quad Vertex Shader
#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

float hash(float n)
{
    return fract(sin(n) * 1e4);
}

vec3 RandomColor(int seed)
{
    float r = hash(float(seed) * 0.123456 + 0.1);
    float g = hash(float(seed) * 0.654321 + 0.2);
    float b = hash(float(seed) * 0.987654 + 0.3);
    return vec3(r, g, b);
}

void main()
{
    switch (u_VisualizationMode)
    {
        case VISUALIZATION_MODE_ALBEDO:    
        {
            o_Color = texture(g_Albedo, v_TexCoord);
            break;
        }
        case VISUALIZATION_MODE_DEPTH:
        {
            o_Color = vec4(vec3(texture(g_Depth, v_TexCoord).r), 1.0);
            break;
        }
        case VISUALIZATION_MODE_LINEAR_DEPTH:
        {
            o_Color = vec4(vec3(LinearDepth(texture(g_Depth, v_TexCoord).r) / (u_ZFar - u_ZNear)), 1.0);
            break;
        }
        case VISUALIZATION_MODE_POSITION:
        {
            const float depth = texture(g_Depth, v_TexCoord).r;
            o_Color = vec4(WorldPosFromDepth(depth, v_TexCoord), 1.0);
            break;
        }
        case VISUALIZATION_MODE_NORMAL:
        {
            o_Color = vec4(texture(g_Normal, v_TexCoord).rgb, 1.0);
            break;
        }
        case VISUALIZATION_MODE_EMISSIVE:
        {
            o_Color = vec4(texture(g_Emissive, v_TexCoord).rgb, 1.0);
            break;
        }
        case VISUALIZATION_MODE_ROUGHNESS:
        {
            o_Color = vec4(vec3(texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).r), 1.0);
            break;
        }
        case VISUALIZATION_MODE_METALLIC:
        {
            o_Color = vec4(vec3(texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).g), 1.0);
            break;
        }
        case VISUALIZATION_MODE_SPECULAR:
        {
            o_Color = vec4(vec3(texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).b), 1.0);
            break;
        }
        case VISUALIZATION_MODE_AMBIENT_OCCLUSION:
        {
            o_Color = vec4(vec3(texture(g_Roughness_Metallic_Specular_AO, v_TexCoord).a), 1.0);
            break;
        }
        case VISUALIZATION_MODE_VELOCITY:
        {
            o_Color = vec4(texture(g_Velocity, v_TexCoord).rg, 0.0, 1.0);
            break;
        }
        case VISUALIZATION_MODE_OBJECT_ID:
        {
            const int entityID = texture(g_EntityID, v_TexCoord).r;
            o_Color = entityID == -1 ? vec4(0.1, 0.1, 0.1, 1.0) : vec4(RandomColor(texture(g_EntityID, v_TexCoord).r), 1.0);
            break;
        }
        case VISUALIZATION_MODE_SUBMESH_INDEX:
        {
            const int entityID = texture(g_EntityID, v_TexCoord).r;
            const int submeshIndex = texture(g_SubmeshIndex, v_TexCoord).r;
            const vec3 randomColor = RandomColor(submeshIndex + entityID * 2);

            o_Color = (entityID == -1 || submeshIndex == -1 ) ? vec4(0.1, 0.1, 0.1, 1.0) : vec4(randomColor, 1.0);
            break;
        }
        default:
        {
            o_Color = vec4(1.0, 0.0, 1.0, 1.0);
        }
    }
}