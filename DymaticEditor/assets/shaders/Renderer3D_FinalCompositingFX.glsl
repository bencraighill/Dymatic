// Final Compositing Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;
layout (location = 1) out float o_EntityID;

layout (binding = 0) uniform sampler2D u_ColorTexture;
layout (binding = 1) uniform sampler2D u_EntityIDTexture;

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
  
  float vig = uv.x*uv.y * u_VignetteIntensity; // multiply with sth for intensity
  
  vig = pow(vig, u_VignettePower); // change pow for modifying the extend of the  vignette

  return vec3(vig);
}

void main()
{
  // stronger aberration near the edges by raising to power 3
	vec2 distFromCenter = v_TexCoord - 0.5;
  vec2 aberrated = u_AberrationAmount * pow(distFromCenter, vec2(3.0, 3.0));
  
  // Extract hdr color with the abberated texture coords applied
	vec3 hdrColor = vec3(texture(u_ColorTexture, v_TexCoord + aberrated).r, texture(u_ColorTexture, v_TexCoord).g, texture(u_ColorTexture, v_TexCoord - aberrated).b);
  
  // exposure tone mapping
  //vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
  
  // Apply ACES tonemapping
  vec3 FragColor = aces(hdrColor);

  // Color Grading

  // Film Grain
  if (u_GrainAmount != 0.0)
  {
    float x = (v_TexCoord.x + 4.0 ) * (v_TexCoord.y + 4.0 ) * (u_Time * 10.0);
	  vec3 grain = vec3(mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005) * u_GrainAmount;
    FragColor += grain;
    FragColor = max(vec3(0.0f), FragColor);
  }

  // Apply Vignette
  FragColor *= vignette();

  // Gamma Correction
	const float gamma = u_Gamma;//2.2;
  FragColor = pow(FragColor, vec3(1.0/gamma));

  o_Color = vec4(FragColor, 1.0);
  o_EntityID = texture(u_EntityIDTexture, v_TexCoord).r;
}