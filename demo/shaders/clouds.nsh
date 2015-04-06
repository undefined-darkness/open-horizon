@all

varying vec2 tc;
varying vec2 tc2;
varying vec3 light;

@uniform pos "pos"
@predefined camera_pos "nya camera position"
@uniform light_dir "light dir"=0.5,1.0,0.5

@vertex

uniform vec4 pos;
uniform vec4 camera_pos;
uniform vec4 light_dir;

void main()
{
    vec4 p = gl_Vertex;
    vec4 t = gl_MultiTexCoord0;
    vec2 d = gl_MultiTexCoord1.xy*gl_MultiTexCoord1.zw;

    vec3 ldir=normalize(vec3(0.5,1.0,0.5));

    vec3 cam_dir = normalize(camera_pos.xyz - (p.xyz + pos.xyz));
    
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    p.xyz += (right.xyz*d.x + up.xyz*d.y);

	//light=vec3(0.7+0.3*max(dot(normalize(p.xyz),ldir.xyz),0.0));

    p.xyz += pos.xyz;

    tc=t.xy;
    tc2=t.zw;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 color = texture2D(base_map,tc);
    vec4 detail = texture2D(base_map,tc2);
    color *= detail;

    gl_FragColor = vec4(color.rgb, color.a); // * 0.8
}
