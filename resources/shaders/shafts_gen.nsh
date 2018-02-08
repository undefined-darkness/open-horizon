@predefined camera_pos "nya camera position"
@predefined camera_dir "nya camera dir"
@predefined vp "nya viewport"

@uniform light_dir "light dir"
@uniform sun_color "sun color" = 1.0,1.0,1.0

@all

varying vec2 tc;
varying vec2 tc2;
varying vec4 color;

@vertex

uniform vec4 camera_pos;
uniform vec4 camera_dir;
uniform vec4 light_dir;
uniform vec4 sun_color;

uniform vec4 vp;

void main()
{
    vec4 p = gl_Vertex;
    tc=p.xy;
    p.x *= vp.w/vp.z;

    vec4 sun_pos = gl_ModelViewProjectionMatrix * vec4(camera_pos.xyz + light_dir.xyz * 100.0, 1.0);
    sun_pos.xy /= sun_pos.w;
    sun_pos.xy += p.xy*0.3;

    tc2 = sun_pos.xy*0.5+vec2(0.5);

    gl_Position = vec4(sun_pos.xy,1.0,1.0); //*light_dir.w

    float cdl = dot(camera_dir, light_dir);
    color = sun_color * cdl * cdl;
    if(cdl < 0.0)
        gl_Position = vec4(0.0);
}

@sampler depth_map "depth"

@fragment

uniform sampler2D depth_map;

void main()
{
    float depth = texture2D(depth_map, tc2).x;
    if (depth<1.0)
        discard;
    gl_FragColor = color * pow(max(1.0-dot(tc,tc), 0.0), 10.0);
}
