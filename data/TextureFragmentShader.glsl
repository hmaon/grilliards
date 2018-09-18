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

uniform vec3 randOffsets[50] = vec3[50](
        vec3(0.648074, 0, 0.000000),
        vec3(0.698455, 0, 0.179333),
        vec3(0.803148, 0, 0.441535),
        vec3(0.230520, 0, 0.216473),
        vec3(0.485212, 0, 0.764571),
        vec3(0.231247, 0, 0.711706),
        vec3(0.041651, 0, 0.662016),
        vec3(-0.161192, 0, 0.844995),
        vec3(-0.399416, 0, 0.848803),
        vec3(-0.604713, 0, 0.730973),
        vec3(-0.443117, 0, 0.321943),
        vec3(-0.891810, 0, 0.353093),
        vec3(-0.524978, 0, 0.066320),
        vec3(-0.465343, 0, -0.058786),
        vec3(-0.920432, 0, -0.364425),
        vec3(-0.280252, 0, -0.203615),
        vec3(-0.509939, 0, -0.616411),
        vec3(-0.312883, 0, -0.664909),
        vec3(-0.181673, 0, -0.952363),
        vec3(0.035520, 0, -0.564569),
        vec3(0.151387, 0, -0.465921),
        vec3(0.321496, 0, -0.506597),
        vec3(0.609899, 0, -0.572733),
        vec3(0.175261, 0, -0.096351),
        vec3(0.898227, 0, -0.230626),
        vec3(0.761577, 0, -0.000000),
        vec3(0.273957, 0, 0.070340),
        vec3(0.371785, 0, 0.204391),
        vec3(0.461040, 0, 0.432946),
        vec3(0.371232, 0, 0.584968),
        vec3(0.000000, 0, 0.000000),
        vec3(0.044400, 0, 0.705711),
        vec3(-0.154519, 0, 0.810015),
        vec3(-0.361286, 0, 0.767771),
        vec3(-0.555693, 0, 0.671718),
        vec3(-0.548702, 0, 0.398655),
        vec3(-0.347891, 0, 0.137740),
        vec3(-0.887374, 0, 0.112101),
        vec3(-0.611581, 0, -0.077261),
        vec3(-0.371911, 0, -0.147250),
        vec3(-0.792672, 0, -0.575910),
        vec3(-0.371679, 0, -0.449283),
        vec3(-0.104294, 0, -0.221636),
        vec3(-0.165491, 0, -0.867533),
        vec3(0.048637, 0, -0.773068),
        vec3(0.251047, 0, -0.772642),
        vec3(0.239629, 0, -0.377595),
        vec3(0.371703, 0, -0.349052),
        vec3(0.123928, 0, -0.068130),
        vec3(0.762663, 0, -0.195819)
);const vec3 white = vec3(1.0, 1.0, 1.0);
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
		// square area light :o
#if 0
		for (float x = -LIGHT_RADIUS; x < LIGHT_RADIUS; x += LIGHT_RADIUS * 0.332)
		{
			for (float y = -LIGHT_RADIUS; y < LIGHT_RADIUS; y += LIGHT_RADIUS * 0.332)
			{
				tests++;
				vec3 lightOffset = vec3(x, 0, y) 
				/* * rand(vec2(x,y) * 123) */ /* this part is meant to be stochastic sampling but the performance hit is *AWFUL* */
				;
				// do a line-sphere intersection test
				if (ray_sphere_intersection(lightPosition_worldspace[0].xyz + lightOffset, pos.xyz, SpherePos[i].xyz, SPHERE_RADIUS))
				{
					hits++;
				}
			}
		}
#else
		// round area light, random sample pattern
		// serious people would probably call it stochastic sampling
		tests = 50;
		for (int j = 0; j < 50; ++j)
		{
				vec3 lightOffset = randOffsets[j] * LIGHT_RADIUS;
				// do a line-sphere intersection test
				if (ray_sphere_intersection(lightPosition_worldspace[0].xyz + lightOffset, pos.xyz, SpherePos[i].xyz, SPHERE_RADIUS))
				{
					hits++;
				}
		}
#endif
		
		shadow *= 1.0 - (hits / tests);
		if (shadow < 0.1) break;
	}

	// Output color = color of the texture at the specified UV
	c += diffuse * light_diffuse[0] * nDotVP * shadow;
	c *= texture2D( tex_id, UV );
	c += specular * light_specular[0] * pf * shadow;

	color = clamp(c, 0, 1).rgb;
}
