@predefined camera_pos "nya camera pos"

@uniform param "param"
@uniform dir "dir"
@uniform pos "pos"

@all

varying vec2 tc;
varying vec4 color;

@vertex

uniform vec4 param[16];
uniform vec4 camera_pos;
uniform vec4 pos;
uniform vec4 dir;

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.z);

    vec4 p = param[idx];

    tc = gl_MultiTexCoord0.xy + vec2(p.z, 6.0);
    tc *= vec2(64.0 / 2048.0, 128.0 / 2048.0);

    vec3 r = vec3(dir.z,dir.y,-dir.x);
    vec3 u = cross(dir.xyz, r);

    float radius = 30.0;
    float size = v.x * 10.0;
    float len = (p.w * 5.0 - 2.0 + v.y) * 200.0;
    vec3 n = r * p.x + u * p.y;
    v.xyz = pos.xyz + n * radius;
    v.xyz += size * normalize(cross(camera_pos.xyz - v - dir.xyz * 100.0, n)) + dir.xyz * len;

    color = vec4(1.0, 1.0, 1.0, (0.5 - abs(p.w-0.5)) * 0.05);

    gl_Position = gl_ModelViewProjectionMatrix * vec4(v, 1.0);
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    gl_FragColor = vec4(color.xyz, texture2D(base_map, tc).r * color.a);
}
