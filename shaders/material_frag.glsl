varying vec3 norm;
varying vec3 cam_dir;
varying vec3 color;

// Declare any additional variables here. You may need some uniform variables.
uniform samplerCube cubemap;


void main(void)
{
	// Your shader code here.
	// Note that this shader won't compile since gl_FragColor is never set.
	vec3 reflect_vec = reflect(cam_dir, normalize(norm));
	gl_FragColor = textureCube(cubemap, reflect_vec);	
}
