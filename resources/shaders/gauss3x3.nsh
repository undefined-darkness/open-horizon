@predefined vp "nya viewport"
@sampler base "diffuse"

@all

varying vec2 tc;
varying vec2 iwh;

@vertex

uniform vec4 vp;

void main()
{
    tc = gl_MultiTexCoord0.xy;
    iwh = 2.0 / vp.zw;
    gl_Position = gl_Vertex;
}

@fragment

uniform sampler2D base;

void main( void )
{
	const mat3 gaussianCoef = mat3(1.0,2.0,1.0,2.0,4.0,2.0,1.0,2.0,1.0);

    vec2 texCoord = tc-iwh;
	vec4 result = vec4(0.0);
	for(int i=0;i<3;++i)
    {
        texCoord.y = tc.y-iwh.y;
		for(int j=0;j<3;++j)
        {
			result += gaussianCoef[i][j] * texture2D(base, texCoord);
            texCoord.y += iwh.y;
		}
        texCoord.x += iwh.x;
	}

	result *= 1.0/16.0;
	gl_FragColor = result;
}
