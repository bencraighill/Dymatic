// Dymatic Scene Path Tracing Shader

#type compute
#version 430 core
#include Include/Buffers.glslh

#define ENABLE_RAY_INVERSE_DIRECTION
#include Include/Ray.glslh

#define PATH_TRACE_LOCAL_SIZE 16
layout(local_size_x = PATH_TRACE_LOCAL_SIZE, local_size_y = PATH_TRACE_LOCAL_SIZE, local_size_z = 1) in;

layout (set = 0, binding = 0) uniform writeonly image2D u_OutputColor;
layout (set = 0, binding = 1, r32i) uniform writeonly iimage2D u_OutputID;
layout (set = 0, binding = 2) uniform writeonly image2D u_OutputNormal;
layout (set = 0, binding = 3, r32f) uniform writeonly image2D u_OutputDepth;
layout (set = 0, binding = 4, r32i) uniform writeonly iimage2D u_OutputSubmeshIndex;

#define MAX_RAY_DEPTH 4

#define SAMPLES 1
#define SAMPLE_VARIATION 0.001

#define EPSILON 1e-3

#define HIT_TYPE_TRIANGLE       0
#define HIT_TYPE_VOLUME         1
#define HIT_TYPE_POINT_LIGHT    2

// External Material Samplers and Uniforms are defined here by the renderer

// Begin Template
/* MATERIAL_HEADER */
// End Template

struct MaterialPropertiesInput
{
    vec2 TexCoord;
};

