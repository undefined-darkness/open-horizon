@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"

@all

varying vec2 tc;

@vertex

uniform vec4 pos[240];

uniform vec4 camera_pos;

void main()
{
    //ToDo

    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    gl_FragColor = vec4(vec3(0.7), texture2D(base_map, tc).r * 0.7);
}
