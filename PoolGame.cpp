#include <glew.h>
#define NO_SDL_GLEXT 1

#include "objloader.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include <GL/freeglut.h>

#include <stdio.h>

// for windows to define M_PI and such:
#define _USE_MATH_DEFINES 1

#include <cmath>
#include <limits>
#include <math.h>
#include <string.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//#include"game_interface.h"
#include "PoolGame.h"
#include "final.h"
#include "shader.hpp"

namespace PoolGame
{

// PoolGame global variables
PoolTable *table = NULL;
bool chasecam = false;
float camera_angle = 0;
float cue_angle = 270;
float cue_power = 1.0f;
bool use_mass = true;
int hits = 0;

PoolGameState state = intro; // pool game state? you don't say? must be the state of the pool game?
PoolGameMode mode = amazeballs; // yep.


// graphics and sound assets:
GLint textures[NUM_TEXTURES] = {-1};

namespace Sounds
{
// sounds?
Mix_Chunk *ballclack = NULL;
Mix_Chunk *beepverb = NULL;
Mix_Chunk *squirble = NULL;
Mix_Chunk *sadwhistle = NULL;
};

// some file-global stuff
// I seem to have trouble initializing arrays as static class members
// so now they're just global variables. *shrug*
GLfloat mat_white[] = {1.0, 1.0, 1.0, 1.0};
GLfloat light_position[] = {0.0, 100.0, -85.0, 1.0};
GLfloat light_position1[] = {0.0, 100.0, 85.0, 1.0};
GLfloat nil_position[] = {0, 0, 0, 1};
// https://en.wikipedia.org/wiki/Sunset_%28color%29
GLfloat sunset[] = {250/255.0f, 214/255.0f, 165/255.0f, 1.0f};
GLfloat tungsten_100w[] = {255.0f/255, 214.0f/255, 170.0f/255};
GLfloat dim_ambiance[] = {255.0f/1024, 214.0f/1024, 170.0f/1024};
GLfloat mat_blue[] = {0.2f, 0.5f, 1.0f, 1.0f};
GLfloat mat_green[] = {0.2f, 1.0f, 0.2f, 1.0f};
GLfloat mat_black[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat mat_grey[] = {0.33f, 0.33f, 0.33f, 1.0f};
// GLfloat mat_shininess[] = {66.0f};

GLfloat v_down[] = {0, -1, 0};


typedef struct BallInfos_struct
{
    char *texture;
    double mass;
    char *name;
} BallInfos;

//#define NUM_TEXTURES 16
BallInfos bally[NUM_TEXTURES] = {
    {"data/celestia.png",    0.3,    "Princess Celestia"},
    {"data/mercurymap.jpg",  0.055,  "Mercury"},
    {"data/venusmap.jpg",    0.815,  "Venus"},
    {"data/earth.jpg",       1,      "Earth"},
    {"data/moonmap1k.jpg",   0.0123, "The Moon"},
    {"data/marsmap1k.jpg",   0.107,  "Mars"},
    {"data/plutomap1k.jpg",  0.01,   "Pluto"},
    {"data/freezerbeef.jpg", 0.2,    "Ground Beef"},
    {"data/ball_8.png",      1,      "Number 8"},
	{"data/nyan.png",        0.2,    "Nyan Cat!"},
	{"data/1.jpg",           0.3,    "Adventure Time"},
	{"data/melon.jpg",       0.2,    "Melon"},
	{"data/ultrapotato.jpg", 0.2,    "Potato"},
	{"data/eye.jpg",         0.2,    "Eye"},
    {"data/clouds.jpg",      0.5,    "Cloudworld"},
    {"data/apple.jpg",       0.2,    "Apple"}
};
    
// the number of the beef:
#define NUM_BEEF 7




// here's a resource for some billiards physics:
// http://billiards.colostate.edu/threads/physics.html
// holy hell am I ever not using most of it
// O_o



// the speed of a cue ball on a breaking shot is 20-ish MPH; second source:
// http://www.sfbilliards.com/onoda_all_txt.pdf
// 20 mph = 894.08 centimeters per second
static int max_speed = 894;



// some forward declarations
class PoolTable;
class PoolBall;
void squirble();
void sadwhistle();


// the poolball is probably the most intensive object in this game:
class PoolBall 
{
public:
    PoolBall(PoolTable *t, double initx, double initz, double initmass, int number);
	~PoolBall();
	
	static std::vector<glm::vec3> vertices;
	static std::vector<glm::vec2> uvs;
	static std::vector<glm::vec3> normals;
	static GLuint VAO_id, vertexbuffer, uvbuffer, normalbuffer;

    void render(glm::dmat4 &parent_model);
	double distance_squared(PoolBall& other);

    void friction(double time_fraction);

    void move(double time_fraction);

	static const double diameter;
    static const double rolling_friction;
    static const double COrestitution; // without this, collisions don't look realistic enough

	void handle_collision(PoolBall& other, long double t, double time_fraction);

    void actually_reposition(double fdx, double fdz);

	double movement_remaining; // [0..1], defines how much of time_fraction we still have to roll this frame, in case of partial moves due to collisions

    double x, z; // position

    double dx, dz; // velocity

	//double rotation[16]; // 4x4 rotation matrix for the orientation of the ball
	glm::dmat4 rotation;

	double mass;

	float material[4];

    static GLUquadric *gluq;

	GLuint tex;

	char *name;

	PoolTable *table;
    int index; // our own index in table->ball[] just because this design is uh
    // probably don't use index to look things up in the array; just
    // use it for identity

    bool inplay;

private:
	
	// this is to be called when the ball has been repositioned to the point of collision with a wall
	void test_pocket();

	friend class PoolTable; // they're like best buddies, for real
};


// use GLUquadric for sphere objects
GLUquadric *PoolBall::gluq = NULL;
const double PoolBall::diameter = 5.7; // standard American pool balls are 57mm (=5.7cm) 
const double PoolBall::rolling_friction = 0.15;
const double PoolBall::COrestitution = 0.95;;

std::vector<glm::vec3> PoolBall::vertices;
std::vector<glm::vec2> PoolBall::uvs;
std::vector<glm::vec3> PoolBall::normals;	

GLuint PoolBall::VAO_id, PoolBall::vertexbuffer, PoolBall::uvbuffer, PoolBall::normalbuffer;



// constructor
PoolBall::PoolBall(PoolTable *t, double initx, double initz, double dummy, int initindex) : x(initx), z(initz), rotation(1.0), table(t), index(initindex)
{
    inplay = true;

	movement_remaining = 0.0;

	switch(index)
	{
	case 0:
        if (state == intro)
        {
            // for the intro scene, send the cue ball down the table
            dx = 0.01;
            dz = max_speed / 4;
            break;
        }


	default:
        dx = dz = 0.0;
		
		//dx = rand()%(max_speed/2) - max_speed/4;
		
		//dz = rand()%(max_speed/2) - max_speed/4;

		break;
	}

    if (PoolBall::gluq == NULL)
    {
        PoolBall::gluq = gluNewQuadric();
        gluQuadricNormals(PoolBall::gluq, GLU_SMOOTH);
        gluQuadricTexture(PoolBall::gluq, GL_TRUE);
    }

	switch(mode)
	{
	case grilliards:
		tex = textures[NUM_BEEF];
		name = bally[NUM_BEEF].name;
		mass = 0.8;
		break;

	default:
		tex = textures[index % NUM_TEXTURES];
		mass = use_mass ? bally[index % NUM_TEXTURES].mass : 1;
		name = bally[index % NUM_TEXTURES].name;
		break;
	}
}

// destructor
PoolBall::~PoolBall()
{
}



// draw the pool ball!
void PoolBall::render(glm::dmat4 &parent_model)
{
	glEnable(GL_TEXTURE_2D);

	glMaterialfv(GL_FRONT, GL_SPECULAR, sunset);
	glMaterialf (GL_FRONT, GL_SHININESS, 115);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // default really
    glEnable(GL_COLOR_MATERIAL);

	glColor3ub(255, 255, 255);

	if (tex)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
	}

	glm::dmat4 model = parent_model * glm::translate(glm::dmat4(1.0), glm::dvec3(x, diameter/2, z)) * rotation * glm::scale(glm::dmat4(1.0), glm::dvec3(diameter/2, diameter/2, diameter/2));

	glm::mat4 MVP;
	MVP << (perspective * view * model);

	//NaNtest(perspective);
	//NaNtest(view);
	//NaNtest(model);
	//NaNtest(MVP);
	
	glm::vec4 test(0.0, 0.0, 0.0, 1.0);
	test = MVP * test;
	//printf("%lf, %lf, %lf, %lf\n", test[0], test[1], test[2], test[3]);

	glErrorCheck();
	
	glUseProgram(program_id); glErrorCheck();

	glUniformMatrix4fv(MVP_id, 1, 0, &MVP[0][0]); glErrorCheck();
	glUniform1i(glGetUniformLocation(program_id, "tex_id"), 0); glErrorCheck();	
		
	glBindVertexArray(VAO_id); glErrorCheck();

	glEnableVertexAttribArray(0); glErrorCheck();
	//glBindBuffer(GL_ARRAY_BUFFER, PoolBall::vertexbuffer); glErrorCheck();
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); glErrorCheck();
	
