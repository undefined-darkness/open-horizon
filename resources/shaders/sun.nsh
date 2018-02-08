@predefined camera_pos "nya camera position"
@predefined vp "nya viewport"

@uniform light_dir "light dir"
@uniform color "color" = 1.0,1.0,1.0

@all

varying vec2 tc;

@vertex

uniform vec4 camera_pos;
uniform vec4 light_dir;

uniform vec4 vp;

void main()
{
    vec4 p = gl_Vertex;
    tc=p.xy;//*0.5+vec2(0.5);
    p.x *= vp.w/vp.z;

    vec4 sun_pos = gl_ModelViewProjectionMatrix * vec4(camera_pos.xyz + light_dir.xyz * 100.0, 1.0);
    sun_pos.xy /= sun_pos.w;

    gl_Position = vec4(p.xy*0.3 + sun_pos.xy,1.0,1.0); //*light_dir.w
}

@sampler base_map "diffuse"

@fragment

//uniform sampler2D base_map;
uniform vec4 color;

void main()
{
    //vec4 base = texture2D(base_map, tc);
    vec4 base=vec4(pow(max(1.0-dot(tc,tc), 0.0), 10.0));
    gl_FragColor = vec4(color.rgb, base.r);
}
