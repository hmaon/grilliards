#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 color_part;

in vec3 normal_viewspace;
in vec3 normal_modelspace;
in vec3 toLight_viewspace[2];
flat in vec3 lightVec_viewspace[2];
flat in vec3 halfVec_directionalLight_viewspace[2]; 

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D tex_id;
uniform vec4 light_specular[2];
uniform vec4 light_diffuse[2];
uniform int shininess;
uniform float specular;
uniform float diffuse;


const vec3 white = vec3(1.0, 1.0, 1.0);
const vec3 eye = vec3(0.0, 0.0, 1.0);

void main(){
	float pf;
	vec4 c = color_part;

#if 0	
	float nDotVP = max( 0.0, dot(normal_viewspace, lightVec_viewspace[0]) );

	float nDotHV = max(  0.0, dot(normal_viewspace, halfVec_directionalLight_viewspace[0] )  );

	if (nDotVP == 0.0)
	{
		pf = 0.0;
	}
	else
	{
		pf = pow(nDotHV, shininess);
	}
#else
	vec3 VP = normalize(toLight_viewspace[0]);
	vec3 normal = normalize(normal_viewspace);
	vec3 halfVector = normalize(VP + eye);

	float nDotVP = max(0.0, dot(normal, VP) );
	float nDotHV = max(0.0, dot(normal, normalize(halfVector)) );

	if (nDotVP == 0.0)
	{
		pf = 0.0;
	}
	else
	{
		pf = pow(nDotHV, shininess);
	}
#endif

	// Output color = color of the texture at the specified UV
	c += diffuse * light_diffuse[0] * nDotVP;
	c += specular * light_specular[0] * pf;
	c *= texture2D( tex_id, UV );
	//vec3 norm = normalize(normal_modelspace);
	//float v = norm.x / sqrt( 2 * ( 1 + norm.z ) ); 
	//float u = norm.y / sqrt( 2 * ( 1 + norm.z ) );
	//c *= texture2D(tex_id, vec2(u,v));

	c = max(c, light_specular[0] * pf * 0.5);

	color = clamp(c, 0, 1).rgb;
}
