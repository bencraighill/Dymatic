// Final Compositing Shader

#type vertex
#version 450 core

const vec2 madd = vec2(0.5,0.5);

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec2 o_TexCoord;

void main() 
{
   o_TexCoord = a_Position.xy*madd+madd; // scale vertex attribute to [0-1] range
   gl_Position = vec4(a_Position.xy,0.0,1.0);
}

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

  const float aberrationAmount =  0.05;

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

  // Apply Vignette
  FragColor *= vignette();

  // Gamma Correction
	const float gamma = 2.2;
  FragColor = pow(FragColor, vec3(1.0/gamma));

  o_Color = vec4(FragColor, 1.0);
  o_EntityID = texture(u_EntityID, v_TexCoord).r;
}