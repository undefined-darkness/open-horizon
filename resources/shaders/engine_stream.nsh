@uniform param "param"

@all
varying vec4 color;

@vertex

uniform vec4 param; //radius dist afterburner

void main()
{
    vec3 tc = gl_MultiTexCoord0.xyz;
    color = mix(vec4(0.18, 0.4, 0.6, 1.8), vec4(0.87, 0.35, 0.24, 0.0), tc.y);

    vec4 pos = gl_Vertex;
    pos.xy *= param.x;
    pos.x += tc.z * param.y;
    pos.z *= param.z;
    gl_Position = gl_ModelViewProjectionMatrix * pos;
}

@fragment

void main()
{
    gl_FragColor = color;
}
