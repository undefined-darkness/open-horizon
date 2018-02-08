@predefined camera_pos "nya camera position"
@predefined vp "nya viewport"

@uniform light_dir "light dir"

@sampler base_map "diffuse"
@sampler color_map "color"
@sampler depth_map "depth"

@all

varying vec4 color;
varying vec2 tc;
varying float special;

@vertex

uniform vec4 dir;
uniform vec4 camera_pos;
uniform vec4 light_dir;

uniform vec4 vp;

uniform sampler2D color_map;
uniform sampler2D depth_map;

void main()
{
    vec4 p = gl_Vertex;
    tc=p.xy;
    color = gl_MultiTexCoord0;
    special = gl_MultiTexCoord0.a;
    color.a = light_dir.a;

    p.x *= vp.w/vp.z;

    vec4 sun_pos = gl_ModelViewProjectionMatrix * vec4(camera_pos.xyz + light_dir.xyz * 100.0, 1.0);
    sun_pos.xy /= sun_pos.w;

    vec3 frame_color = texture2D(color_map, sun_pos.xy*0.5 + vec2(0.5)).rgb;
    float depth = texture2D(depth_map, sun_pos.xy*0.5 + vec2(0.5)).r;
    if(length(frame_color)*depth<1.0 || max(sun_pos.x*sun_pos.x, sun_pos.y*sun_pos.y)>1.0)
    {
        color.a = 0.0;
        special = 0.0;
    }

    gl_Position = vec4(p.xy*p.w*2.0 + sun_pos.xy*p.z,0.0,1.0);
}

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 base = texture2D(base_map, tc);
    float sun = max(1.0-dot(tc,tc), 0.0);
    gl_FragColor = vec4(color.rgb, mix(base.r*color.a, sun*sun*0.2, special));
}