	glEnableVertexAttribArray(1); glErrorCheck();
	//glEnableVertexAttribArray(2);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size()); glErrorCheck();
	
	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	//glDisableVertexAttribArray(2);
	
	glUseProgram(0); glErrorCheck();
	
	glDisable(GL_TEXTURE_2D);
}

// distance squared to another ball
double PoolBall::distance_squared(PoolBall& other)
{
	return pow(x-other.x, 2) + pow(z-other.z, 2);
}

// for qsort:
PoolBall *current_ball = NULL;
int PoolBall_distance_cmp(const void *a, const void *b)
{
    if (current_ball == NULL) return 0;
    PoolBall *A = (PoolBall*)a;
    PoolBall *B = (PoolBall*)b;

    double distance = current_ball->distance_squared(*A) - current_ball->distance_squared(*B);
	if (distance < 0.0) return -1;
	if (distance > 0.0) return 1;
	return 0; // unlikely
}

// slow your roll
void PoolBall::friction(double time_fraction)
{
	if (dx == 0.0 && dz == 0.0) return;

    dx *= 1.0-(time_fraction*rolling_friction/* *mass */); // forget mass in this calculation, it's unfun
    dz *= 1.0-(time_fraction*rolling_friction/* *mass */);

    if ((dx*dx + dz*dz) < 0.025) dx = dz = 0.0;
}


//
// PoolTable!
//
class PoolTable
{
public:
	int ballsleft;

	void clear_balls()
	{
		if (balls > 0)
		{
			for (int i = 0; i < balls; ++i)
			{
				if (ball[i] != NULL) delete ball[i];
				ball[i] = NULL;
			}
		}
	}

    PoolTable()
    {
        fprintf(stderr, "PoolTable() reporting...\n");

        balls = 0; 
		memset(ball, 0, sizeof(void*)*maxballs);
    }


    static const double width; // these two are important
    static const double length;
    static const double height; // but this is for visuals, no gameplay effect

	static const int maxballs = 20;


    GLfloat shadowmap[138*2][138]; // bleh, just... these are obviously width and length but we need int constants. It just has to fit the table dimensions.
    // TODO, replace with real shadows :/

	// PoolTable::render()
    void render(glm::dmat4 &parent_model)
    {

		glDisable(GL_TEXTURE_2D);

        glMaterialf(GL_FRONT, GL_SHININESS, 110.0); // not too shiny! hurgh. 0 is max shininess, 128 is min.

        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_grey);

        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // default really
        glEnable(GL_COLOR_MATERIAL);
               

        glColor3ub(63, 255, 63);

        //
        // stupid fake shadows :P
        // I don't think I have time to implement anything proper.
        // 

        memset((void*)shadowmap, 0, sizeof(GLfloat)*138*138*2);

