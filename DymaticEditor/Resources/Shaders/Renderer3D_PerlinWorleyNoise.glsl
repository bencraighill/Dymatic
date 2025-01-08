// Perlin-Worley Noise Shader for Volumetric Cloud Simulation

#type compute
#version 450 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout (set = 0, binding = 0) restrict writeonly uniform image3D u_Output;

const float OutputSize = 256.0;

// Hash by David_Hoskins
#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uvec2(UI0, UI1)
#define UI3 uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xffffffffU))

vec3 hash33(vec3 p)
{
    uvec3 q = uvec3(ivec3(p)) * UI3;
	q = (q.x ^ q.y ^ q.z) * UI3;
	return -1. + 2. * vec3(q) * UIF;
}

#undef UI0
#undef UI1
#undef UI2
#undef UI3
#undef UIF

float gradientNoise(vec3 x, float frequency)
{
    // Grid
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    // Quintic interpolant
    vec3 u = w * w * w * (w * (w * 6.0 - 15.0) + 10.0);
    
    // Gradients
    vec3 ga = hash33(mod(p + vec3(0.0, 0.0, 0.0), frequency));
    vec3 gb = hash33(mod(p + vec3(1.0, 0.0, 0.0), frequency));
    vec3 gc = hash33(mod(p + vec3(0.0, 1.0, 0.0), frequency));
    vec3 gd = hash33(mod(p + vec3(1.0, 1.0, 0.0), frequency));
    vec3 ge = hash33(mod(p + vec3(0.0, 0.0, 1.0), frequency));
    vec3 gf = hash33(mod(p + vec3(1.0, 0.0, 1.0), frequency));
    vec3 gg = hash33(mod(p + vec3(0.0, 1.0, 1.0), frequency));
    vec3 gh = hash33(mod(p + vec3(1.0, 1.0, 1.0), frequency));
    
    // Projections
    float va = dot(ga, w - vec3(0.0, 0.0, 0.0));
    float vb = dot(gb, w - vec3(1.0, 0.0, 0.0));
    float vc = dot(gc, w - vec3(0.0, 1.0, 0.0));
    float vd = dot(gd, w - vec3(1.0, 1.0, 0.0));
    float ve = dot(ge, w - vec3(0.0, 0.0, 1.0));
    float vf = dot(gf, w - vec3(1.0, 0.0, 1.0));
    float vg = dot(gg, w - vec3(0.0, 1.0, 1.0));
    float vh = dot(gh, w - vec3(1.0, 1.0, 1.0));
	
    // Interpolation
    return va + 
           u.x * (vb - va) + 
           u.y * (vc - va) + 
           u.z * (ve - va) + 
           u.x * u.y * (va - vb - vc + vd) + 
           u.y * u.z * (va - vc - ve + vg) + 
           u.z * u.x * (va - vb - ve + vf) + 
           u.x * u.y * u.z * (-va + vb + vc - vd + ve - vf - vg + vh);
}

float worleyNoise(vec3 uv, float frequency)
{
    const vec3 id = floor(uv);
    const vec3 position = fract(uv);

    float minDistance = 10000.0;
    for (float x = -1.0; x <= 1.0; x++)
    {
        for (float y = -1.0; y <= 1.0; y++)
        {
            for (float z = -1.0; z <= 1.0; z++)
            {
                vec3 offset = vec3(x, y, z);
                vec3 h = hash33(mod(id + offset, vec3(frequency))) * 0.5 + 0.5;
                h += offset;
                vec3 d = position - h;
                minDistance = min(minDistance, dot(d, d));
            }
        }
    }

    // Inverted worley noise
    return 1.0 - minDistance;
}

float perlinFbm(vec3 position, float frequency, int octaves)
{
    const float G = exp2(-0.85);
    float amp = 1.0;
    float noise = 0.0;

    for (int i =0; i < octaves; i++)
    {
        noise += amp * gradientNoise(position * frequency, frequency);
        frequency *= 2.0;
        amp *= G;
    }

    return noise;
}

float worleyFbm(vec3 position, float frequency)
{
    return  worleyNoise(position * frequency * 1.0, frequency * 1.0) * 0.625 +
            worleyNoise(position * frequency * 2.0, frequency * 2.0) * 0.250 +
            worleyNoise(position * frequency * 4.0, frequency * 4.0) * 0.125;

}

void main()
{
    //if (any(greaterThanEqual(ivec3(gl_GlobalInvocationID), imageSize(u_Output))))
    //    return;

    const vec3 position = vec3(gl_GlobalInvocationID.xyz) / OutputSize;

    const float freq = 4.0;

    const float worley0 = worleyFbm(position, freq * 1.0);
    const float worley1 = worleyFbm(position, freq * 2.0);
    const float worley2 = worleyFbm(position, freq * 4.0);

    float pfbm = mix(1.0, perlinFbm(position, 4.0, 7), 0.5);
    pfbm = abs(pfbm * 2.0 - 1.0); // billowy perlin noise

    // Pack perlin-worley noise into texture
    imageStore(u_Output, ivec3(gl_GlobalInvocationID), vec4(pfbm, worley0, worley1, worley2));
}