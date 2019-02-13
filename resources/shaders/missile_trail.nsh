@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform dir "tr dir"
@uniform param "tr param"
@uniform light_dir "light dir"=0.0,1.0,0.0

@all

varying vec3 tc;
varying vec4 color;

@vertex

uniform vec4 pos[240];
uniform vec4 dir[240];
uniform vec4 param;
uniform vec4 light_dir;

uniform vec4 camera_pos;

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.y);

    float t = param.x - pos[idx].w;
    float width = min(0.1 + t * 0.6, 1.0);

    vec3 p = pos[idx].xyz;
    vec3 normal = normalize(cross(camera_pos.xyz - p, dir[idx].xyz)) * v.x;

    color = vec4(max(0.0, 0.6 + 0.4 * dot(light_dir.xyz,normalize(normal))));
    color.a *= min(8.0-t, 1.0) * param.y;

    //for perspective-correct interpolation
    tc.z = width;
    tc.x = v.x * tc.z;
    tc.y = dir[idx].w * 0.5;

    p += normal * width * 10.0;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0);
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec2 ftc = vec2(tc.x / tc.z * 0.125 + 0.375, (tc.y - floor(tc.y)) * 0.95 + 0.025);
    gl_FragColor.rgb = color.rgb;
    gl_FragColor.a = color.a * texture2D(base_map, ftc).r;
}