// PCG (permuted congruential generator). Thanks to: www.pcg-random.org and www.shadertoy.com/view/XlGcRh
uint NextRandom(inout uint state)
{
	state = state * 747796405 + 2891336453;
	uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

float RandomValue(inout uint state)
{
	return NextRandom(state) / 4294967295.0;
}

// Random value in normal distribution (with mean=0 and sd=1)
float RandomValueNormalDistribution(inout uint state)
{
	// Thanks to https://stackoverflow.com/a/6178290
	float theta = 2 * 3.1415926 * RandomValue(state);
	float rho = sqrt(-2 * log(RandomValue(state)));
	return rho * cos(theta);
}

// Calculate a random direction
vec3 RandomDirection(inout uint state)
{
	// Thanks to https://math.stackexchange.com/a/1585996
	float x = RandomValueNormalDistribution(state);
	float y = RandomValueNormalDistribution(state);
	float z = RandomValueNormalDistribution(state);
	return normalize(vec3(x, y, z));
}

vec2 RandomPointInCircle(inout uint rngState)
{
	float angle = RandomValue(rngState) * 2 * PI;
	vec2 pointOnCircle = vec2(cos(angle), sin(angle));
	return pointOnCircle * sqrt(RandomValue(rngState));
}

vec3 TraceRay(Ray ray, inout uint randomState, out int hitID, out vec3 hitNormal, out vec3 hitPosition, out int hitSubmeshIndex);

bool IntersectTriangle(
    Ray ray, RayTriangle triangle,
    out float distance, out vec3 normal, out vec3 barycentricCoords, out bool isFrontFace
);

bool RayBoundingBox(Ray ray, vec3 boxMin, vec3 boxMax);
float RayBoundingBoxDistance(Ray ray, vec3 boxMin, vec3 boxMax);
bool RayBoundingBoxDistance(Ray ray, vec3 boxMin, vec3 boxMax, out float distanceNear, out float distanceFar);

bool IntersectSphere(Ray ray, vec3 center, float radius, out float distanceNear, out float distanceFar);

vec2 InterpolateTextureCoords(vec3 baryCoords, vec2 texCoord0, vec2 texCoord1, vec2 texCoord2);

float reflectance(float cosine, float refractionIndex);

void GetMaterialProperties(
    uint material, MaterialPropertiesInput Input,
    out vec3 albedoColor, out float specularProbability, out float smoothness, out vec3 emissive, out vec3 specularColor,
    out bool isDielectric, out float refractionIndex
);

void main()
{
    // Get the current fragment's screen coordinates
    const vec2 texCoord = gl_GlobalInvocationID.xy / vec2(u_ScreenDimensions);

    // Transform pixel coordinates to world space to calculate the focus point
    const vec3 focusPointLocal = vec3(texCoord * 2.0 - 1.0, 1.0);
    const vec4 worldPosition = u_InverseViewProjection * vec4(focusPointLocal, 1.0);
    const vec3 focusPoint = worldPosition.xyz / worldPosition.w;

    const vec3 worldCameraPosition = u_ViewPosition.xyz;

    Ray ray;

    // Perform ray-triangle intersection
    int hitID, hitSubmeshIndex;
    vec3 hitNormal, hitPosition;

    // Random Generation
    uint pixelIndex = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
    uint randomState = pixelIndex + uint(u_Time * 719393.0);

    const float DefocusStrength = 10.0;
    const float DivergeStrength = 10.0;

    // Trace rays
    vec3 color = vec3(0.0);
    for (int i = 0; i < SAMPLES; i++)
    {
        //ray.Origin = worldCameraPosition;
        //ray.Direction = normalize(focusPoint - ray.Origin);

        // Calculate ray origin and direction
        vec2 defocusJitter = RandomPointInCircle(randomState) * DefocusStrength / float(u_ScreenDimensions.x);
		ray.Origin = worldCameraPosition + u_Right.xyz * defocusJitter.x + u_Up.xyz * defocusJitter.y;

		vec2 jitter = RandomPointInCircle(randomState) * DivergeStrength / float(u_ScreenDimensions.x);
		vec3 jitteredFocusPoint = focusPoint + u_Right.xyz * jitter.x + u_Up.xyz * jitter.y;
		ray.Direction = normalize(jitteredFocusPoint - ray.Origin);
        
        // Trace
        color += TraceRay(ray, randomState, hitID, hitNormal, hitPosition, hitSubmeshIndex);
    }
    color /= SAMPLES;

    // Calculate the depth
    const vec4 clipPos = u_ViewProjection * vec4(hitPosition, 1.0);
    const vec3 ndcPos = clipPos.xyz / clipPos.w;
    const float depth = ndcPos.z;

    // Write a color value to the output texture
    imageStore(u_OutputColor,   ivec2(gl_GlobalInvocationID.xy), vec4(color, 1.0));
    imageStore(u_OutputID,      ivec2(gl_GlobalInvocationID.xy), ivec4(hitID, 0, 0, 0));
    imageStore(u_OutputNormal,  ivec2(gl_GlobalInvocationID.xy), vec4(hitNormal, 1.0));
    imageStore(u_OutputDepth,   ivec2(gl_GlobalInvocationID.xy), vec4(depth, 0.0, 0.0, 0.0));
    imageStore(u_OutputSubmeshIndex, ivec2(gl_GlobalInvocationID.xy), ivec4(hitSubmeshIndex, 0, 0, 0));
}

shared uint nodeStack[PATH_TRACE_LOCAL_SIZE * PATH_TRACE_LOCAL_SIZE][MAX_BVH_DEPTH + 1];

vec3 TraceRay(Ray worldRay, inout uint randomState,  out int hitID, out vec3 hitNormal, out vec3 hitPosition, out int hitSubmeshIndex)
{
    vec3 accumulatedColor = vec3(0.0);
    vec3 rayColor = vec3(1.0);
    hitID = -1;
    hitSubmeshIndex = -1;
    hitNormal = vec3(0.0);
    hitPosition = worldRay.Origin + worldRay.Direction * u_ZFar;
    Ray localRay;

    worldRay.InverseDirection = 1.0 / worldRay.Direction;
    
    for (uint bounceIndex = 0; bounceIndex < MAX_RAY_DEPTH; bounceIndex++)
    {
        HitRecord record;
        record.ID = -1;
        record.Distance = FLT_MAX;
        record.Material = 0;
        int hitType = -1;
        uint hitIndex;

        float scatteringDistribution;
        vec3 volumeColor;

        // Test all volumes
        for (uint volumeIndex = 0; volumeIndex < u_VolumeCount; volumeIndex++)
        {
            const Volume volume = u_Volumes[volumeIndex];

            float distanceNear, distanceFar;
            if (!RayBoundingBoxDistance(worldRay, volume.min.xyz, volume.max.xyz, distanceNear, distanceFar))
                continue;
            
            if (distanceNear >= record.Distance)
                continue;
            
            const float rayLength = length(worldRay.Direction);
            const float distanceInsideVolume = (distanceFar - distanceNear) * rayLength;

            const float density = volume.scatteringIntensity;
            const float hitDistance = (-1.0 / density) * log(RandomValue(randomState));

            if (hitDistance >= distanceInsideVolume)
                continue;

            const float scatterDistance = distanceNear + hitDistance / rayLength;

            if (scatterDistance >= record.Distance)
                continue;

            hitType = HIT_TYPE_VOLUME;
            scatteringDistribution = volume.scatteringDistribution;
            volumeColor = volume.color.xyz;
            record.Distance = scatterDistance;
            record.Position = worldRay.Origin + worldRay.Direction * record.Distance;
        }

        // Test All Point Lights
        for (uint pointLightIndex = 0; pointLightIndex < u_PointLightCount; pointLightIndex++)
        {
            const PointLight pointLight = u_PointLights[pointLightIndex];

            if (bounceIndex == 0 || pointLight.enabled == 0)
                continue;

            const float centerRadius = 0.5;
            float distanceNear, distanceFar;
            if (IntersectSphere(worldRay, pointLight.position.xyz, centerRadius, distanceNear, distanceFar))
            {
                const float centerDistance = (distanceNear + distanceFar) * 0.5;

                if (centerDistance >= record.Distance)
                    continue;

                hitType = HIT_TYPE_POINT_LIGHT;
                hitIndex = pointLightIndex;

                record.Distance = centerDistance;
            }
        }

        // Test all models and their vertex BVH
        for (uint modelIndex = 0; modelIndex < u_BVHModelCount; modelIndex++)
        {
            const BVHModel model = u_BVHModels[modelIndex];

            // Transform the ray's origin and direction into the local space of the model
            // This allows us to easily render a transformed model without having to transfer
            // every triangle and bounding box in the BVH.
            localRay.Origin = (model.InverseModel * vec4(worldRay.Origin, 1.0)).xyz;
            localRay.Direction = (model.InverseModel * vec4(worldRay.Direction, 0.0)).xyz;
            localRay.InverseDirection = 1.0 / localRay.Direction;

            bool modelHit = false;

            uint traversalCount = 0;

            for (uint submeshIndex = 0; submeshIndex < model.SubmeshCount; submeshIndex++)
            {
                BVHSubmesh submesh = u_BVHSubmeshes[model.SubmeshIndex + submeshIndex];

                if (RayBoundingBoxDistance(localRay, submesh.Min.xyz, submesh.Max.xyz) >= record.Distance)
                    continue;

                uint stackIndex = 0;

                // Begin by pushing the root node for the model's BVH onto the stack
                nodeStack[gl_LocalInvocationIndex][stackIndex++] = submesh.NodeIndex;

                while (stackIndex > 0)
                {
                    traversalCount++;

                    uint nodeIndex = nodeStack[gl_LocalInvocationIndex][--stackIndex];
                    const BVHNode node = u_BVHNodes[nodeIndex];

                    if (RayBoundingBoxDistance(localRay, node.Min, node.Max) == FLT_MAX)
                        continue;

                    // Check if we have reached a leaf node (no children), in which case we test triangles.
                    if (node.ChildIndex == 0)
                    {
                        // uint t = nodeIndex;
                        // return vec3(RandomValue(t), RandomValue(t), RandomValue(t));

                        // Itterate over all triangles inside the node
                        for (uint triangleIndex = node.TriangleIndex; triangleIndex < node.TriangleIndex + node.TriangleCount; triangleIndex++)
                        {
                            float distance;
                            vec3 normal, barycentricCoords;
                            bool isFrontFace;

                            // Check if an intersection with the triangle occurs, updating the hit record if so.
                            if (IntersectTriangle(localRay, u_Triangles[triangleIndex], distance, normal, barycentricCoords, isFrontFace))
                            {
                                if (distance < record.Distance && distance >= 0.0)
                                {
                                    hitType = HIT_TYPE_TRIANGLE;
                                    modelHit = true;

                                    record.Distance = distance;
                                    record.Position = worldRay.Origin + worldRay.Direction * record.Distance;
                                    record.Normal = normal;
                                    record.Material = submesh.Material;
                                    record.ID = model.EntityID;
                                    record.FrontFace = isFrontFace;

                                    const vec2 texCoord0 = vec2(u_Triangles[triangleIndex].Position[0].w, u_Triangles[triangleIndex].Normal[0].w);
                                    const vec2 texCoord1 = vec2(u_Triangles[triangleIndex].Position[1].w, u_Triangles[triangleIndex].Normal[1].w);
                                    const vec2 texCoord2 = vec2(u_Triangles[triangleIndex].Position[2].w, u_Triangles[triangleIndex].Normal[2].w);
                                    record.TexCoord = InterpolateTextureCoords(barycentricCoords, texCoord0, texCoord1, texCoord2);

                                    if (bounceIndex == 0)
                                        hitSubmeshIndex = int(submeshIndex);
                                }
                            }
                        }
                    }
                    else
                    {
                        // Otherwise, push children onto the stack to be tested
                        uint childIndexA = node.ChildIndex + 1;
                        uint childIndexB = node.ChildIndex + 0;

                        BVHNode childA = u_BVHNodes[childIndexA];
                        BVHNode childB = u_BVHNodes[childIndexB];

                        float distanceA = RayBoundingBoxDistance(localRay, childA.Min, childA.Max);
                        float distanceB = RayBoundingBoxDistance(localRay, childB.Min, childB.Max);

                        // Ensure the closest child is looked at first (so we quickly discard later result if an intersection occurs)
                        bool isNearestA = distanceA < distanceB;
                        float distanceNear = isNearestA ? distanceA : distanceB;
                        float distanceFar = isNearestA ? distanceB : distanceA;
                        uint childIndexNear = isNearestA ? childIndexA : childIndexB;
                        uint childIndexFar = isNearestA ? childIndexB : childIndexA;

                        if (distanceFar  < record.Distance) nodeStack[gl_LocalInvocationIndex][stackIndex++] = childIndexFar;
                        if (distanceNear < record.Distance) nodeStack[gl_LocalInvocationIndex][stackIndex++] = childIndexNear;
                    }
                }
            }

            // If we have intersected this model convert it's normal back to worldspace from local ray space
            if (modelHit)
            {
                record.Normal = normalize(model.Model * vec4(record.Normal, 0.0)).xyz;

                //const float hottness = traversalCount / 150.0;
                //return hottness > 1.0 ? vec3(1.0, 0.0, 0.0) : vec3(hottness);
            }
        }

        // Determine what to do next based on if we hit an object
        if (hitType == HIT_TYPE_VOLUME)
        {
            worldRay.Origin = record.Position;
            worldRay.Direction = mix(worldRay.Direction, RandomDirection(randomState), scatteringDistribution);
            rayColor *= volumeColor;
        }
        else if (hitType == HIT_TYPE_POINT_LIGHT)
        {
            const PointLight pointLight = u_PointLights[hitIndex];
            const float distance2 = record.Distance * record.Distance;
            accumulatedColor += (pointLight.color.rgb * pointLight.intensity) / distance2;
        }
        else if (hitType == HIT_TYPE_TRIANGLE)
        {
            if (bounceIndex == 0)
            {
                hitID = record.ID;
                hitNormal = record.Normal;
                hitPosition = record.Position;
            }

            // Material Properties
            vec3 albedoColor, emissive, specularColor;
            float specularProbability, smoothness, refractionIndex;
            bool isDielectric;
            const MaterialPropertiesInput Input = MaterialPropertiesInput(record.TexCoord);

            GetMaterialProperties(
                record.Material, Input,
                albedoColor, specularProbability, smoothness, emissive, specularColor,
                isDielectric, refractionIndex
            );

            worldRay.Origin = record.Position + EPSILON * record.Normal;

            if (isDielectric)
            {
                float refractionRatio = record.FrontFace ? (1.0 / refractionIndex) : refractionIndex;
                vec3 unitDirection = normalize(worldRay.Direction);
                float cosTheta = min(dot(-unitDirection, record.Normal), 1.0);
                float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

                bool cannotRefract = refractionRatio * sinTheta > 1.0;
                vec3 direction;

                if (cannotRefract || reflectance(cosTheta, refractionRatio) > RandomValue(randomState))
                {
                    direction = reflect(unitDirection, record.Normal);
                }
                else
                {
                    direction = refract(unitDirection, record.Normal, 0.5);
                }

                worldRay.Direction = normalize(direction);
            }
            else
            {
                // Calculate new ray position and direction
                bool isSpecularBounce = specularProbability >= RandomValue(randomState);

                vec3 diffuseDir = normalize(record.Normal + RandomDirection(randomState));
			    vec3 specularDir = reflect(worldRay.Direction, record.Normal);
			    worldRay.Direction = normalize(mix(diffuseDir, specularDir, isSpecularBounce ? smoothness : 0.0));

                // Update light calculations
			    accumulatedColor += emissive * rayColor;
			    rayColor *= isSpecularBounce ? specularColor : albedoColor;

                // Random early exit if ray colour is nearly 0 (can't contribute much to final result)
			    float p = max(rayColor.r, max(rayColor.g, rayColor.b));
			    if (RandomValue(randomState) >= p)
			    	break;

			    rayColor *= 1.0 / p;
            }
        }
        else
        {
            vec3 skyColor;

            // Sky Light
            if (u_UsingSkyLight == 1)
            {
                skyColor = texture(u_EnvironmentMap, worldRay.Direction).rgb * u_SkyLightIntensity;
            }
            else
            {
                //vec3 unitDirection = normalize(worldRay.Direction);
                //float t = 0.5 * (unitDirection.y + 1.0);
                //skyColor = (1.0 - t) * vec3(0.8, 0.9, 1.0) + t * vec3(0.5, 0.7, 1.0);
                skyColor = vec3(0.0);
            }

            // Directional Light (don't show lighting in the sky i.e. first bounce)
            if (bounceIndex != 0 && u_UsingDirectionalLight == 1)
            {
                const vec3 lightDir = normalize(-vec3(u_DirectionalLight.direction));
                const float diff = max(dot(lightDir, worldRay.Direction), 0.0);

                skyColor += u_DirectionalLight.color.rgb * u_DirectionalLight.color.a * diff;
            }

            accumulatedColor += skyColor * rayColor;

            break;
        }
    }

    return accumulatedColor;
}

bool IntersectTriangle(
    Ray ray, RayTriangle triangle,
    out float distance, out vec3 normal, out vec3 barycentricCoords, out bool isFrontFace)
{
    vec3 edgeAB = triangle.Position[1].xyz - triangle.Position[0].xyz;
	vec3 edgeAC = triangle.Position[2].xyz - triangle.Position[0].xyz;
	vec3 normalVector = cross(edgeAB, edgeAC);
	vec3 ao = ray.Origin - triangle.Position[0].xyz;
	vec3 dao = cross(ao, ray.Direction);

	float determinant = -dot(ray.Direction, normalVector);
	float invDet = 1 / determinant;

	// Calculate dst to triangle & barycentric coordinates of intersection point
	float dst = dot(ao, normalVector) * invDet;
	float u = dot(edgeAC, dao) * invDet;
	float v = -dot(edgeAB, dao) * invDet;
	float w = 1.0 - u - v;

    // Initialize hit info
    normal = normalize(triangle.Normal[0].xyz * w + triangle.Normal[1].xyz * u + triangle.Normal[2].xyz * v);
    distance = dst;
    barycentricCoords = vec3(w, u, v);
    isFrontFace = dot(ray.Direction, normal) < 0.0;

    // Note: To disable back faces, just remove abs()
    return abs(determinant) >= 1E-8 && dst >= 0 && u >= 0 && v >= 0 && w >= 0;
}

#if 0
bool IntersectTriangle(
    Ray ray, RayTriangle triangle,
    out float distance, out vec3 normal
)
{
    vec3 edge1 = triangle.Position[1].xyz - triangle.Position[0].xyz;
    vec3 edge2 = triangle.Position[2].xyz - triangle.Position[0].xyz;
    vec3 pvec = cross(ray.Direction, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < EPSILON)
        return false;

    float invDet = 1.0 / det;
    vec3 tvec = ray.Origin - triangle.Position[0].xyz;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.Direction, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0)
        return false;
        
    distance = dot(edge2, qvec) * invDet;
    if (abs(distance) < EPSILON)
        return false;
    
    // Interpolating the normal using barycentric coordinates
    normal = normalize(triangle.Normal[0].xyz * (1 - u - v) + triangle.Normal[1].xyz * u + triangle.Normal[2].xyz * v);
    
    // Original normal code (does not use vertex normal data):
    //normal = normalize(cross(edge1, edge2));
    
    return true;
}
#endif

