@uniform param "param"
@uniform vector_l "vector l"
@uniform vector_r "vector r"
@sampler base "diffuse"

@all
varying vec4 color;
varying vec2 tc;

@vertex

uniform vec4 param; //radius dist yscale time
uniform vec4 vector_l;
uniform vec4 vector_r;

void main()
{
    vec3 tc0 = gl_MultiTexCoord0.xyz;
    //color = mix(vec4(0.18, 0.4, 0.6, 1.8), vec4(0.87, 0.35, 0.24, 0.0), tc0.y);
    color = mix(vec4(0.18, 0.4, 0.6, 0.5), vec4(0.87, 0.35, 0.24, 1.0), tc0.y) * 2.8;

    tc = (vec2(1921.0 + 124.0 * param.w, 768.0 + 124.0) + vec2(3.0, -124.0) * tc0.xy) / 2048.0;

    vec4 pos = gl_Vertex;
    pos.xy *= param.x;
    pos.y *= param.z;
    pos.x += tc0.z * param.y;
    pos.z = 0.0;
    pos.xyz += mix(vector_r.xyz, vector_l.xyz, tc0.z * 0.5 + 0.5) * gl_Vertex.z;
    gl_Position = gl_ModelViewProjectionMatrix * pos;
}

@fragment
uniform sampler2D base;

void main()
{
    gl_FragColor = color * texture2D(base, tc).a;
}
