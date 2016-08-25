
uniform mat4 P;

//uniform mat4 MV;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat4 uProjMatrix;
uniform vec3 uLightPos;
uniform vec3 uLightCol;
uniform vec3 UaColor;
uniform vec3 UdColor;
uniform vec3 UsColor;
uniform float Ushine;
uniform int wall;

varying vec4 varyingLight;
varying vec4 varyingHalf;
varying vec4 vANor;
void main()
{

    vec3 vCo = uLightCol*pow(max(0.0,dot(vANor,varyingHalf)),Ushine)*UsColor
    +uLightCol*max(0.0,dot(vANor,normalize(varyingLight)))*UdColor
    +uLightCol*UaColor;

    gl_FragColor = vec4(vCo.r, vCo.g, vCo.b, 1.0);
//    gl_FragColor = vec4(UsColor.r, UsColor.g, UsColor.b, 1.0);

}