        int sx, sz;
        for (int i = 0; i < balls; ++i)
        {
            if (!ball[i]->inplay) continue;
            double bx = ball[i]->x + width/2;
            double bz = ball[i]->z + length/2;

            for (int x = 0; x < (PoolBall::diameter+1)/2; ++x)
            {
                for (int z = 0; z < (PoolBall::diameter+1)/2; ++z)
                {
                    double d = x*x + z*z;

                    d -= (PoolBall::diameter * PoolBall::diameter * 0.4);

                    if (d > 0.0) continue;

                    d /= -(PoolBall::diameter * PoolBall::diameter);

                    d += 0.01;
                    if (d > 1.0) d = 1.0;

                    sx = bx+x; sz = bz+z;
                    if (sx > 0 && sx < 138 && sz > 0 && sz < 138*2)
                        shadowmap[sz][sx] = d;

                    sx = bx-x; sz = bz+z;
                    if (sx > 0 && sx < 138 && sz > 0 && sz < 138*2)
                        shadowmap[sz][sx] = d;
                    
                    sx = bx-x; sz = bz-z;
                    if (sx > 0 && sx < 138 && sz > 0 && sz < 138*2)
                        shadowmap[sz][sx] = d;
                   
                    sx = bx+x; sz = bz-z;
                    if (sx > 0 && sx < 138 && sz > 0 && sz < 138*2)
                        shadowmap[sz][sx] = d;
                }
            }
        }

#if 0
		// this is a really quick way of drawing a slab of the proper dimensions but the polygons won't get lit up well by positional lights
        glScaled(width, 10.0, length);
        glutSolidCube(1.0);
		//glutWireCube(1.0);
#endif
		// glTranslated(-width/2, 5, -length/2);
		glm::dmat4 model = parent_model * glm::translate(glm::dmat4(1.0), glm::dvec3(-width/2, 5.0, -length/2));


        // this draws a table surface out of ~38000 triangles which is really
        // pretty stupid. The correct approach would be to use two triangles
        // and a good fragment shader. That's something for the next level
        // of the class, though, right?
#if 0
		for(int x = 0; x < width-1; x++)
		{
			glBegin(GL_TRIANGLE_STRIP); // GL_BACON_STRIP, mmm, delicious
			glNormal3d(0, 1, 0);
			for(int z = 0; z < length; z++)
			{
                // OH YEAH, VERTEX-BASED SHADOWS LIKE IT'S 1995!
                GLfloat s = 1-shadowmap[z][x+1];
                glColor3f(0.25*s, 1.0*s, 0.25*s);
				glVertex3d(x+1, 0, z);

                s = 1-shadowmap[z][x];
                glColor3f(0.25*s, 1.0*s, 0.25*s);
				glVertex3d(x, 0,   z);
			}
			glEnd();
		}
#endif


        //glDisable(GL_COLOR_MATERIAL);

		// draw "pockets"
		// except the table should really be a mesh that we load and I'm not spending a lot of time on these :/

        glColor3ub(255, 255, 0);
#if 0        
        glPushMatrix();
        glTranslated(width/2, 10-PoolBall::diameter*2, 0);
        glutSolidCube(PoolBall::diameter*2);
        glPopMatrix();
        
        glPushMatrix();
        glTranslated(-width/2, 10-PoolBall::diameter*2, 0);
        glutSolidCube(PoolBall::diameter*2);
        glPopMatrix();

        glPushMatrix();
        glTranslated(width/2, 10-PoolBall::diameter*2, length/2);
        glutSolidCube(PoolBall::diameter*2);
        glPopMatrix();

        glPushMatrix();
        glTranslated(-width/2, 10-PoolBall::diameter*2, length/2);
        glutSolidCube(PoolBall::diameter*2);
        glPopMatrix();

        glPushMatrix();
        glTranslated(width/2, 10-PoolBall::diameter*2, -length/2);
        glutSolidCube(PoolBall::diameter*2);
        glPopMatrix();

        glPushMatrix();
        glTranslated(-width/2, 10-PoolBall::diameter*2, -length/2);
        glutSolidCube(PoolBall::diameter*2);
        glPopMatrix();

#endif
        // 
        // draw the balls
        //

		model = parent_model * glm::translate(glm::dmat4(1.0), glm::dvec3(0, 5+ ball[0]->diameter, 0)); 

		if (balls > 0)
		{
			ballsleft = NUM_TEXTURES;

			//glTranslated(0, 5 + ball[0]->diameter, 0);

			for (int i = 0; i < balls; ++i)
			{
				if (ball[i]->inplay) ball[i]->render(model);
				else --ballsleft;
			}

			ballsleft--; // don't count the cue ball, eh

			if (!ballsleft)
				state = over;
		}
    }

	void update(void)
	{
		for (int i = 0; i < balls; ++i)
		{
			ball[i]->movement_remaining += 1.0;
		}

		for (int i = 0; i < balls; ++i)
		{
			ball[i]->move(animation_time - old_animation_time);
			ball[i]->friction(animation_time - old_animation_time);
		}
	}

	int balls;

	PoolBall *ball[maxballs];

	friend class PoolBall;
    friend class PoolGame;
};

const double PoolTable::width = 137; // 4.5 feet = 137.16cm
const double PoolTable::length = PoolTable::width*2;
const double PoolTable::height = 10;


// this is to be called when the ball has been repositioned to the point of collision with a wall
void PoolBall::test_pocket()
{
	bool out = false;

	if (state == intro || state == over || state == menu || state == help) return;

	if (abs(z) < diameter) out = true; // side pockets
	if (abs(z) > table->length/2-diameter && abs(x) > table->width/2-diameter) out = true; // corner pockets... too big? *shrug*

	if (out)
	{
		if (index == 0)
		{
			// scratched the cue ball
			state = over;
			inplay = false;
			sadwhistle();
		} else
		{
			squirble();
			inplay = false;
		}
	}

}



