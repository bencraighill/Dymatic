// G Buffer Visualization Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D g_Albedo;
layout (binding = 1) uniform isampler2D g_EntityID;
layout (binding = 2) uniform sampler2D g_Normal;
layout (binding = 3) uniform sampler2D g_Emissive;
layout (binding = 4) uniform sampler2D g_Roughness_Metallic_Specular_AO;
layout (binding = 5) uniform sampler2D g_Depth;

vec3 RandomColor(int seed)
{
    float r = fract(sin(float(seed)*43758.5453123));
    float g = fract(sin(float(seed)*76543.1415926));
    float b = fract(sin(float(seed)*53123.8765432));
    return vec3(r, g, b);
}

float LinearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
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
            o_Color = vec4(vec3(sqrt(LinearDepth(texture(g_Depth, v_TexCoord).r) / (u_ZFar - u_ZNear))), 1.0);
            break;
        }
        case VISUALIZATION_MODE_OBJECT_ID:
        {
            o_Color = vec4(RandomColor(texture(g_EntityID, v_TexCoord).r), 1.0);
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
        default:
        {
            o_Color = vec4(1.0, 0.0, 1.0, 1.0);
        }
    }
}