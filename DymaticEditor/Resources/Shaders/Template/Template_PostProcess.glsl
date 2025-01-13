Compiler(Comment)
// Usage: Dymatic Post Processing Volume Shader

CompilerInclude(Template_ConstantDefinitions)
CompilerInclude(Template_SceneDefinitions)
CompilerDefine(Pixel Position, v_TexCoord);

#type vertex
#version 450 core
#include Renderer3D_Fullscreen.glsl

#type fragment
#version 450 core
#include Include/Buffers.glslh
#include Include/Math.glslh

Compiler(ExpressionHeader)

layout (location = 0) in vec2 v_TexCoord;
layout (set = 0, binding = 20) uniform sampler2D g_Color;

layout (location = 0) out vec4 o_Color;

Compiler(Buffers)

void main()
{
    Compiler(Properties)
    o_Color = vec4(color, 1.0);
}

// End of Generated File