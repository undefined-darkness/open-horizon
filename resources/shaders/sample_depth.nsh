@all

varying vec2 tc;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}

@sampler depth_map "depth"

@fragment

uniform sampler2D depth_map;

void main()
{
    //gl_FragColor = vec4(0.0);
    gl_FragDepth = texture2D(depth_map, tc).x;
}
