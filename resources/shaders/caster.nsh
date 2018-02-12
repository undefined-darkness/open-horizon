@include "skeleton.nsh"

@vertex

void main()
{
	vec3 pos = tr(gl_Vertex.xyz, gl_MultiTexCoord3, gl_MultiTexCoord4);
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}

@fragment

void main()
{
    gl_FragColor = vec4(1.0);
}
