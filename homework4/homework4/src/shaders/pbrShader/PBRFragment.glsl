#ifdef GL_ES
precision mediump float;
#endif

uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform vec3 uLightRadiance;
uniform vec3 uLightDir;

uniform sampler2D uAlbedoMap;
uniform float uMetallic;
uniform float uRoughness;
uniform sampler2D uBRDFLut;
uniform samplerCube uCubeTexture;

varying highp vec2 vTextureCoord;
varying highp vec3 vFragPos;
varying highp vec3 vNormal;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    // TODO: To calculate GGX NDF here
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = dot(N,H);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    // TODO: To calculate Smith G1 here
    //突然发现这里k的计算跟公式里面不太一样
    float a = roughness;
    float k = (a * a) / 2.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    // TODO: To calculate Smith G here
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float G1 = GeometrySchlickGGX(NdotL, roughness);
    float G2 = GeometrySchlickGGX(NdotV, roughness);
    return G1 * G2;
}

vec3 fresnelSchlick(vec3 F0, vec3 V, vec3 H)
{
    float VdotH = dot(V, H);
    vec3 result = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    return result;
}

void main(void) {
    vec3 albedo = pow(texture2D(uAlbedoMap, vTextureCoord).rgb, vec3(2.2));

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vFragPos);
    float NdotV = max(dot(N, V), 0.0);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, uMetallic);

    vec3 Lo = vec3(0.0);

    vec3 L = normalize(uLightDir);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0); 

    vec3 radiance = uLightRadiance;

    float NDF = DistributionGGX(N, H, uRoughness);   
    float G   = GeometrySmith(N, V, L, uRoughness); 
    vec3 F = fresnelSchlick(F0, V, H);

    //G项是用来压用来归一化的分母的，G里面的Dot需要钳制，不然会计算不正确，导致压不下来
    //特别是本来分母偏高的可能就是视线和法线夹角高或者光线和法线夹角高的情况，如果不钳制，夹角因为高，Dot出来的值可能为负数
    //导致根本在该压的地方压不住
    vec3 numerator    = NDF * G * F; 
    float denominator = max((4.0 * NdotL * NdotV), 0.001);
    vec3 BRDF = numerator / denominator;

    Lo += BRDF * radiance * NdotL;
    vec3 color = Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); 
    gl_FragColor = vec4(color, 1.0);
}