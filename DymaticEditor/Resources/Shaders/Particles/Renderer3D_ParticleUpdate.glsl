// Update particle system compute shader

#type compute
#version 450 core
#include Include/Buffers.glslh

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

float LinearDepth(float depthSample)
{
    float depthRange = 2.0 * depthSample - 1.0;
    // Near... Far... wherever you are...
    float linear = (2.0 * u_ZNear * u_ZFar) / (u_ZFar + u_ZNear - depthRange * (u_ZFar - u_ZNear));
    return linear;
}

float rand(float seed)
{
    return fract(sin(seed) * 143758.5453);
}

float random(float seed)
{
    return rand(rand(u_Time) + rand(seed));
}

vec3 randomVec3(float seed)
{
    float r = fract(sin(seed) * 43758.5453);
    float g = fract(sin(seed * 31.345) * 43758.5453);
    float b = fract(sin(seed * 61.789) * 43758.5453);
    return vec3(mix(0.0, 1.0, r), mix(0.0, 1.0, g), mix(0.0, 1.0, b));
}

void InitParticle(uint index)
{
    u_Particles[index].position = vec4(u_Model[3].xyz + u_MinimumPosition.xyz + (sqrt(randomVec3(u_Time) * randomVec3(fract(index * 58.958)))) * (u_MaximumPosition.xyz - u_MinimumPosition.xyz), 1.0);
    u_Particles[index].velocity = vec4(u_MinimumVelocity.xyz + (sqrt(randomVec3(u_Time) * randomVec3(index))) * (u_MaximumVelocity.xyz - u_MinimumVelocity.xyz), 1.0);
    u_Particles[index].lifeTime = u_Lifetime;
    u_Particles[index].lifeRemaining = u_Lifetime;
}


void main()
{
    uint index = gl_GlobalInvocationID.x;

    if (index >= u_MaxParticles)
        return;
    
    // Create particles code
    if (index == 0 && u_ParticleCount < u_MaxParticles - 1)
    {
        uint currentIndex = 0;
        uint emissionIndex = 0;
        while (emissionIndex < u_EmissionCount)
        {
            if (currentIndex < u_ParticleCount)
            {
                if (u_Particles[currentIndex].lifeRemaining == 0.0)
                {
                    InitParticle(currentIndex);
                    emissionIndex++;
                }
                currentIndex++;
            }
            else
            {
                InitParticle(u_ParticleCount);
                u_ParticleCount++;
                emissionIndex++;
            }
            
        }
    }

    barrier();

    // Return early if the particle has a zero lifetime.
    // Note: we allow the particle at the end to always proceed so it can be removed (as this sometimes doesn't happen the first time below)
    if (u_Particles[index].lifeRemaining == 0.0 && index != u_ParticleCount - 1)
        return;

    u_Particles[index].lifeRemaining = max(u_Particles[index].lifeRemaining - u_DeltaTime, 0.0);

    // Only execute if this particle has just died
    if (u_Particles[index].lifeRemaining == 0.0)
    {
        // Remove the current particle, update the particle count to the previous non-zero lifeRemaining index
        if (index == u_ParticleCount - 1)
        {
            for (uint i = u_ParticleCount - 1; i >= 0; i--)
            {
                if (u_Particles[i].lifeRemaining != 0.0)
                {
                    atomicExchange(u_ParticleCount, i + 1);
                    break;
                }
            }
        }
    }
    else
    {
        // Update velocity
        u_Particles[index].velocity.xyz += u_Acceleration.xyz;
        
        // Update position and check for collisions
        if (u_CollisionRadius != 0.0)
        {
            const vec4 clipSpacePosition = u_ViewProjection * vec4(u_Particles[index].position.xyz, 1.0);
            const vec3 ndcPosition = (clipSpacePosition.xyz / clipSpacePosition.w) * 0.5 + 0.5;

            const vec2 screenCoords = ndcPosition.xy;
            const float sceneDepth =  LinearDepth(texture(g_Depth, screenCoords).r);
            const float particleDepth = LinearDepth(ndcPosition.z);

            if (abs(particleDepth - sceneDepth) < u_CollisionRadius)
            {
                // A collision occured
                const float restitution = 0.4;
                const vec3 sceneNormal = texture(g_Normal, screenCoords).rgb;
                u_Particles[index].velocity.xyz = reflect(u_Particles[index].velocity.xyz, sceneNormal) * restitution;
            }
        }
        
        u_Particles[index].position.xyz = u_Particles[index].position.xyz + u_Particles[index].velocity.xyz * u_DeltaTime;
    }
}