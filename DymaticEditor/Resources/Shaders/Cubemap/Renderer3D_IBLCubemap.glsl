// IBL Cubemap Vertex Shader

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec3 o_Position;

void main()
{
    o_Position = a_Position;

    // Note: u_Model is repurposed here to represent u_ViewProjection matrix
    gl_Position = u_Model * vec4(a_Position, 1.0);
}