@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform dir "tr dir"
@uniform param "tr param"

@all

varying vec2 tc;
varying vec4 color;

@vertex

uniform vec4 pos[240];
uniform vec4 dir[240];
uniform vec4 param;

uniform vec4 camera_pos;

void main()
{
    vec2 v = gl_Vertex.xy;
    int idx = int(v.y);

    tc.y = dir[idx].w * 0.005;
    tc.x = (v.x * 0.5 + 0.5) * 0.25 + 0.25;

    float t = param.x - pos[idx].w;
    float width = min(0.5 + t * 3.0, 5.0);
    color = vec4(0.7);
    color.a *= min(8.0-t, 1.0) * param.y;

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
    gl_FragColor.rgb = color.rgb;
    gl_FragColor.a = color.a * texture2D(base_map, ftc).r;
}
