@uniform bloom_param "bloom_param"
@sampler base_map "diffuse"

@all

varying vec2 tc;

@vertex

void main()
{
    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}

@fragment

uniform sampler2D base_map;
uniform vec4 bloom_param;

void main()
{
    vec4 color = texture2D(base_map,tc);
    color = color * 1.25 - bloom_param.xxxx;
    float lum = dot(color.xyz, vec3(0.2125, 0.7154, 0.0721));
    color = mix(vec4(lum), color, vec4(0.3));
    color = max( color, vec4(0.0));

    gl_FragColor = color / (color + bloom_param.yyyy);
}
