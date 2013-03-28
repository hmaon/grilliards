#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 color_part;

in vec3 normal_viewspace;
flat in vec3 lightVec_viewspace;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D tex_id;
uniform vec4 light_specular[2];
uniform int shininess;


const vec3 white = vec3(1.0, 1.0, 1.0);

void main(){

	vec4 c = color_part;

	// Eye vector (towards the camera)
	vec3 E = vec3(0.0, 0.0, 1.0);
	// Direction in which the triangle reflects the light
	vec3 R = reflect( -lightVec_viewspace, normalize(normal_viewspace) );
	// Cosine of the angle between the Eye vector and the Reflect vector,
	// clamped to 0
	//  - Looking into the reflection -> 1
	//  - Looking elsewhere -> < 1
	float cosAlpha = clamp( dot(E, R), 0, 1 );

	float pf = pow(cosAlpha, shininess);

	// Output color = color of the texture at the specified UV
	c += light_specular[0] * pf;
	c *= texture2D( tex_id, UV );

	c = max(c, light_specular[0] * pf);

	color = clamp(c, 0, 1).rgb;

	//color = white;
}
