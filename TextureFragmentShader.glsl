#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 color_part;

in vec3 normal_viewspace;
in vec3 normal_modelspace;
in vec3 reflectVec_viewspace[2];
flat in vec3 lightVec_viewspace[2];

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

void main(){

	vec4 c = color_part;
	
	float nDotVP = max( 0.0, dot(normal_viewspace, lightVec_viewspace[0]) );

	// Eye vector (towards the camera)
	vec3 E = vec3(0.0, 0.0, 1.0);

	// Direction in which the triangle reflects the light
	//vec3 R = reflect( -lightVec_viewspace, normalize(normal_viewspace) ); // recalculate R every fragment
	vec3 R = normalize(reflectVec_viewspace[0]); // or interpolate it from vertexes?

	// Cosine of the angle between the Eye vector and the Reflect vector,
	// clamped to 0
	//  - Looking into the reflection -> 1
	//  - Looking elsewhere -> < 1
	float cosAlpha = clamp( dot(E, R), 0, 1 );

	float pf;
	if (nDotVP == 0.0)
	{
		pf = 0.0;
	}
	else
	{
		pf = pow(cosAlpha, shininess);
	}

	// Output color = color of the texture at the specified UV
	c += diffuse * light_diffuse[0] * nDotVP;
	c += specular * light_specular[0] * pf;
	c *= texture2D( tex_id, UV );
	//vec3 norm = normalize(normal_modelspace);
	//float v = norm.x / sqrt( 2 * ( 1 + norm.z ) ); 
	//float u = norm.y / sqrt( 2 * ( 1 + norm.z ) );
	//c *= texture2D(tex_id, vec2(u,v));

	c = max(c, light_specular[0] * pf);

	color = clamp(c, 0, 1).rgb;
}
