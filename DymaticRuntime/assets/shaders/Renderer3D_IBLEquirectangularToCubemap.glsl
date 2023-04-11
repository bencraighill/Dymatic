// IBL Equirectangular to Cubemap Shader

// Include cubemap vertex shader
#include assets/shaders/Renderer3D_IBLCubemap.glsl

#type fragment
#version 450 core

layout (location = 0) in vec3 v_Position;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_EquirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(v_Position));
    vec3 color = texture(u_EquirectangularMap, uv).rgb;
    
    o_Color = vec4(color, 1.0);
}