@predefined camera_pos "nya camera position"
@predefined vp "nya viewport"

@uniform light_dir "light dir"

@all

varying vec4 color;
varying vec2 tc;

@vertex

uniform vec4 dir;
uniform vec4 camera_pos;
uniform vec4 light_dir;

uniform vec4 vp;

void main()
{
    vec4 p = gl_Vertex;
    tc=p.xy;
    color = gl_MultiTexCoord0;
    color.a = light_dir.a;

    p.y *= vp.z/vp.w;

    vec4 sun_pos = gl_ModelViewProjectionMatrix * vec4(camera_pos.xyz + light_dir.xyz * 100.0, 1.0);
    sun_pos.xy /= sun_pos.w;

    gl_Position = vec4(p.xy*p.w*1.5 + sun_pos.xy*p.z,0.0,1.0);
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 base = texture2D(base_map, tc);
    gl_FragColor = vec4(color.rgb, base.r*color.a); //0.8
}
