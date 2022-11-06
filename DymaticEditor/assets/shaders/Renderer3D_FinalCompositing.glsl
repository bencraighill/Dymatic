// Final Compositing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;
layout (location = 1) out float o_EntityID;

layout (binding = 0) uniform sampler2D u_ColorTexture;
layout (binding = 1) uniform sampler2D u_EntityID;

layout(std140, binding = 4) uniform PostProcessing
{
    vec4 u_SSAOSamples[64];
    float u_Exposure;
    int u_BlurHorizontal;
    float u_Time;
};

vec3 aces(vec3 x) 
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 vignette()
{
	vec2 uv = v_TexCoord.xy;
   
  uv *=  1.0 - uv.yx;
  
  float vig = uv.x*uv.y * 15.0; // multiply with sth for intensity
  
  vig = pow(vig, 0.25); // change pow for modifying the extend of the  vignette

  return vec3(vig);
}

float noise(vec2 coords, float time)
{
  float e = fract((time * 0.01));

  float cx = coords.x*e;
  float cy = coords.y*e;

  return fract(23.0*fract(2.0/fract(fract(cx*2.4/cy*23.0+pow(abs(cy/22.4),3.3))*fract(cx*time/pow(abs(cy), 0.050)))));
}

const float aberrationAmount =  0.01;
const float grainAmount = 0.025;

void main()
{
  // stronger aberration near the edges by raising to power 3
	vec2 distFromCenter = v_TexCoord - 0.5;
  vec2 aberrated = aberrationAmount * pow(distFromCenter, vec2(3.0, 3.0));
  
  // Extract hdr color with the abberated texture coords applied
	vec3 hdrColor = vec3(texture(u_ColorTexture, v_TexCoord + aberrated).r, texture(u_ColorTexture, v_TexCoord).g, texture(u_ColorTexture, v_TexCoord - aberrated).b);
  
  // exposure tone mapping
  //vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
  
  // Apply ACES tonemapping
  vec3 FragColor = aces(hdrColor * u_Exposure);

  // Color Grading

  // Film Grain
  if (grainAmount != 0.0)
    FragColor += grainAmount * noise(v_TexCoord, u_Time);

  // Apply Vignette
  FragColor *= vignette();

  // Gamma Correction
	const float gamma = 2.2;
  FragColor = pow(FragColor, vec3(1.0/gamma));

  o_Color = vec4(FragColor, 1.0);
  o_EntityID = texture(u_EntityID, v_TexCoord).r;
}