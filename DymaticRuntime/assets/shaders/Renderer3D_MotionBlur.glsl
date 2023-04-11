// Motion Blur Shader

// Include Fullscreen Quad Vertex Shader
#include assets/shaders/Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include assets/shaders/Buffers.glslh

layout (location = 0) in vec2 v_TexCoord;
layout (location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_SceneTexture;
layout (binding = 1) uniform sampler2D u_DepthTexture;

const int numSamples = 20;

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

    for(int i = 1; i < numSamples; ++i, TexCoord += velocity * 0.005) 
    {   
        // Sample the color buffer along the velocity vector.
        vec4 currentColor = texture(u_SceneTexture, TexCoord);  
        // Add the current color to our color sum.   
        color += currentColor; 
    } 
    
    // Average all of the samples to get the final blur color.
    o_Color = color / numSamples;
}