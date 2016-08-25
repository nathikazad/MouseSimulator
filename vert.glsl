attribute vec4 aPosition;
attribute vec3 aNormal;
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
    gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * aPosition;
    varyingLight = normalize(vec4(uLightPos,0)-uViewMatrix*uModelMatrix*aPosition);
    vec4 lightVector = vec4(uLightPos,1)-uViewMatrix*uModelMatrix*aPosition;
    if(wall==0)
        vANor = vec4(aNormal,0);
    else
    {
        vec4 t_normal=vec4(aNormal,0)*vec4(-1,-1,-1,1);
        if(dot(t_normal,lightVector) > dot(vec4(aNormal,0),lightVector))
            vANor = vec4(aNormal,0)*vec4(-1,-1,-1,1);
        else
            vANor = vec4(aNormal,0);
    }
    
    vec4 cameraVector = vec4(0,0,0,1)-uViewMatrix*uModelMatrix*aPosition;
    varyingHalf=normalize(lightVector+cameraVector);
}