// called for the ball to update its stupid position and crash into other stupid things
// this needs the full declaration of PoolTable to function, hence this location....
void PoolBall::move(double base_time_fraction)
{
	if (movement_remaining == 0.0) return;

	// sanity check...
	if (dx*base_time_fraction > table->length || dz*base_time_fraction > table->length) 
	{
		fprintf(stderr, "velocity insanity.\n");
		return;
	}

	if (!inplay) return;

	double time_fraction = base_time_fraction * movement_remaining;

	double fdx = dx * time_fraction;
	double fdz = dz * time_fraction;

	// test for collisions with the walls
	// this uses some algorithms based on the collision of a ray with a plane, in 2D
	// the original 3D formula is here: http://www.cs.princeton.edu/courses/archive/fall00/cs426/lectures/raycast/sld017.htm
    // or here: http://cgafaq.info/wiki/Ray_Plane_Intersection
    // or dozens of other pages on the net, some with fewer typos than others

	// though they're not really specific to any number of dimensions...

	// t = N·(Q-E) / N·D
	// where N is a normal vector of the plane, Q is a point on the plane, 
    // E is the ray origin,
	// and D is the ray's direction vector
	// if t < 0, the plane is behind the origin
	// if N·D=0, the ray is parallel to the plane
	// if t >=0, the intersection point is E + tD

	double NdotD, numerator, tx, tz;

    double Qx, Qz;
    double Nx, Nz;

    if (fdx > 0.0)
    {
        // test distance to right wall
        Qx = table->width/2;
        Qz = 0;

        Nx = -1;
        Nz = 0;

        NdotD = dotProduct(Nx, Nz, fdx, fdz);
        numerator = dotProduct(Nx, Nz, Qx-(x+diameter/2), Qz-z);

        // x+diameter/2 rather than just x because we're testing the collison
        // of the surface of the ball rather than its center

        tx = numerator / NdotD;
    } else if (fdx < 0.0)
    {
        // try the left wall
        Qx = -table->width/2;
        Qz = 0;

        Nx = 1;
        Nz = 0;

        NdotD = dotProduct(Nx, Nz, fdx, fdz);
        numerator = dotProduct(Nx, Nz, Qx-(x-diameter/2), Qz-z);

        tx = numerator / NdotD;
    } else
        tx = 1000000.0; // just some impossibly-large number


    if (fdz > 0.0)
    {
        // test distance to near wall
        Qx = 0;
        Qz = table->length/2;

        Nx = 0;
        Nz = -1;

        NdotD = dotProduct(Nx, Nz, fdx, fdz);
        numerator = dotProduct(Nx, Nz, Qx-x, Qz-(z+diameter/2));

        // x+diameter/2 rather than just x because we're testing the collison
        // of the surface of the ball rather than its center

        tz = numerator / NdotD;
    } else if (fdz < 0.0)
    {
        // try the far wall
        Qx = 0;
        Qz = -table->length/2;

        Nx = 0;
        Nz = 1;


        NdotD = dotProduct(Nx, Nz, fdx, fdz);
        numerator = dotProduct(Nx, Nz, Qx-x, Qz-(z-diameter/2));

        tz = numerator / NdotD;
    } else
        tz = 1000000.0; // just some impossibly-large number


    // the ball will be closer to one of the sides than the other, probably
    if (tx < tz)
    {
        if (tx < 1.0)
        {
            //fprintf(stderr, "tx==%g\n", tx);
            double t_rest = 1.0 - tx;
            // {x,y} + {fdx,fdy} * tx = the point where the ball hits the table

            actually_reposition(fdx*tx, fdz*tx);
			test_pocket();

            dx *= -1; // BOUNCE! :D

			movement_remaining = t_rest;

            return move(time_fraction);
        }
    } else
    {
        if (tz < 1.0)
        {
            //fprintf(stderr, "tz==%g\n", tz);
            double t_rest = 1.0 - tz;
            // {x,y} + {fdx,fdy} * tx = the point where the ball hits the table

            actually_reposition(fdx*tz, fdz*tz);
			test_pocket();

            dz *= -1; // BOUNCE! :D

			movement_remaining = t_rest;

            return move(time_fraction);
        }
    }
    // wall code done


    // test for collisions with other balls
    // I found this to be very helpful: 
    // http://twobitcoder.blogspot.com/2010/04/circle-collision-detection.html

    double Vabx, Vabz;
    double Pabx, Pabz;
    long double a, b, c;

    // sort the balls by distance, check nearest for collisions first
    PoolBall *sorted[table->maxballs];
    for (int r = 0, w = 0; r < table->maxballs; ++w, ++r) sorted[w] = table->ball[r]; 
    current_ball = this; // for PoolBall_distance_cmp(); see its implementation
    qsort(sorted, table->balls, sizeof(PoolBall*), PoolBall_distance_cmp);

    for (int i = 0; i < table->balls; ++i)
    {
        if (sorted[i]->index == index) continue; // could probably just start at index 1 but let's not make this confusing...
        // test whether other ball has already run move() this frame??? XXX

        if (!(sorted[i]->inplay)) continue; // Don't collide with balls in pockets...

		Pabx =  x - sorted[i]-> x; // vector in the direction of the other ball? or whatever? derp
        Pabz =  z - sorted[i]-> z;

		// sanity-check current distance!
		if ((Pabx*Pabx + Pabz*Pabz) < diameter*diameter)
		{
			//fputs("unfortunate intersection.\n", stderr);
			handle_collision(*(sorted[i]), 0.0, time_fraction);
			fdx = dx * time_fraction;
			fdz = dz * time_fraction; 
			//break; // Just stop doing things, algorithm. Clearly you're not good at them.
		}


        Vabx = fdx - sorted[i]->dx*time_fraction; // relative velocity vector
        Vabz = fdz - sorted[i]->dz*time_fraction;
		


        if (abs(Vabx) < 0.00000001) Vabx = 0.0;
        if (abs(Vabz) < 0.00000001) Vabz = 0.0;

		if (Vabx == 0.0 && Vabz == 0.0) continue; // no relative motion? clearly not going to collide, huh.

        a = dotProduct(Vabx, Vabz, Vabx, Vabz);
        b = 2 * dotProduct(Pabx, Pabz, Vabx, Vabz);
        c = dotProduct(Pabx, Pabz, Pabx, Pabz) - diameter*diameter;
        
		// b^2-4ac is the discriminant of the quadratic polynomial
        if ((b*b - 4*a*c) >= 0.0)
        {
            // we have one or two roots
            long double t0, t1, t;

            t0 = quadratic_formula0(a, b, c);
            t1 = quadratic_formula1(a, b, c);

			if (t0 < 0.0 && t1 < 0.0) { 
				//puts("past collision?"); 
				continue; 
			}
			
			if (t0 < 0.0) t = t1;
			else if (t1 < 0.0) t = t0;
			else if (t0 < t1) t = t0;
			else t = t1;

            //printf("%g, %g\n", t1, t0);

            if (t >= 0.0  && t <= 1.0)
            {
				/*
                printf("OMG bonk: %g, %g?!\n", (double)t, (double)
                    (pow((x+t*fdx) - (sorted[i]->x + t * sorted[i]->dx * time_fraction), 2) + 
                    pow((z+t*fdz) - (sorted[i]->z + t * sorted[i]->dz * time_fraction), 2) -
                    pow(diameter, 2)));
				*/

				handle_collision(*(sorted[i]), t, time_fraction);
				time_fraction = base_time_fraction * movement_remaining; // movement_remaining was updated by handle_collision()... 
				fdx = dx * time_fraction;
				fdz = dz * time_fraction; // now we can keep looping without recursing, yes?
            }
        }
    }

	movement_remaining = 0.0;

    actually_reposition(fdx, fdz); // no collisions, simple case
}

    
void PoolBall::actually_reposition(double fdx, double fdz)
{
    x += fdx;
    z += fdz;

	double distance = sqrt(fdx*fdx + fdz*fdz);
	if (distance < std::numeric_limits<double>::epsilon()) return; 
	// a distance of ~0 results in a bogus axis and an angle of ~0, which fill the rotation matrix with NaNs

	glm::dvec3 axis(fdz, 0, -fdx);
	rotation = glm::rotate(glm::dmat4(1.0), 360.0 * (distance /(M_PI * diameter)), glm::normalize(axis)) * rotation;
}


