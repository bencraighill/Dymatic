// Motion Blur Shader

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
layout (location = 1) out int o_EntityID;

layout (binding = 0) uniform sampler2D u_SceneTexture;
layout (binding = 1) uniform sampler2D u_DepthTexture;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
};

layout(std140, binding = 2) uniform Object
{
	mat4 u_Model;
	mat4 u_ModelInverse;
	mat4 lightSpaceMatrix;
	int u_EntityID;
};

const int numSamples = 10;

void main()
{
    // Get the depth buffer value at this pixel.
    float zOverW = texture(u_DepthTexture, v_TexCoord).r;
    
    // H is the viewport position at this pixel in the range -1 to 1.
    vec4 H = vec4(v_TexCoord.x * 2 - 1, (1 - v_TexCoord.y) * 2 - 1, zOverW, 1);
    // Transform by the view-projection inverse.
    //vec4 D = mul(H, inverse(u_ViewProjection));
    vec4 D = inverse(u_ViewProjection) * H;
    
    // Divide by w to get the world position.
    vec4 worldPos = D / D.w; 
    // Current viewport position
    vec4 currentPos = H;
    
    // Use the world position, and transform by the previous view-projection matrix.
    //vec4 previousPos = mul(worldPos, /*Should be previous*/u_ViewProjection);
    vec4 previousPos = /*Should be previous*/u_Model * worldPos;
    
    // Convert to nonhomogeneous points [-1,1] by dividing by w.
    previousPos /= previousPos.w;

    // Use this frame's position and last frame's to compute the pixel velocity.
    vec2 velocity = (currentPos.xy - previousPos.xy)/2.f;

    // Get the initial color at this pixel.
    vec4 color = texture(u_SceneTexture, v_TexCoord);
    vec2 TexCoord = v_TexCoord;
    TexCoord += velocity * 0.1;

    for(int i = 1; i < numSamples; ++i, TexCoord += velocity * 0.1) 
    {   
        // Sample the color buffer along the velocity vector.
        vec4 currentColor = texture(u_SceneTexture, TexCoord);  
        // Add the current color to our color sum.   
        color += currentColor; 
    } 
    
    // Average all of the samples to get the final blur color.
    o_Color = color / numSamples; 
        
    o_EntityID = -1;
}