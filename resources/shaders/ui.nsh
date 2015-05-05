@sampler base_map "diffuse"

@uniform color "color"
@uniform tr "transform"
@uniform tc_tr "tc transform"

@all

varying vec2 tc;

@vertex

uniform vec4 tr[200];
uniform vec4 tc_tr[200];

void main()
{
    tc = tc_tr[gl_InstanceID].xy + tc_tr[gl_InstanceID].zw * gl_MultiTexCoord0.xy;
    gl_Position = vec4(tr[gl_InstanceID].xy + tr[gl_InstanceID].zw *gl_Vertex.xy, 0.0, 1.0);
}

@fragment

uniform sampler2D base_map;
uniform vec4 color;

void main()
{
    gl_FragColor = texture2D(base_map, tc) * color;
}
