// Basic Grid Shader

#type vertex
#version 450 core

layout(location = 0) in int a_VertexIndex;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_ViewPosition;
};

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

// Grid position are in clipped space
vec3 gridPlane[4] = vec3[] (
    vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0),
    vec3(-1, 1, 0)
);

vec3 UnprojectPoint(float x, float y, float z) {
    mat4 viewProjInv = inverse(u_ViewProjection);
    vec4 unprojectedPoint =  viewProjInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec3 p = gridPlane[a_VertexIndex].xyz;
    nearPoint = UnprojectPoint(p.x, p.y, 0.0).xyz; // unprojecting on the near plane
    farPoint = UnprojectPoint(p.x, p.y, 1.0).xyz; // unprojecting on the far plane
    gl_Position = vec4(p, 1.0); // using directly the clipped coordinates
}

#type fragment
#version 450 core

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale; // use the scale variable to set the distance between the lines
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
    return color;
}
void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    o_Color = grid(fragPos3D, 10) * float(t > 0);
    o_EntityID = -1;
}