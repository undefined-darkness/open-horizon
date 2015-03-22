@all

varying vec2 tc;

@uniform pos "pos"
@uniform up "up"
@uniform right "right"
@predefined camera_pos "nya camera position"

@vertex

uniform vec4 pos;
uniform vec4 up;
uniform vec4 right;
uniform vec4 camera_pos;

void main()
{
    vec4 p = gl_Vertex;
    vec4 t = gl_MultiTexCoord0;
    vec2 d = gl_MultiTexCoord1.xy*t.zw;

    p.xyz += (pos.xyz + right.xyz*d.x + up.xyz*d.y);
    
    tc=t.xy;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    gl_FragColor=texture2D(base_map,tc);
}