bool RayBoundingBox(Ray ray, vec3 boxMin, vec3 boxMax)
{
    return RayBoundingBoxDistance(ray, boxMin, boxMax) != FLT_MAX;
}

float RayBoundingBoxDistance(Ray ray, vec3 boxMin, vec3 boxMax)
{
    float distanceNear, distanceFar;
    RayBoundingBoxDistance(ray, boxMin, boxMax, distanceNear, distanceFar);

    const bool didHit = distanceFar >= distanceNear && distanceFar > 0.0;
    return didHit ? distanceNear : FLT_MAX;
}

bool RayBoundingBoxDistance(Ray ray, vec3 boxMin, vec3 boxMax, out float distanceNear, out float distanceFar)
{
    vec3 tMin = (boxMin - ray.Origin) * ray.InverseDirection;
    vec3 tMax = (boxMax - ray.Origin) * ray.InverseDirection;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    distanceFar = min(min(t2.x, t2.y), t2.z);
    distanceNear = max(max(t1.x, t1.y), t1.z);

    return distanceFar >= distanceNear && distanceFar > 0.0;
}

bool IntersectSphere(Ray ray, vec3 center, float radius, out float distanceNear, out float distanceFar)
{
    vec3 L = center - ray.Origin;
    float tca = dot(L, ray.Direction);
    float d2 = dot(L, L) - tca * tca;
    float radius2 = radius * radius;
    
    // If the distance from the sphere center to the ray is greater than the radius, there is no intersection
    if (d2 > radius2)
        return false;
    
    float thc = sqrt(radius2 - d2);
    distanceNear = tca - thc;
    distanceFar = tca + thc;
    
    return true;
}

vec2 InterpolateTextureCoords(vec3 barycentricCoords, vec2 texCoord0, vec2 texCoord1, vec2 texCoord2)
{
    return barycentricCoords.x * texCoord0 + barycentricCoords.y * texCoord1 + barycentricCoords.z * texCoord2;
}

// Calculate reflectance using Schlick's approximation
float reflectance(float cosine, float refractionIndex)
{
    float r0 = (1.0 - refractionIndex) / (1.0 + refractionIndex);
    r0 = r0 * r0;
    
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

void GetMaterialProperties(
    uint material, MaterialPropertiesInput Input,
    out vec3 albedoColor, out float specularProbability, out float smoothness, out vec3 emissive, out vec3 specularColor,
    out bool isDielectric, out float refractionIndex
)
{
    // Default Properties
    albedoColor = vec3(0.8);
    specularProbability = 1.0;
    smoothness = 0.0;
    emissive = vec3(0.0);
    specularColor = vec3(0.8);
    isDielectric = false;
    refractionIndex = 1.5;

    if (material == 0)
        return;

    // External Material Properties and logic are defined here by the renderer

    // Begin Template
    /* MATERIAL_PROPERTIES */
    // End Template
}