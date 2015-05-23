@sampler base_map "diffuse"

@uniform color "color"
@uniform tr "transform"
@uniform tc_tr "tc transform"

@all

varying vec2 tc;

@vertex

uniform vec4 tr[100];
uniform vec4 tc_tr[100];

void main()
{
    int idx = int(gl_MultiTexCoord0.z);
    tc = tc_tr[idx].xy + tc_tr[idx].zw * gl_MultiTexCoord0.xy;
    gl_Position = vec4(tr[idx].xy + tr[idx].zw * gl_Vertex.xy, 0.0, 1.0);
}

@fragment

uniform sampler2D base_map;
uniform vec4 color;

void main()
{
    vec4 base = texture2D(base_map, tc);
    gl_FragColor = base * color;
    //gl_FragColor = mix(vec4(1.0,0.0,0.0,0.5),base * color,base.a);
}
