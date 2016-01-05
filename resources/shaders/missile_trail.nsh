@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform dir "tr dir"

@all

varying vec2 tc;

@vertex

uniform vec4 pos[240];
uniform vec4 dir[240];

uniform vec4 camera_pos;

void main()
{
    vec2 v = gl_Vertex.xy;
    int idx = int(v.y);
    
    tc.y = dir[idx].w * 0.005;
    tc.x = (v.x * 0.5 + 0.5) * 0.25 + 0.25;

    float width = 5.0;

    vec3 p = pos[idx].xyz;
    p += normalize(cross(camera_pos.xyz - p, dir[idx].xyz)) * v.x * width;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0);
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec2 ftc = vec2(tc.x, (tc.y - floor(tc.y)) * 0.95 + 0.025);

    gl_FragColor = vec4(vec3(0.7), texture2D(base_map, ftc).r * 0.7);
}