// Calculate the consequences of a collision between two pool balls.
// It's handled like a 2D elastic collision between two circles, really;
// Sorry, no jumping the cue ball at this point.
// The long double precision is unnecessary; it's just there because I 
// was trying to eliminate potential precision errors while debugging 
// a really stupid typo in the collision detection code and t ended 
// up a long double.
//
// thanks to http://www.vobarian.com/collisions/2dcollisions2.pdf
// for the algorithm and sample code
//
// other is the other ball; it will be moved into the position of collision
//
// t*time_fraction is the multiplier for dx,dz to move the balls into the position where they collide
// t may be 0.0 if some balls intersected :/
//
//
// Note that this algorithm moves the balls immediately and then 
// updates movement_remaining. A more sound implementation would
// make a list of all the possible collisions, wind time to the first
// collision, update that, and recalculate everything. However, this
// is not meant to be a totally rigorous physics simulation. 
// We don't need all that. We need just enough physics to have fun.
//
void PoolBall::handle_collision(PoolBall& other, long double t, double time_fraction)
{
	if (mass == 0.0 || other.mass == 0.0) return; // ephemeral entities do not collide

	// roll balls to the point of impact
	if (t != 0.0 && time_fraction != 0.0)
	{
		actually_reposition(dx*t*time_fraction, dz*t*time_fraction);
		movement_remaining -= t;

		other.actually_reposition(other.dx*t*time_fraction, other.dz*t*time_fraction);
		other.movement_remaining -= t;
	}

	// unit normal and unit tangent vectors
	double uNx = other.x - x; // normal
	double uNz = other.z - z;

	double mag = sqrt(uNx*uNx + uNz*uNz); 

	uNx /= mag; // normalized (unit) normal
	uNz /= mag;

	// untangle stupid-ass intersected spheres. ugh!
	if (abs(t) < std::numeric_limits<double>::epsilon())
	{
		other.actually_reposition((diameter-mag) * uNx * 1.0001, (diameter-mag) * uNz * 1.0001);
		return handle_collision(other, -1.0, 0); // uNz, uNz, etc. have to be recalculated
	}



	double uTx = -uNz; // unit tangent
	double uTz = uNx; 

	// scalar projections of velocities onto above vectors
	double v1n = dotProduct(uNx, uNz, dx, dz);
	double v1t = dotProduct(uTx, uTz, dx, dz);
	double v2n = dotProduct(uNx, uNz, other.dx, other.dz);
	double v2t = dotProduct(uTx, uTz, other.dx, other.dz);

	// tangential velocities don't change in a purely elastic collision

    // apply coefficient of restitution to the velocity projections
    v1n *= COrestitution;
    v2n *= COrestitution;

	// normal velocities do:
	double v1n_prime = (v1n * (mass - other.mass) + 2.0 * other.mass * v2n) / (mass + other.mass);
	double v2n_prime = (v2n * (other.mass - mass) + 2.0 * mass       * v1n) / (mass + other.mass);


	// the new velocity vectors are vXn_prime * uN + vXt * uT

	dx = v1n_prime * uNx + v1t * uTx;
	dz = v1n_prime * uNz + v1t * uTz;

	other.dx = v2n_prime * uNx + v2t * uTx;
	other.dz = v2n_prime * uNz + v2t * uTz;


    // play a collision sound
	Mix_PlayChannel(index%8, Sounds::ballclack, 0);
	Mix_Volume(index%8, (int) (64.0 * ((abs(v1n_prime) + abs(v2n_prime)) / (max_speed*2.0)))); // scale the volume to the total normal velocity relative to max speed that two balls could have
	// 128 is max channel volume but it's rude to blast max volume at people
	//next_channel();
}

// first 6 arguments to gluLookAt()
double wide_view[6] = {200, 150, 0, 0, 0, 0};




void idle1(void)
{
	table->update();
}

// currently selected menu item
int menu_option = STARTGAME;


void beep()
{
	Mix_PlayChannel(next_sound_channel, Sounds::beepverb, 0);
	Mix_Volume(next_sound_channel, 64); 
    next_channel();
}

void squirble()
{
	Mix_PlayChannel(next_sound_channel, Sounds::squirble, 0);
	Mix_Volume(next_sound_channel, 32); 
    next_channel();
}

void sadwhistle()
{
	Mix_PlayChannel(next_sound_channel, Sounds::sadwhistle, 0);
	Mix_Volume(next_sound_channel, 64); 
    next_channel();
}



