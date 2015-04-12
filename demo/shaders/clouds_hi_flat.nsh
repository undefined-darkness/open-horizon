@predefined camera_pos "nya camera position"

@sampler base_map "diffuse"

@all

varying vec2 tc;

@vertex

uniform vec4 camera_pos;

void main()
{
    vec4 p = gl_Vertex;
    vec4 t = gl_MultiTexCoord0;
    vec2 d = gl_MultiTexCoord1.xy;

    p.xz += d * 15000.0;

    p.y = 15000.0 + camera_pos.y - 2000.0;

    p.xyz =  (p.xyz-camera_pos.xyz)*0.1 + camera_pos.xyz;

    tc=t.xy;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 color = texture2D(base_map, tc);
    gl_FragColor = vec4(color.rgb * 0.8, color.a);
}
