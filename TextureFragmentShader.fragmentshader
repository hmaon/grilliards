#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 color_part;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D tex_id;

const vec3 white = vec3(1.0, 1.0, 1.0);

void main(){

	// Output color = color of the texture at the specified UV
	color = texture2D( tex_id, UV ).rgb * color_part.rgb;
	//color = white;
}
