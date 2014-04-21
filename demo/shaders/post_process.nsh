@all

varying vec4 tc;

@uniform tr "transform"

@vertex

uniform vec4 tr;

void main()
{
    vec4 pos=gl_Vertex; tc=pos;
    gl_Position=vec4(vec2(-1.0,-1.0)+pos.xy*2.0,0.0,1.0);
}

@sampler base_map "diffuse"
@sampler curve_map0 "curve0"
@sampler curve_map1 "curve1"
@sampler curve_map2 "curve2"

@fragment

uniform sampler2D base_map;
uniform sampler2D curve_map0;
uniform sampler2D curve_map1;
uniform sampler2D curve_map2;

void main()
{
	vec4 color=texture2D(base_map,tc.xy);//+vec4(0.05);
	color.r = texture2D(curve_map0,color.ra).r;
	color.g = texture2D(curve_map1,color.ga).g;
	color.b = texture2D(curve_map2,color.ba).b;
    gl_FragColor=color;
}
