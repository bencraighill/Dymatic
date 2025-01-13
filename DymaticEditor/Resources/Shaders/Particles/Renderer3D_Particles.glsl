// Render particles shader

#type vertex
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_Index;

layout (location = 0) out vec4 v_Color;
layout (location = 1) out vec2 v_TexCoord;

void main()
{
    // Attempt to performantly discard the vertex if it is being drawn but has no life remaining
    // This is quite experimental (Try disabling this if issues occur)
    if (u_Particles[a_Index].lifeRemaining == 0.0)
    {
        gl_CullDistance[0] = -1.0;
        return;
    }

    const float weight = u_Particles[a_Index].lifeRemaining / u_Particles[a_Index].lifeTime;

    // Caluclate TexCoords
    const vec2 pageSize = vec2(1.0 / u_HorizontalFlipCount, 1.0 / u_VerticalFlipCount);
    const vec2 texCoord = (a_Position.xy + vec2(1.0)) * 0.5;
    const int pageIndex = int(weight * (u_VerticalFlipCount * u_HorizontalFlipCount));
    v_TexCoord = vec2((vec2(pageIndex % u_HorizontalFlipCount, pageIndex / u_HorizontalFlipCount) + texCoord) * pageSize);

    // Calculate position and size
    const float billboardSize = mix(u_SizeEnd, u_SizeBegin, weight);
    vec3 position =
    u_Particles[a_Index].position.xyz
    + u_Right.xyz * a_Position.x * billboardSize
    + u_Up.xyz * a_Position.y * billboardSize;

    v_Color = mix(u_ColorEnd, u_ColorBegin, weight);
    v_Color.rgb *= mix(u_IntensityEnd, u_IntensityStart, weight);
    gl_Position = u_ViewProjection * vec4(position, 1.0);
}

#type fragment
#version 450 core
#include Include/Buffers.glslh

layout (location = 0) in vec4 v_Color;
layout (location = 1) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Color;

layout (set = 0, binding = 0) uniform sampler2D u_ColorMap;

void main()
{
    vec2 texCoord = v_TexCoord;
    o_Color = vec4(texture(u_ColorMap, texCoord) * v_Color);
}