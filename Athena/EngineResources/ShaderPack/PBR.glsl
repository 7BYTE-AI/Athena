#type VERTEX_SHADER
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) out VertexOutput Output;


layout(std140, binding = CAMERA_BUFFER_BINDER) uniform CameraData
{
	mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    mat4 RotationViewMatrix;
    vec4 Position;
    float NearClip;
	float FarClip;
} u_Camera;

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 Transform;
    int ID;
    bool IsAnimated;
} u_Entity;

layout(std430, binding = BONES_BUFFER_BINDER) readonly buffer BoneTransforms
{
    mat4 g_Bones[MAX_NUM_BONES];
};


void main()
{
    mat4 fullTransform = u_Entity.Transform;

    if(bool(u_Entity.IsAnimated))
    {
        mat4 boneTransform = g_Bones[a_BoneIDs[0]] * a_Weights[0];
        for(int i = 1; i < MAX_NUM_BONES_PER_VERTEX; ++i)
        {
            boneTransform += g_Bones[a_BoneIDs[i]] * a_Weights[i];
        }

       fullTransform *= boneTransform;
    }

    vec4 transformedPos = fullTransform * vec4(a_Position, 1);

    gl_Position = u_Camera.ProjectionMatrix * u_Camera.ViewMatrix * transformedPos;

    Output.WorldPos = vec3(transformedPos);
    Output.TexCoord = a_TexCoord;
    Output.Normal = normalize(vec3(fullTransform * vec4(a_Normal, 0)));

    vec3 T = normalize(vec3(fullTransform * vec4(a_Tangent, 0)));
    vec3 N = Output.Normal;
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(vec3(fullTransform * vec4(a_Bitangent, 0)));
    Output.TBN = mat3(T, B, N);
}


#type FRAGMENT_SHADER
#version 460 core
#extension GL_ARB_shading_language_include : require

#include "/PoissonDisk.glsl"

#define SOFT_SHADOW_SAMPLES 16

layout(location = 0) out vec4 out_Color;

struct VertexOutput
{
    vec3 WorldPos;
	vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
};

layout (location = 0) in VertexOutput Input;


layout(std140, binding = CAMERA_BUFFER_BINDER) uniform CameraData
{
	mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    mat4 RotationViewMatrix;
    vec4 Position;
    float NearClip;
	float FarClip;
} u_Camera;

layout(std140, binding = ENVMAP_BUFFER_BINDER) uniform EnvMapData
{
	float LOD;
    float Intensity;
} u_EnvMapData;

layout(std140, binding = ENTITY_BUFFER_BINDER) uniform EntityData
{
    mat4 Transform;
    int ID;
    bool IsAnimated;
} u_Entity;

layout(std140, binding = MATERIAL_BUFFER_BINDER) uniform MaterialData
{
    vec4 Albedo;
    float Roughness;
    float Metalness;
    float Emission;

	int EnableAlbedoMap;
	int EnableNormalMap;
	int EnableRoughnessMap;
	int EnableMetalnessMap;
    int EnableAmbientOcclusionMap;
} u_Material;

struct Split
{
    vec2 LightFrustumPlanes;
    float SplitDepth;
    float _Padding;
};

layout(std140, binding = SHADOWS_BUFFER_BINDER) uniform ShadowsData
{
    mat4 LightViewProjMatrices[SHADOW_CASCADES_COUNT];
    mat4 LightViewMatrices[SHADOW_CASCADES_COUNT];
    Split CascadeSplits[SHADOW_CASCADES_COUNT];
	float MaxDistance;
	float FadeOut;
	float LightSize;
	bool SoftShadows;
} u_Shadows;

struct DirectionalLight
{
    vec4 Color;
    vec3 Direction;
    float Intensity;
};

struct PointLight
{
    vec4 Color;
    vec3 Position;
    float Intensity;
    float Radius;
    float FallOff;
};

layout(std430, binding = LIGHT_BUFFER_BINDER) readonly buffer LightBuffer
{
    DirectionalLight g_DirectionalLightBuffer[MAX_DIRECTIONAL_LIGHT_COUNT];
    int g_DirectionalLightCount;

    PointLight g_PointLightBuffer[MAX_POINT_LIGHT_COUNT];
    int g_PointLightCount;
};


