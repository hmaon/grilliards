#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL_mixer.h>

#include "final.h"
#include "PoolBall.h"
#include "PoolTable.h"
#define _USE_MATH_DEFINES
#include <math.h>


namespace PoolGame
{

// use GLUquadric for sphere objects
const double PoolBall::diameter = 5.7; // standard American pool balls are 57mm (=5.7cm)  err what
const double PoolBall::rolling_friction = 0.15;
const double PoolBall::COrestitution = 0.95;;

SimpleMesh PoolBall::mesh;


// constructor
PoolBall::PoolBall(PoolTable *t, double initx, double initz, double dummy, int initindex) : x(initx), z(initz), rotation(1.0), table(t), index(initindex)
{
    in_play = true;

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

    switch(mode)
    {
    case grilliards:
        tex = textures[NUM_BEEF];
        name = bally[NUM_BEEF].name;
        mass = 0.8;
        break;

    default:
        tex = textures[index % NUM_TEXTURED_BALLS];
        mass = use_mass ? bally[index % NUM_TEXTURED_BALLS].mass : 1;
        name = bally[index % NUM_TEXTURED_BALLS].name;
        break;
    }
}

// destructor
PoolBall::~PoolBall()
{
}

glm::vec3 PoolBall::fpos()
{
    return glm::vec3(x, diameter / 2, z);
}

glm::dvec3 PoolBall::dpos()
{
    return glm::dvec3(x, diameter / 2, z);
}

glm::vec4 PoolBall::fpos4()
{
    return glm::vec4(x, diameter / 2, z, 1);
}

// draw the pool ball!
void PoolBall::render(glm::dmat4 &parent_model)
{
    if (PoolGame::idSkipSphere == -1) perror("wtf");
    glUseProgram(idProgram);
    glUniform1i(PoolGame::idSkipSphere, index); glErrorCheck();
    glUniform1f(PoolGame::idSpecular, 0.9f); glErrorCheck();
    glm::dmat4 model = parent_model * glm::translate(glm::dmat4(1.0), dpos()) * rotation * glm::scale(glm::dmat4(1.0), glm::dvec3(diameter/2));

    mesh.render(tex, model);

#if 0
    glUseProgram(0); glErrorCheck();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixd(&view[0][0]);
    glMultMatrixd(&parent_model[0][0]);
    glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
extern double wide_view[];	
    //gluLookAt(eye[0], eye[1], eye[2], wide_view[3], wide_view[4], wide_view[5], 0, 1, 0);
    glLoadMatrixd(&perspective[0][0]);
    
    glEnable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
#if 0
    glVertex4f(x, 0, z, 1.0f);
    glVertex4f(light_position[0], light_position[1], light_position[2], 1.0f);
#endif
    
#if 0
    for (int i = 0; i < vertices.size(); ++i)
    {
        glVertex3f(vertices[i][0], vertices[i][1], vertices[i][2]);
        glVertex3f(vertices[i][0] + normals[i][0], vertices[i][1] + normals[i][1], vertices[i][2] + normals[i][2]);
    }
#endif
    glEnd(); glErrorCheck();
#endif
    
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
            in_play = false;
            sadwhistle();
        } else
        {
            squirble();
            in_play = false;
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

    if (!in_play) return;

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
    PoolBall *sorted[PoolTable::maxballs];
    for (int r = 0, w = 0; r < table->maxballs; ++w, ++r) sorted[w] = table->ball[r]; 
    current_ball = this; // for PoolBall_distance_cmp(); see its implementation
    qsort(sorted, table->balls, sizeof(PoolBall*), PoolBall_distance_cmp);

    for (int i = 0; i < table->balls; ++i)
    {
        if (sorted[i]->index == index) continue; // could probably just start at index 1 but let's not make this confusing...
        // test whether other ball has already run move() this frame??? XXX

        if (!(sorted[i]->in_play)) continue; // Don't collide with balls in pockets...

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
    rotation = glm::rotate(glm::dmat4(1.0), (M_PI*2) * (distance /(M_PI * diameter)), glm::normalize(axis)) * rotation;
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
}

