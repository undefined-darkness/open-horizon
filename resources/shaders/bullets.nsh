@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform dir "b dir"
@uniform color "b color"
@uniform b_tc "b tc"
@uniform b_size "b size"

@all

varying vec2 tc;

@vertex

uniform vec4 pos[240];
uniform vec4 dir[240];
uniform vec4 b_tc;
uniform vec4 b_size;

uniform vec4 camera_pos;

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.z);

    tc = gl_MultiTexCoord0.xy * b_tc.zw + b_tc.xy;

    vec3 p = pos[idx].xyz;
    vec3 d = normalize(dir[idx].xyz);
    p += normalize(cross(camera_pos.xyz - p, d)) * v.x * b_size.x;
    p += (v.y * 0.5 + 0.5) * d * b_size.y;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0);
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;
uniform vec4 color;

void main()
{
    gl_FragColor = vec4(color.xyz, texture2D(base_map, tc).r * color.a);
}
