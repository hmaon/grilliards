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

uniform vec3 randOffsets[64] = vec3[64](
        vec3(0.375000, 0, 0.000000),
        vec3(0.430928, 0, 0.042443),
        vec3(0.704273, 0, 0.140089),
        vec3(0.644160, 0, 0.195404),
        vec3(0.305544, 0, 0.126561),
        vec3(0.846771, 0, 0.452608),
        vec3(0.293969, 0, 0.196424),
        vec3(0.537993, 0, 0.441519),
        vec3(0.673146, 0, 0.673146),
        vec3(0.354637, 0, 0.432126),
        vec3(0.268964, 0, 0.402533),
        vec3(0.353548, 0, 0.661441),
        vec3(0.067650, 0, 0.163320),
        vec3(0.253999, 0, 0.837323),
        vec3(0.142195, 0, 0.714864),
        vec3(0.083996, 0, 0.852830),
        vec3(0.000000, 0, 0.530330),
        vec3(-0.053406, 0, 0.542239),
        vec3(-0.182490, 0, 0.917441),
        vec3(-0.181428, 0, 0.598088),
        vec3(-0.379682, 0, 0.916633),
        vec3(-0.131759, 0, 0.246504),
        vec3(-0.524308, 0, 0.784682),
        vec3(-0.549401, 0, 0.669447),
        vec3(-0.565962, 0, 0.565962),
        vec3(-0.453218, 0, 0.371946),
        vec3(-0.549965, 0, 0.367475),
        vec3(-0.817563, 0, 0.436996),
        vec3(-0.461940, 0, 0.191342),
        vec3(-0.870829, 0, 0.264163),
        vec3(-0.803930, 0, 0.159912),
        vec3(-0.971580, 0, 0.095692),
        vec3(-0.612372, 0, 0.000000),
        vec3(-0.843709, 0, -0.083098),
        vec3(-0.561815, 0, -0.111752),
        vec3(-0.775210, 0, -0.235157),
        vec3(-0.774697, 0, -0.320890),
        vec3(-0.190942, 0, -0.102060),
        vec3(-0.805067, 0, -0.537929),
        vec3(-0.595645, 0, -0.488833),
        vec3(-0.216506, 0, -0.216506),
        vec3(-0.296710, 0, -0.361543),
        vec3(-0.380373, 0, -0.569268),
        vec3(-0.348603, 0, -0.652190),
        vec3(-0.000000, 0, -0.000000),
        vec3(-0.220717, 0, -0.727605),
        vec3(-0.126715, 0, -0.637039),
        vec3(-0.086636, 0, -0.879627),
        vec3(-0.000000, 0, -0.901388),
        vec3(0.069309, 0, -0.703702),
        vec3(0.077116, 0, -0.387689),
        vec3(0.266644, 0, -0.879006),
        vec3(0.243914, 0, -0.588860),
        vec3(0.195431, 0, -0.365625),
        vec3(0.546821, 0, -0.818375),
        vec3(0.380305, 0, -0.463403),
        vec3(0.176777, 0, -0.176777),
        vec3(0.690050, 0, -0.566309),
        vec3(0.649066, 0, -0.433692),
        vec3(0.731250, 0, -0.390862),
        vec3(0.416387, 0, -0.172473),
        vec3(0.493196, 0, -0.149609),
        vec3(0.122598, 0, -0.024386),
        vec3(0.786763, 0, -0.077489)
);



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
		tests = 64;
		for (int j = 0; j < 64; ++j)
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