void menu_keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 13:
            beep();
            switch(menu_option)
            {
            case STARTGAME:
                newgame();
                state = cue;
                glutKeyboardFunc(keyboard1);
                glutSpecialFunc(special1);
                glutPostRedisplay();
                break;

            case CONFUSED:
                state = help;
                glutSpecialFunc(special1);
                glutKeyboardFunc(keyboard1);
                glutPostRedisplay();
                break;

			case GAMEMODE:
				switch(mode)
				{
				case amazeballs: mode = grilliards; break;
				default:		 mode = amazeballs; break;
				}
				glutPostRedisplay();
				break;

            case USEMASS:
                use_mass = !use_mass;
                glutPostRedisplay();
                break;

            case EXEUNT:
                glutLeaveMainLoop();
                break;

            default:
                break;
            }
            break;

        case 'w':
        case 'W':
            beep();
            menu_option--;
            if (menu_option < 0) menu_option = MAXMENUOPTION-1;
            break;

        case 's':
        case 'S':
            beep();
            menu_option++;
            menu_option %= MAXMENUOPTION;
            break;

        case 'n':
        case 'N':
            newgame();
            break;

        
        case 27:
            return base_key_down(key, x, y);
            break;

    }
}


void keyboard1(unsigned char key, int x, int y)
{
	if (state == confirm)
	{
		if (key == 'y' || key == 'Y')
		{
			state = over;
			key = SDLK_ESCAPE;
		} else
		{
			state = cue;
			return;
		}
	} 


    switch(key)
    {
    case SDLK_RETURN:
		if (state == cue) // enter hits the ball
		{
			table->ball[0]->dx = -cos(M_PI*cue_angle/180.0) * cue_power * max_speed;
			table->ball[0]->dz = -sin(M_PI*cue_angle/180.0) * cue_power * max_speed;
			hits++;
			state = watch;
			glutPostRedisplay();
			break;
		}

    case SDLK_ESCAPE:
        if (state == intro || state == help || state == over)
        {
            state = menu;
            glutKeyboardFunc(menu_keyboard);
            glutSpecialFunc(menu_special);
            glutPostRedisplay(); 
            break;
        }

		if (state == cue && key == SDLK_ESCAPE)
		{
			state = confirm;
			glutPostRedisplay();
			break;
		}

        if (state == watch)
        {
            state = cue;
            break;
        }

        break;

    case 'A':
    case 'a':
        camera_angle -= 10;
        glutPostRedisplay();
        break;

    case 'D':
    case 'd':
        camera_angle += 10;
        glutPostRedisplay();
        break;

    case 'W':
    case 'w':
		cue_power += 0.1f;
		if (cue_power > 1.0f) cue_power = 1.0f;
		glutPostRedisplay();
		break;

    case 'S':
    case 's':
		cue_power -= 0.05f;
		if (cue_power < 0.0f) cue_power = 0.0f;
		glutPostRedisplay();
        break;


    case 'Q':
    case 'q':
        cue_angle -= 1;
        break;


    case 'E':
    case 'e':
        cue_angle += 1;
        break;
 

	case 'n':
	case 'N':
		newgame();
		break;


    case '?':
        state = help;
        break;


    case 'c':
    case 'C':
        chasecam = !chasecam;
        glutPostRedisplay();
        break;

    case 'x':
    case 'X':
        chasecam = false;
        viewangle = 45.0;
        camera_angle = 0;
		winReshapeFcn(winWidth, winHeight);
        glutPostRedisplay();
        break;


// + and - zoom in and out
    case '+':
    case '=':
        viewangle -= 3;
        if (viewangle < 10) viewangle = 10;
        winReshapeFcn(winWidth, winHeight);
        glutPostRedisplay();
        break;
    
    case '-':
    case '_':
        viewangle += 3;
        if (viewangle > 110) viewangle = 110;
        winReshapeFcn(winWidth, winHeight);
        glutPostRedisplay();
        break;


    default:
        return base_key_down(key, x, y);
    }
}

void menu_special(int key, int x, int y)
{
    switch(key)
    {
        case 100: return menu_keyboard('a', x ,y);
        case 101: return menu_keyboard('w', x ,y);
        case 102: return menu_keyboard('d', x ,y);
        case 103: return menu_keyboard('s', x ,y);
        default:
        break;
    }


    fprintf(stderr, "Very special key: %d\n", key);
}

void special1(int key, int x, int y)
{
    switch(key)
    {
        case 100: return keyboard1('a', x ,y);
        case 101: return keyboard1('w', x ,y);
        case 102: return keyboard1('d', x ,y);
        case 103: return keyboard1('s', x ,y);
        default:
        break;
    }


    fprintf(stderr, "Very special key: %d\n", key);
}



