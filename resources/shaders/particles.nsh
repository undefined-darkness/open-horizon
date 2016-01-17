@predefined camera_pos "nya camera pos"

@uniform tr_pos "tr pos"
@uniform tr_tc_rgb "tr tc_rgb"
@uniform tr_tc_a "tr tc_a"
@uniform col "color"

@all

varying vec2 tc_rgb;
varying vec2 tc_rgb_s;
varying vec2 tc_a;
varying vec2 from_alpha;
varying vec4 color;

@vertex

uniform vec4 camera_pos;

uniform vec4 tr_pos[120];
uniform vec4 tr_tc_rgb[120];
uniform vec4 tr_tc_a[120];
uniform vec4 col[120];

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.z);

    vec4 p = tr_pos[idx];
    vec4 t = tr_tc_rgb[idx];
    vec4 ta = tr_tc_a[idx];
    color = col[idx];

    vec3 cam_dir = normalize(camera_pos.xyz - p.xyz);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    p.xyz += (gl_Vertex.x * right + gl_Vertex.y * up) * p.w;

    float color_repeat = 1.0;

    tc_rgb = gl_MultiTexCoord0.xy * t.zw * color_repeat + t.xy;
    tc_rgb_s = t.zw;
    tc_a = gl_MultiTexCoord0.xy * ta.zw + ta.xy;

    from_alpha.x = t.w < 0.0 ? 1.0 : 0.0;
    from_alpha.y = ta.w < 0.0 ? 1.0 : 0.0;

    p.w = 1.0;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 o;

    vec4 base = texture2D(base_map, tc_rgb); //(tc_rgb - floor(tc_rgb * tc_rgb_s) / tc_rgb_s)
    o.rgb = mix(base.rgb, base.aaa, from_alpha.x);

    vec4 alpha = texture2D(base_map, tc_a);
    o.a = mix(alpha.r, alpha.a, from_alpha.y);

    gl_FragColor = o * color;
}
