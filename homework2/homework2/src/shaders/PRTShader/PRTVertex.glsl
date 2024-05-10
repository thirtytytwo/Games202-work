attribute vec3 aVertexPosition;
attribute vec3 aNormalPosition;
attribute mat3 aPrecomputeLT;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat3 uPrecomputeLR;
uniform mat3 uPrecomputeLG;
uniform mat3 uPrecomputeLB;

varying highp vec3 vNormal;
varying highp vec3 vColor;

vec3 ComputeSH(mat3 precomputeLR, mat3 precomputLG, mat3 precomputeLB, mat3 precomputeLT){
    vec3 result = vec3(uPrecomputeLR[0][0], uPrecomputeLG[0][0], uPrecomputeLB[0][0]) * aPrecomputeLT[0][0] + 
                vec3(uPrecomputeLR[0][1], uPrecomputeLG[0][1], uPrecomputeLB[0][1]) * aPrecomputeLT[0][1] +
                vec3(uPrecomputeLR[0][2], uPrecomputeLG[0][2], uPrecomputeLB[0][2]) * aPrecomputeLT[0][2] +
                vec3(uPrecomputeLR[1][0], uPrecomputeLG[1][0], uPrecomputeLB[1][0]) * aPrecomputeLT[1][0] +
                vec3(uPrecomputeLR[1][1], uPrecomputeLG[1][1], uPrecomputeLB[1][1]) * aPrecomputeLT[1][1] +
                vec3(uPrecomputeLR[1][2], uPrecomputeLG[1][2], uPrecomputeLB[1][2]) * aPrecomputeLT[1][2] +
                vec3(uPrecomputeLR[2][0], uPrecomputeLG[2][0], uPrecomputeLB[2][0]) * aPrecomputeLT[2][0] +
                vec3(uPrecomputeLR[2][1], uPrecomputeLG[2][1], uPrecomputeLB[2][1]) * aPrecomputeLT[2][1] +
                vec3(uPrecomputeLR[2][2], uPrecomputeLG[2][2], uPrecomputeLB[2][2]) * aPrecomputeLT[2][2];

    return result;
}
void main(void) {

    vNormal = (uModelMatrix * vec4(aNormalPosition, 0.0)).xyz;

    vColor = ComputeSH(uPrecomputeLR, uPrecomputeLG, uPrecomputeLB, aPrecomputeLT);

    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aVertexPosition, 1.0);
}