void display1(void)
{
	int notzero = 1;


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

	//
	// lights
	//
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1); // two dingy lights hanging over our table
	//glEnable(GL_LIGHT2);


	//glLightfv(GL_LIGHT2, GL_AMBIENT, dim_ambiance);

    glLightfv(GL_LIGHT0, GL_DIFFUSE, tungsten_100w);
    glLightfv(GL_LIGHT0, GL_SPECULAR, tungsten_100w);
	glLightf (GL_LIGHT0, GL_SPOT_CUTOFF, 90.0f);
	glLightf (GL_LIGHT0, GL_SPOT_EXPONENT, 2.0); // spot fades out to edges
	
    glLightfv(GL_LIGHT1, GL_DIFFUSE, tungsten_100w);
    glLightfv(GL_LIGHT1, GL_SPECULAR, tungsten_100w);
	glLightf (GL_LIGHT1, GL_SPOT_CUTOFF, 90.0f);
	glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 5.0); // spot fades out to edges




	// 
	// light attenuation
	// (sucks, better to fake it with spotlights)
	//glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0); 
	//glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0);

	//glLightf (GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0009);
	//glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0009);



	//
	// light model
	//
	notzero = GL_SEPARATE_SPECULAR_COLOR;
	glLightModeliv(GL_LIGHT_MODEL_LOCAL_VIEWER, &notzero); 
	glLightModeliv(GL_LIGHT_MODEL_COLOR_CONTROL, &notzero);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, dim_ambiance);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    static int ballfollowed = 0;
    static int countdown = 10;

    if (chasecam)
    {
        double x, z, energy=-1;

        x=table->ball[ballfollowed]->x;
        z=table->ball[ballfollowed]->z;

        // find the ball with the most energy
        if (state != intro && state != cue && (frames == 3 || energy<0.0)) 
        {
            for (int i = 0; i < table->balls; ++i)
            {
                if (!table->ball[i]->inplay) continue;
                double ener_i = table->ball[i]->mass * (pow(table->ball[i]->dx,2) + pow(table->ball[i]->dz,2));
                if (ener_i > energy)
                {
                    x = table->ball[i]->x;
                    z = table->ball[i]->z;
                    energy = ener_i;
                    ballfollowed = i;
                }
            }
        }

        if (state == intro && frames == 3)
        {
            ballfollowed++;
            if (ballfollowed == NUM_TEXTURES) 
            {
                countdown = ballfollowed;
                ballfollowed = 0;
                chasecam = false;
            }
        }

		if (state == cue) ballfollowed = 0;

        if (state == cue || !running)
        {
            // manual control
            view = glm::lookAt( glm::dvec3(-sin(M_PI*camera_angle/180.0)*66+x, 50, -cos(M_PI*camera_angle/180.0)*66+z), 
                        glm::dvec3(x, -5, z),
                        glm::dvec3(0, 1, 0));
        } else
        {
            // look on
            view = glm::lookAt( glm::dvec3(0, 50, 0),
                        glm::dvec3(x, -5, z),
                        glm::dvec3(0, 1, 0));
        }
            
    } else 
    {

		look_at(wide_view);
        if (state == intro || state == over || state == menu)
        {
            glRotatef((animation_time+time_offset)*10, 0, 1, 0);
        } else
        {
            glRotatef(camera_angle, 0, 1, 0);
        }
        if (state == intro && frames == 3)
        {
            printf("%d\n", countdown);
            if (!(countdown--)) chasecam = true;
        }
    }

	//
	// Set light positions
	// 


	//glPushMatrix();
	//glTranslatef(light_position[0], light_position[1], light_position[2]);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	//glPopMatrix();

	//glPushMatrix();
	//glTranslatef(light_position1[0], light_position1[1], light_position1[2]);
    glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
	//glPopMatrix();

	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, v_down);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, v_down);

	//
	// Draw scene
	//

	glm::dmat4 model = glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, -10.0, 0.0));
    //glTranslated(0, -10, 0);

    table->render(model); // our table and balls!

    // cue "stick"
	// 

	if (state == cue)
	{
		glDisable(GL_TEXTURE_2D);
		glColor3ub(255, 63, 0);
		glBegin(GL_LINES);
		glVertex3f(table->ball[0]->x, -PoolBall::diameter/2, table->ball[0]->z);
		glVertex3f(table->ball[0]->x + 50*cos(M_PI*cue_angle/180.0), -PoolBall::diameter/2, table->ball[0]->z + 50*sin(M_PI*cue_angle/180.0));
		glEnd();
	}


	glPushMatrix();
	glTranslated(light_position[0], light_position[1], light_position[2]);
    glRotatef(90, 1, 0, 0);
	glutWireCylinder(5, 10, 6, 1);
	glPopMatrix();

	glPushMatrix();
	glTranslated(light_position1[0], light_position1[1], light_position1[2]);
    glRotatef(90, 1, 0, 0);
	glutWireCylinder(5, 10, 6, 1);
	glPopMatrix();

    // text overlay stuff
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0,(GLdouble) winWidth , 0.0,(GLdouble) winHeight);

    glColor3ub(128, 1, 0);
    if (chasecam)
    {
        glRasterPos2i(winWidth/2,20);
        glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)table->ball[ballfollowed]->name);
    }

    glColor3ub(128, 128, 128);
    glRasterPos2i(0, winHeight-40);
    char *tag;
    switch(state)
    {
        case intro: tag="[intro]"; break;
        case help:  tag="[help]"; break;
        case menu:  tag="[menu]"; break;
        case cue:   tag="[cue]"; break;
        case watch: tag="[balls rolling]"; break;
        case over:  tag="[game over]"; break;
		case confirm: tag="[confirm]"; break;
        case achiev:tag="[really?]"; break;
        default:    tag="[???????]"; break;
    }

    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)tag);

	if (state != intro)
	{
		char temp_str[64];
		glRasterPos2i(0, winHeight-60);
#if  defined(_WIN32) && !defined(__MINGW32__)
				sprintf_s((char*)temp_str, 63, "%d balls remaining.", table->ballsleft);
#else
				sprintf((char*)temp_str, "%d balls remaining.", table->ballsleft);
#endif
		glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
			temp_str);
	}
	
	if (state == watch)
	{
		glRasterPos2i(0, winHeight-80);
		glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)"press Enter any time");
	}


	if (state == cue)
	{
		// draw cue power indicator
        glEnable (GL_BLEND); 
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glErrorCheck();
        glColor4f(cue_power, 1-cue_power, 0.1, 0.7);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(40, winHeight*0.8*cue_power);
        glVertex2f(40, 40);
        glVertex2f(70, 40);
        glVertex2f(70, winHeight*0.8*cue_power);
        glEnd();

		glBegin(GL_LINE_LOOP);
        glVertex2f(40, winHeight*0.8);
        glVertex2f(40, 40);

        glVertex2f(70, 40);
        glVertex2f(70, winHeight*0.8);
		glEnd();
		glDisable(GL_BLEND);
		glErrorCheck();
	}


    if (state == intro || state == menu || state == help || state == over || state == confirm)
    {
		char hits_str[64];

        glEnable (GL_BLEND); 
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.1, 0.1, 0.1, 0.7);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(40, 240);
        glVertex2f(40, 40);
        glVertex2f(300, 40);
        glVertex2f(300, 240);
        glEnd();
        glColor4f(0.7, 0.7, 0.6, 1.0);

        glRasterPos2i(42,50);
        switch(state)
        {
        case intro:
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "hit Enter");
            glRasterPos2i(42, 220);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "a strange pool game");
            glRasterPos2i(42, 200);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "by greg velichansky");

            break;

        case menu:
            // draw selection box
            glColor4f(1, 1, sunset[2], 0.3);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(42, 238-(menu_option*20));
            glVertex2f(42, 218-(menu_option*20));
            glVertex2f(300, 218-(menu_option*20));
            glVertex2f(300, 238-(menu_option*20));
            glEnd();
            glColor4f(0.7, 0.7, 0.6, 1.0);

            glRasterPos2i(42, 220);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "New Game");
            glRasterPos2i(42, 200);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Halp?");
            glRasterPos2i(42, 180);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Game Type: ");
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                ((mode == grilliards)?"Grilliards!" : "Amazeballs!"));
			if (mode == amazeballs)
			{
				glRasterPos2i(42, 160);
				glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
					"With Option: ");
				glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
					(use_mass ? "Varied Mass" : "Uniform Mass"));
			}
            glRasterPos2i(42, 140);
            //glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                //"");
            glRasterPos2i(42, 120);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Exit (Esc)");
            break;

        case help:
            glRasterPos2i(42, 220);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Buttons:");
            glRasterPos2i(42, 200);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "A,D to rotate camera");
            glRasterPos2i(42, 180);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "W,S to set cue stick power");
            glRasterPos2i(42, 160);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Q,E to aim cue stick");
            glRasterPos2i(42, 140);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "C,+,- to change camera");
            glRasterPos2i(42, 120);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "X to reset camera");
            glRasterPos2i(42, 100);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "N to restart");
            glRasterPos2i(42, 80);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Spacebar to pause");
            glRasterPos2i(42, 60);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "when in doubt, hit Enter");
            break;

		case confirm:
			glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Really quit? (Y/N)");
			break;

		case over:
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "hit Enter");
            glRasterPos2i(42, 220);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Game Over!");
            glRasterPos2i(42, 200);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "You ");
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
				(table->ball[0]->inplay ? "won! :D" : "scratched. :("));

