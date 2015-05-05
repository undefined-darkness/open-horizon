@predefined vp "nya viewport"
@sampler base_map "diffuse"
@sampler prev_map "prev"
@uniform mix_speed "lum_adapt_speed"

@all

varying vec2 tc[4];

@vertex

uniform vec4 vp;

void main()
{
    vec2 off = 1.0 / vp.zw;
    tc[0] = gl_MultiTexCoord0.xy;
    tc[1] = gl_MultiTexCoord0.xy + off;
    tc[2] = gl_MultiTexCoord0.xy + vec2(off.x, 0.0);
    tc[3] = gl_MultiTexCoord0.xy + vec2(0.0, off.y);
    gl_Position = gl_Vertex;
}

@fragment

uniform sampler2D base_map;
uniform sampler2D prev_map;
uniform vec4 mix_speed;

void main()
{
    vec4 prev = texture2D(prev_map, tc[0]);
    vec4 color = ( texture2D(base_map, tc[0])
                 + texture2D(base_map, tc[1])
                 + texture2D(base_map, tc[2])
                 + texture2D(base_map, tc[3]) ) * 0.25;

    gl_FragColor = mix(prev, color, mix_speed);
}
