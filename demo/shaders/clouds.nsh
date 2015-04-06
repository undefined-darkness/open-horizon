@all

varying vec2 tc;
varying vec2 tc2;
varying vec4 light;

@uniform pos "pos"
@predefined camera_pos "nya camera position"
@uniform light_dir "sprite light dir"=-0.295,0.573,0.764
@uniform diffuse_min "diffuse min"=0.82
@uniform obj_ul "obj upper lower"=2082,782
@uniform amb_l "amb low"=0.256,0.345,0.431
@uniform amb_u "amb up"=0.764,0.761,0.737
@uniform diff "diff"=0.749,0.746,0.706
@uniform fade_fn "fade_farnear"=15000,20000

@vertex

uniform vec4 pos;
uniform vec4 camera_pos;
uniform vec4 light_dir;
uniform vec4 diffuse_min;
uniform vec4 obj_ul;
uniform vec4 amb_l;
uniform vec4 amb_u;
uniform vec4 diff;
uniform vec4 fade_fn;

void main()
{
    vec4 p = gl_Vertex;
    vec4 t = gl_MultiTexCoord0;
    vec2 d = gl_MultiTexCoord1.xy*gl_MultiTexCoord1.zw;

    vec3 cam_diff = camera_pos.xyz - (p.xyz + pos.xyz);
    vec3 cam_dir = normalize(cam_diff);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    vec3 n = normalize(vec3(p.x,1.0,p.z));
    float l = dot(light_dir.xyz, n);
    l = clamp(l, diffuse_min.x, 1.0);

    p.xyz += pos.xyz;
    p.xyz += (right.xyz*d.x + up.xyz*d.y);

	float a = clamp((p.y - obj_ul.y) / (obj_ul.x - obj_ul.y), 0.0, 1.0);
	vec3 amb = (amb_u.xyz - amb_l.xyz) * a + amb_l.xyz;
    light.xyz = diff.xyz * l + amb;

    float dist = length(cam_diff);
    light.w = clamp((fade_fn.y - dist) / (fade_fn.y - fade_fn.x), 0.0, 1.0);

    tc=t.xy;
    tc2=t.zw;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 color = texture2D(base_map, tc);
    vec4 detail = texture2D(base_map, tc2);
    color *= detail;
    color *= light;

    gl_FragColor = vec4(color.rgb * 0.8, color.a); //
}
