@predefined vp "nya viewport"
@sampler base_map "diffuse"

@all

varying vec4 tc;

@vertex

uniform vec4 vp;

void main()
{
    vec2 off = 1.0 / vp.zw;
    tc.xy = gl_MultiTexCoord0.xy;
    tc.zw = 1.0 / vp.zw;
    gl_Position = gl_Vertex;
}

@fragment

uniform sampler2D base_map;

float luminance(vec2 tc)
{
	vec3 color = texture2D(base_map, tc).xyz;
    float lum = dot(color, vec3(0.2125, 0.7154, 0.0721));
	return 0.69314 * exp(lum * 1.25 + 0.0001);
}

void main()
{
	vec3 c = vec3(0.0, -1.0, 1.0);

	vec4 tc0, tc1;
	
	float s=0.0;

    tc0.xy = vec2(1.0 / 128.0) + tc.xy;
	tc0.zw = tc0.xy - tc.zw;

	s += luminance(tc0.zw);

	tc1 = tc.zwzw * c.xyzy + tc0.xyxy;

	s += luminance(tc1.xy);
	s += luminance(tc1.zw);

	tc1 = tc.zwzw * c.yxzx + tc0.xyxy;

	s += luminance(tc1.zw);
	s += luminance(tc1.xy);
	s += luminance(tc0.xy);

	tc1 = tc.zwzw * c.yzxz + tc0.xyxy;
	tc0.xy += tc.zw;

	s += luminance(tc0.xy);
	s += luminance(tc1.zw);
	s += luminance(tc1.xy);

	s /= 9.0;
	gl_FragColor = vec4(s, s, s, 1.0);
}
