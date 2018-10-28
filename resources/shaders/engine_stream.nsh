@uniform param "param"
@uniform vector_l "vector l"
@uniform vector_r "vector r"

@all
varying vec4 color;

@vertex

uniform vec4 param; //radius dist yscale
uniform vec4 vector_l;
uniform vec4 vector_r;

void main()
{
    vec3 tc = gl_MultiTexCoord0.xyz;
    color = mix(vec4(0.18, 0.4, 0.6, 1.8), vec4(0.87, 0.35, 0.24, 0.0), tc.y);

    vec4 pos = gl_Vertex;
    pos.xy *= param.x;
    pos.y *= param.z;
    pos.x += tc.z * param.y;
    pos.z = 0.0;
    pos.xyz += mix(vector_r.xyz, vector_l.xyz, tc.z * 0.5 + 0.5) * gl_Vertex.z;
    gl_Position = gl_ModelViewProjectionMatrix * pos;
}

@fragment

void main()
{
    gl_FragColor = color;
}