layout(binding = ALBEDO_MAP_BINDER)     uniform sampler2D u_AlbedoMap;
layout(binding = NORMAL_MAP_BINDER)     uniform sampler2D u_NormalMap;
layout(binding = ROUGHNESS_MAP_BINDER)  uniform sampler2D u_RoughnessMap;
layout(binding = METALNESS_MAP_BINDER)  uniform sampler2D u_MetalnessMap;
layout(binding = AO_MAP_BINDER)         uniform sampler2D u_AmbientOcclusionMap;

layout(binding = ENVIRONMENT_MAP_BINDER) uniform samplerCube u_EnvironmentMap;
layout(binding = IRRADIANCE_MAP_BINDER) uniform samplerCube u_IrradianceMap;
layout(binding = BRDF_LUT_BINDER)       uniform sampler2D u_BRDF_LUT;

layout(binding = SHADOW_MAP_BINDER)     uniform sampler2DArray u_ShadowMap;
layout(binding = PCF_SAMPLER_BINDER)    uniform sampler2DArrayShadow u_ShadowMapPCF;


float DistributionGGX(vec3 normal, vec3 halfWay, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float normalDotHalfWay  = max(dot(normal, halfWay), 0.0);
    float normalDotHalfWay2 = normalDotHalfWay * normalDotHalfWay;
	
    float numerator = a2;
    float denominator = (normalDotHalfWay2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
	
    return numerator / denominator;
}

float GeometrySchlickGGX(float normalDotView, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float numerator = normalDotView;
    float denominator = normalDotView * (1.0 - k) + k;
	
    return numerator / denominator;
}

float GeometrySmith(vec3 normal, vec3 view, vec3 lightDir, float roughness)
{
    float normalDotView = max(dot(normal, view), 0.0);
    float normalDotLightDir = max(dot(normal, lightDir), 0.0);
    float ggx2 = GeometrySchlickGGX(normalDotView, roughness);
    float ggx1 = GeometrySchlickGGX(normalDotLightDir, roughness);
	
    return ggx1 * ggx2;
}

vec3 FresnelShlick(float cosHalfWayAndView, vec3 reflectivityAtZeroIncidence)
{
    return reflectivityAtZeroIncidence + (1.0 - reflectivityAtZeroIncidence) * pow(clamp(1.0 - cosHalfWayAndView, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosHalfWayAndView, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosHalfWayAndView, 0.0, 1.0), 5.0);
}  

vec3 LightContribution(vec3 lightDirection, vec3 lightRadiance, vec3 normal, vec3 viewVector, vec3 albedo, float metalness, float roughness, vec3 reflectivityAtZeroIncidence)
{
    vec3 negativeLightDir = -lightDirection;
    vec3 halfWayVector = normalize(viewVector + negativeLightDir);

    float NDF = DistributionGGX(normal, halfWayVector, roughness);
    float G = GeometrySmith(normal, viewVector, negativeLightDir, roughness);
    vec3 F = FresnelShlick(max(dot(halfWayVector, viewVector), 0.0), reflectivityAtZeroIncidence);

    vec3 reflectedLight = F;
    vec3 absorbedLight = vec3(1.0) - reflectedLight;
        
    absorbedLight *= 1.0 - metalness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewVector), 0.0) * max(dot(normal, negativeLightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float normalDotLightDir = max(dot(normal, negativeLightDir), 0.0);

    return (absorbedLight * albedo / PI + specular) * lightRadiance * normalDotLightDir;
}
 
float ShadowFading(float distanceFromCamera)
{
    float fade = 1.0;
    if(distanceFromCamera < u_Shadows.MaxDistance)
    {
        fade = clamp(smoothstep(0.0, 1.0, (distanceFromCamera - (u_Shadows.MaxDistance - u_Shadows.FadeOut)) / u_Shadows.FadeOut), 0.0, 1.0);
    }

    return fade;
}

float SamplePCF(vec2 uv, float depthBiased, int cascade)
{
    return texture(u_ShadowMapPCF, vec4(uv, cascade, depthBiased));
}

float SampleHard(vec2 uv, float depthBiased, int cascade)
{
    float shadowMapDepth = texture(u_ShadowMap, vec3(uv, cascade)).r;
    if(shadowMapDepth < depthBiased)
        return 0.0;

    return 1.0;
}

vec2 SamplePoissonDisk(int i)
{
    #if (SOFT_SHADOW_SAMPLES == 16)
        return PoissonDisk16[i];
    #elif (SOFT_SHADOW_SAMPLES == 32)
        return PoissonDisk32[i];
    #elif (SOFT_SHADOW_SAMPLES == 64)
        return PoissonDisk64[i];
    #endif
}

void FindBlocker(out float avgBlockerDepth, out int numBlockers, vec2 uv, float depthBiased, float searchWidth, int cascade)
{
    float blockerSum = 0;
    numBlockers = 0;

    for(int i = 0; i < SOFT_SHADOW_SAMPLES; ++i)
    {
        vec2 offset = SamplePoissonDisk(i) * searchWidth;
        float shadowMapDepth = texture(u_ShadowMap, vec3(uv.xy + offset, cascade)).r;
        if(shadowMapDepth < depthBiased)
        {
            blockerSum += shadowMapDepth;
            numBlockers++;
        }
    }

    avgBlockerDepth = blockerSum / numBlockers;
}

float PenumbraSize(float zReceiver, float zBlocker)
{
    return (zReceiver - zBlocker) / zBlocker;
}

float PCF_Filter(vec2 uv, float depthBiased, float filterRadiusUV, int cascade)
{
    float sum = 0.0;
    for(int i = 0; i < SOFT_SHADOW_SAMPLES; ++i)
    {
        vec2 offset = SamplePoissonDisk(i) * filterRadiusUV;
        sum += SamplePCF(uv.xy + offset, depthBiased, cascade);
    }

    return sum / SOFT_SHADOW_SAMPLES;   
}

float SoftShadow(vec3 projCoords, float bias, int cascade)
{
    vec4 viewPos = u_Shadows.LightViewMatrices[cascade] * vec4(Input.WorldPos, 1.0);
    float zReceiver = -viewPos.z / viewPos.w;

    float lightNearPlane = u_Shadows.CascadeSplits[cascade].LightFrustumPlanes.x;
    float lightFarPlane = u_Shadows.CascadeSplits[cascade].LightFrustumPlanes.y;
    float lightSize = u_Shadows.LightSize;

    float depthBiased = projCoords.z - bias;

    // STEP 1: blocker search 
    float searchWidth = lightSize * (zReceiver - lightNearPlane) / zReceiver;
    float avgBlockerDepth = 0;
    int numBlockers = 0;
    FindBlocker(avgBlockerDepth, numBlockers, projCoords.xy, depthBiased, searchWidth, cascade);

    numBlockers -= 1;
    if(numBlockers < 1)
        return 1.0;

    // STEP 2: penumbra size
    // Convert to view space
    avgBlockerDepth = lightFarPlane * lightNearPlane / (lightFarPlane - avgBlockerDepth * (lightFarPlane - lightNearPlane));
    float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
    float filterRadiusUV =  penumbraRatio * lightSize * lightNearPlane / zReceiver;

    // STEP 3: filtering 
    return PCF_Filter(projCoords.xy, projCoords.z - bias, filterRadiusUV, cascade);
}

float HardShadow(vec3 projCoords, float bias, int cascade)
{
    float depthBiased = projCoords.z - bias;

    return SamplePCF(projCoords.xy, depthBiased, cascade);
}

float ComputeCascadedShadow(vec3 normal, vec3 lightDir, int cascade, float fading)
{
    if(fading == 1.0)
        return 0.0;

    vec4 lightSpacePosition = u_Shadows.LightViewProjMatrices[cascade] * vec4(Input.WorldPos, 1.0);
    vec3 projCoords = 0.5 * lightSpacePosition.xyz / lightSpacePosition.w + 0.5;

    if(projCoords.z > 1.0) return 0.0;

    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadowOcclusion;
    if(u_Shadows.SoftShadows)
        shadowOcclusion = SoftShadow(projCoords, bias, cascade);
    else
        shadowOcclusion = HardShadow(projCoords, bias, cascade);

    return (1 - fading) * (1 - shadowOcclusion);
}


void main()
{
    ////////////////// PBR TEXTURES //////////////////
    vec4 albedo = u_Material.Albedo;
    if(bool(u_Material.EnableAlbedoMap))
        albedo *= texture(u_AlbedoMap, Input.TexCoord);

    vec3 normal;
    if(bool(u_Material.EnableNormalMap))
    {
        normal = texture(u_NormalMap, Input.TexCoord).rgb;
        normal = normal * 2 - 1;
        normal = Input.TBN * normal;
    }
    else
    {
        normal = Input.Normal;
    }

    float roughness = bool(u_Material.EnableRoughnessMap) ? texture(u_RoughnessMap, Input.TexCoord).r : u_Material.Roughness;
    float metalness = bool(u_Material.EnableMetalnessMap) ? texture(u_MetalnessMap, Input.TexCoord).r : u_Material.Metalness;
    float emission = u_Material.Emission;
    float ambientOcclusion = bool(u_Material.EnableAmbientOcclusionMap) ? texture(u_AmbientOcclusionMap, Input.TexCoord).r : 1.0;

    vec3 reflectivityAtZeroIncidence = vec3(0.04);
    reflectivityAtZeroIncidence = mix(reflectivityAtZeroIncidence, albedo.rgb, metalness);

    vec3 viewVector = normalize(u_Camera.Position.xyz - Input.WorldPos);

    //////////////////////// LIGHTS ///////////////////////
    float distanceFromCamera = distance(u_Camera.Position.xyz, Input.WorldPos);
    float shadowFade = ShadowFading(distanceFromCamera);

    vec3 totalIrradiance = vec3(0.0);

    ////////////////// DIRECTIONAL LIGHTS //////////////////
    vec4 worldPosViewSpace = u_Camera.ViewMatrix * vec4(Input.WorldPos, 1.0);
    float depthValue = abs(worldPosViewSpace.z);

    int cascade = SHADOW_CASCADES_COUNT;
    for(int i = 0; i < SHADOW_CASCADES_COUNT; ++i)
    {
        if(depthValue < u_Shadows.CascadeSplits[i].SplitDepth)
        {
            cascade = i;
            break;
        }
    }

    for(int i = 0; i < g_DirectionalLightCount; ++i)
    {
        vec3 lightDirection = normalize(g_DirectionalLightBuffer[i].Direction);
        vec3 lightRadiance = vec3(g_DirectionalLightBuffer[i].Color) * g_DirectionalLightBuffer[i].Intensity;

        float shadow = ComputeCascadedShadow(normal, -lightDirection, cascade, shadowFade);

        if(shadow < 1.0)
            totalIrradiance += (1 - shadow) * LightContribution(lightDirection, lightRadiance, normal, viewVector, albedo.rgb, metalness, roughness, reflectivityAtZeroIncidence);
    }

    ////////////////// POINT LIGHTS //////////////////
    for(int i = 0; i < g_PointLightCount; ++i)
    {
        float dist = length(Input.WorldPos - g_PointLightBuffer[i].Position);
        vec3 lightDirection = (Input.WorldPos - g_PointLightBuffer[i].Position) / dist;
        float factor = dist / g_PointLightBuffer[i].Radius;

        float attenuation = 0.0;
        if(factor < 1.0)
        {
            float factor2 = factor * factor;
            attenuation = (1.0 - factor2) * (1.0 - factor2) / (1 + g_PointLightBuffer[i].FallOff * factor);
            attenuation = clamp(attenuation, 0.0, 1.0);
        }

        vec3 lightRadiance = g_PointLightBuffer[i].Color.rgb * g_PointLightBuffer[i].Intensity * attenuation;

        totalIrradiance += LightContribution(lightDirection, lightRadiance, normal, viewVector, albedo.rgb, metalness, roughness, reflectivityAtZeroIncidence);
    }

    ////////////////// ENVIRONMENT MAP LIGHTNING //////////////////
    float NdotV = max(dot(normal, viewVector), 0.0);

    vec3 reflectedLight = FresnelSchlickRoughness(NdotV, reflectivityAtZeroIncidence, roughness); 
    vec3 absorbedLight = 1.0 - reflectedLight;
    absorbedLight *= 1.0 - metalness;

    vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
    vec3 diffuseIBL = absorbedLight * irradiance;

    vec3 reflectedVec = reflect(-viewVector, normal); 

    vec3 envMapReflectedColor = textureLod(u_EnvironmentMap, reflectedVec, roughness * MAX_SKYBOX_MAP_LOD).rgb;  
    vec2 envBRDF = texture(u_BRDF_LUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = envMapReflectedColor * (reflectedLight * envBRDF.x + envBRDF.y);


    ////////////////// MAIN COLOR //////////////////
    vec3 ambient = (diffuseIBL * albedo.rgb + specularIBL) * ambientOcclusion * u_EnvMapData.Intensity;
    vec3 color = (ambient + totalIrradiance) + albedo.rgb * emission;

    out_Color = vec4(color, albedo.a);
}