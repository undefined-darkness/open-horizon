@predefined vp "nya viewport"
@uniform bloom_param "bloom_param"
@sampler base "diffuse"
@sampler bloom "bloom"
@sampler lum "lum"

@all

varying vec2 tc;
varying vec2 iwh;

@vertex

uniform vec4 vp;

void main()
{
    tc = gl_MultiTexCoord0.xy;
    iwh = 1.0 / vp.zw;
    gl_Position = gl_Vertex;
}

@fragment

uniform sampler2D base;
uniform sampler2D bloom;
uniform sampler2D lum;

uniform vec4 bloom_param;

void main( void )
{
    vec2 off = vec2(-8.0,8.0);

	vec4 t1 = iwh.xyxy * off.xxyx + tc.xyxy;
	vec4 t2 = iwh.xyxy * off.yyxy + tc.xyxy;

	vec4 color = texture2D(base, tc);

    color *= (1.05 / texture2D(lum, vec2(0.5)) * 0.8 + 0.2); //ToDo

    vec4 b = texture2D(bloom, t1.xy);
	b += texture2D(bloom, t1.zw);

	b += texture2D(bloom, t2.xy);
	b += texture2D(bloom, t2.zw);

	gl_FragColor = color * 1.25 + b * 0.25 * bloom_param.z;
}
