// SRR Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_FinalImage;
layout (binding = 1) uniform sampler2D u_Depth;
layout (binding = 2) uniform sampler2D u_Normal;
layout (binding = 3) uniform sampler2D u_Roughness_Metallic_Specular;

const float rayStep = 0.1;
const float minRayStep = 0.1;
const float maxSteps = 30;
const float searchDist = 5;
const float searchDistInv = 0.2;
const int numBinarySearchSteps = 5;
const float maxDDepth = 1.0;
const float maxDDepthInv = 1.0;
const float reflectionSpecularFalloffExponent = 3.0;

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec3 PositionFromDepth(float depth);

vec3 BinarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth);
 
vec4 RayCast(vec3 dir, inout vec3 hitCoord, out float dDepth);

vec3 fresnelSchlick(float cosTheta, vec3 F0);

vec3 hash(vec3 a);

void main()
{
    float Metallic = texture(u_Roughness_Metallic_Specular, v_TexCoord).y;

    //if(Metallic < 0.01)
    //    discard;
 
    vec3 viewNormal = vec3(texture(u_Normal, v_TexCoord) * u_InverseView);
    vec3 viewPos = PositionFromDepth(texture(u_Depth, v_TexCoord).r);//textureLod(gPosition, v_TexCoord, 2).xyz;
    vec3 albedo = texture(u_FinalImage, v_TexCoord).rgb;

    float spec = texture(u_Roughness_Metallic_Specular, v_TexCoord).z;

    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, Metallic);
    vec3 Fresnel = fresnelSchlick(max(dot(normalize(viewNormal), normalize(viewPos)), 0.0), F0);

    // Reflection vector
    vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));


    vec3 hitPos = viewPos;
    float dDepth;
 
    vec3 wp = vec3(vec4(viewPos, 1.0));
    vec3 jitt = mix(vec3(0.0), normalize(vec3(hash(wp))), spec);
    vec4 coords = RayCast((/*vec3(jitt) + */reflected * max(minRayStep, -viewPos.z)), hitPos, dDepth);
 
 
    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - coords.xy));
 
 
    float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

    float ReflectionMultiplier = pow(Metallic, reflectionSpecularFalloffExponent) * 
                screenEdgefactor * 
                -reflected.z;
 
    // Get color
    vec3 SSR = textureLod(u_FinalImage, coords.xy, 0).rgb * clamp(ReflectionMultiplier, 0.0, 0.9) * Fresnel;  

    //o_Color = vec4(SSR, Metallic);
    o_Color = vec4(albedo + SSR * Metallic, 1.0);
}

vec3 PositionFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(v_TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_InverseProjection * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

vec3 BinarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth)
{
    float depth;

    vec4 projectedCoord;
 
    for(int i = 0; i < numBinarySearchSteps; i++)
    {

        projectedCoord = u_Projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
        depth = PositionFromDepth(texture(u_Depth, projectedCoord.xy).r).z;

        dDepth = hitCoord.z - depth;

        dir *= 0.5;

        hitCoord += (dDepth > 0.0) ? dir : -dir;
    }

        projectedCoord = u_Projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
    return vec3(projectedCoord.xy, depth);
}

vec4 RayCast(vec3 dir, inout vec3 hitCoord, out float dDepth)
{
    dir *= rayStep;

    float depth;
    int steps;
    vec4 projectedCoord;

    for(int i = 0; i < maxSteps; i++)
    {
        hitCoord += dir;
 
        projectedCoord = u_Projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
        depth = PositionFromDepth(texture(u_Depth, projectedCoord.xy).r).z;

        if(depth > 1000.0){
            continue;
        }
 
        dDepth = hitCoord.z - depth;

        if((dir.z - dDepth) < 1.2){
            if(dDepth <= 0.0){   
                vec4 Result;
                Result = vec4(BinarySearch(dir, hitCoord, dDepth), 1.0);

                return Result;
            }
        }
        
        steps++;
    }
 
    
    return vec4(projectedCoord.xy, depth, 0.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}