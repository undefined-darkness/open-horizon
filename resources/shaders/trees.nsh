@predefined camera_pos "nya camera pos"

@all

varying vec2 tc;

@vertex

uniform vec4 camera_pos;

void main()
{
    vec4 p = gl_Vertex;
    vec2 d = gl_MultiTexCoord0.zw;
    
    vec3 cam_dir = normalize(camera_pos.xyz - p.xyz);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    p.xyz += d.x * right + d.y * up;

    tc = gl_MultiTexCoord0.xy;
    
    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 color = texture2D(base_map, tc);
    if(color.a < 0.1)
        discard;
        //gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); else

    gl_FragColor = color;
}
