struct VertexOutput
{
    vec3 Position;
    vec4 PreviousClipPosition;
    vec3 Normal;
    vec2 TexCoord;

CompilerIf(VertexIndex)
    float GlobalIndex;
CompilerEndIf()

CompilerIf(VertexColor)
    vec4 Color;
CompilerEndIf()

CompilerIf(Normal)
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat3 TBN;
CompilerEndIf()
};