#if  defined(_WIN32) && !defined(__MINGW32__)
	        sprintf_s((char*)hits_str, 63, "It took %d hits.", hits);
#else
			sprintf((char*)hits_str, "It took %d hits.", hits);
#endif

            glRasterPos2i(42, 180);
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                hits_str);
			break;
			


			

        default:
            glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)
                "Hmm?");
        }

        glDisable (GL_BLEND);
    }

	displayFPSOverlay(true);

    //glFinish();
    glutSwapBuffers();
}




void gogogo()
{
	load_assets();

    if (table == NULL) 
	{
		table = new PoolTable();
		newgame();
	}

    glutDisplayFunc(&display1);
    glutKeyboardFunc(&keyboard1);
    glutSpecialFunc(&special1);
	finalIdleFunc = &idle1;
}


void load_assets()
{
	if (textures[0] == -1)
	{
		for (int i = 0; i < NUM_TEXTURES; ++i)
		{
			textures[i] = send_one_texture(bally[i].texture);
		}
	}

	if (!Sounds::ballclack) Sounds::ballclack = Mix_LoadWAV("data/lonepoolballhit.wav"); // we want clacking balls!
	if (!Sounds::ballclack) fprintf(stderr, ":( %s \n", SDL_GetError());

    if (!Sounds::beepverb) Sounds::beepverb = Mix_LoadWAV("data/beepverb.wav");
	if (!Sounds::beepverb) fprintf(stderr, ":( %s \n", SDL_GetError());

    if (!Sounds::squirble) Sounds::squirble = Mix_LoadWAV("data/lowsquirble.wav");
	if (!Sounds::squirble) fprintf(stderr, ":( %s \n", SDL_GetError());

	if (!Sounds::sadwhistle) Sounds::sadwhistle = Mix_LoadWAV("data/sadwhistle.wav");
	if (!Sounds::sadwhistle) fprintf(stderr, ":( %s \n", SDL_GetError());
	
	// refactor into a "load model" method or something...
	loadOBJ("icosphere.obj", PoolBall::vertices, PoolBall::uvs, PoolBall::normals); 
	
	glErrorCheck();
	
	glGenVertexArrays(1, &PoolBall::VAO_id); glErrorCheck();
	glBindVertexArray(PoolBall::VAO_id); glErrorCheck();
	
	glGenBuffers(1, &PoolBall::vertexbuffer); glErrorCheck();
	glBindBuffer(GL_ARRAY_BUFFER, PoolBall::vertexbuffer); glErrorCheck();
	glBufferData(GL_ARRAY_BUFFER, PoolBall::vertices.size() * sizeof(glm::vec3), &PoolBall::vertices[0], GL_STATIC_DRAW); glErrorCheck();
	glEnableVertexAttribArray(0); glErrorCheck();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); glErrorCheck();
	
	glGenBuffers(1, &PoolBall::uvbuffer); glErrorCheck();
	glBindBuffer(GL_ARRAY_BUFFER, PoolBall::uvbuffer); glErrorCheck();
	glBufferData(GL_ARRAY_BUFFER, PoolBall::uvs.size() * sizeof(glm::vec3), &PoolBall::uvs[0], GL_STATIC_DRAW); glErrorCheck();
	glEnableVertexAttribArray(1); glErrorCheck();
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); glErrorCheck();
	
	glGenBuffers(1, &PoolBall::normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, PoolBall::normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, PoolBall::normals.size() * sizeof(glm::vec3), &PoolBall::normals[0], GL_STATIC_DRAW);	
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	program_id = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader"); glErrorCheck();
	MVP_id = glGetUniformLocation(program_id, "MVP"); glErrorCheck();	
}


// reset all the things?
void newgame()
{
	hits = 0; // this is how we keep score

	table->clear_balls();

	//table->ball[table->balls++] = new PoolBall(table, 0, 0, 1, 0);
	//table->ball[table->balls++] = new PoolBall(table, 0, -20, 1, 1);

    table->ball[0] = new PoolBall(table, 0, -table->length/4, 1, 0);

	for (int i = 1, row = 0, j = 0; i < NUM_TEXTURES; ++i)
	{
		double z = table->length / 4 + row*PoolBall::diameter*0.8;

		double x = j * PoolBall::diameter - row * PoolBall::diameter / 2;

		table->ball[i] = new PoolBall(table, x, z, 1, i);

		if (!j) 
		{
			++row;
			j = row;
		} else --j;

	}

	table->balls = NUM_TEXTURES;
}

}; // namespace
