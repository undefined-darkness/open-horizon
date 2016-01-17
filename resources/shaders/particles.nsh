@predefined camera_pos "nya camera pos"

@uniform tr_pos "tr pos"
@uniform tr_tc_rgb "tr tc_rgb"
@uniform tr_tc_a "tr tc_a"

@all

varying vec2 tc_rgb;
varying vec2 tc_a;
varying vec2 from_alpha;

@vertex

uniform vec4 camera_pos;

uniform vec4 tr_pos[120];
uniform vec4 tr_tc_rgb[120];
uniform vec4 tr_tc_a[120];

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.z);

    vec4 p = tr_pos[idx];
    vec4 t = tr_tc_rgb[idx];
    vec4 ta = tr_tc_a[idx];

    vec3 cam_dir = normalize(camera_pos.xyz - p.xyz);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    p.xyz += (gl_Vertex.x * right + gl_Vertex.y * up) * p.w;

    tc_rgb = gl_MultiTexCoord0.xy * t.zw + t.xy;
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

    vec4 color = texture2D(base_map, tc_rgb);
    o.rgb = mix(color.rgb, color.aaa, from_alpha.x);

    vec4 a = texture2D(base_map, tc_a);
    o.a = mix(a.r, a.a, from_alpha.y);

    gl_FragColor = o;
}
