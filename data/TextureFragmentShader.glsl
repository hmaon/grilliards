#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 color_part;
in vec4 pos; // position in worldspace

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
uniform int skip_sphere;
uniform vec4 lightPosition_worldspace[2];

const int NUM_SPHERES = 16;
const float SPHERE_RADIUS = 2.85; // in a generic renderer you'd expect this to be a uniform but here it really is constant, one half of 5.7cm, the standard pool ball 

const float LIGHT_RADIUS = 30.0;

layout (std140) uniform SpherePositions
{
	vec4 SpherePos[NUM_SPHERES]; // positions
};

const vec3 white = vec3(1.0, 1.0, 1.0);
const vec3 eye = vec3(0.0, 0.0, 1.0);

float cos_remap(float x)
{
	return -cos(x * 3.141592653589) / 2 + 0.5;
}

vec4 tex_tiled(sampler2D tex_id, vec2 UV)
{
	vec2 UV2 = UV + vec2(0.5, 0.5);

	float blend = clamp (2 * (-0.3 + abs(sin(UV.x * 3.14159)) * abs(sin(UV.y * 3.14159))), 0, 1);

	return texture2D(tex_id, UV) * blend + texture2D(tex_id, UV2) * (1-blend);
	//return vec4(0.0, blend, 0.0, 1.0);
}

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

bool ray_sphere_intersection(vec3 p1, vec3 p2, vec3 s, float r)
{
	vec3 d = p2 - p1;

	float a = dot(d, d);
	float b = 2.0 * dot(d, p1 - s);
	float c = dot(s, s) + dot(p1, p1) - 2.0 * dot(s, p1) - r*r;
	
	float test = b*b - 4.0*a*c;

	return test >= 0.0 ? true : false;
	
	//if (test >= 0.0) {
	  // Hit 
	  //float u = (-b - sqrt(test)) / (2.0 * a);
	  //vec3 hitp = p1 + u * (p2 - p1);
	//}	
}

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
	//vec3 VP = normalize(toLight_viewspace[0]);
	vec3 VP = toLight_viewspace[0];
	vec3 normal = normalize(normal_viewspace);
	vec3 halfVector = normalize(VP + eye);

	float nDotVP = max(0.0, dot(normal, VP) );
	float nDotHV = max(0.0, dot(normal, halfVector) );

	if (nDotVP == 0.0)
	{
		pf = 0.0;
	}
	else
	{
		pf = pow(nDotHV, shininess);
	}
#endif

	// shadow stuff
	float shadow = 1.0; // 0.0 for full shadow, 1.0 for no shadow at all 
	for (int i = 0; i < NUM_SPHERES; ++i)
	{
		if (i == skip_sphere) continue;
		if (!ray_sphere_intersection(lightPosition_worldspace[0].xyz, pos.xyz, SpherePos[i].xyz, SPHERE_RADIUS * 3)) continue; // fast fail for distant points...
		
		float hits = 0;
		float tests = 0;
		
		//vec3 sPos = vec3(SpherePos[i].x, pos.y, SpherePos[i].z); // position at table y for fake shadow test		
		//shadow *= step(SPHERE_RADIUS, distance(pos.xyz, sPos));
		
		// take multiple samples
		for (float x = -LIGHT_RADIUS; x < LIGHT_RADIUS; x += LIGHT_RADIUS * 0.332)
		{
			for (float y = -LIGHT_RADIUS; y < LIGHT_RADIUS; y += LIGHT_RADIUS * 0.332)
			{
				tests++;
				vec3 lightOffset = vec3(x, y, 0) 
				/* * rand(vec2(x,y) * 123) */ /* this part is meant to be stochastic sampling but the performance hit is *AWFUL* */
				;
				// do a line-sphere intersection test
				if (ray_sphere_intersection(lightPosition_worldspace[0].xyz + lightOffset, pos.xyz, SpherePos[i].xyz, SPHERE_RADIUS))
				{
					hits++;
				}
			}
		}
		
		shadow *= 1.0 - (hits / tests);
		if (shadow < 0.5) break;
	}

	// Output color = color of the texture at the specified UV
	c += diffuse * light_diffuse[0] * nDotVP * shadow;
	c *= texture2D( tex_id, UV );
	c += specular * light_specular[0] * pf * shadow;

	color = clamp(c, 0, 1).rgb;
}
