@all

varying vec4 tc;

@predefined camera_rot "nya camera rotation"
@predefined vp "nya viewport"

@vertex

uniform vec4 camera_rot;
uniform vec4 vp;

vec3 tr(vec3 pos,vec4 q) { return pos+cross(q.xyz,cross(q.xyz,pos)+pos*q.w)*2.0; }

void main()
{
    vec4 pos=gl_Vertex;
	pos.xy=pos.xy*2.0+vec2(-1.0);
	pos.z=1.0;
    gl_Position=vec4(pos.xyz,1.0);
	//pos.x*=(vp.z/vp.w);
	pos.y*=(vp.w/vp.z);
	tc.xyz=normalize(tr(pos.xyz,camera_rot));
}

@sampler base_map "diffuse"

@fragment

uniform samplerCube base_map;

void main() { gl_FragColor=textureCube(base_map,tc.xyz); }
