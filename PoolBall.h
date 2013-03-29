#pragma once
#include "glm\glm.hpp"
#include "glew.h"
#include "SimpleMesh.h"


namespace PoolGame
{

// the poolball is probably the most intensive object in this game:
class PoolBall 
{
public:
	PoolBall(PoolTable *t, double initx, double initz, double initmass, int number);
	~PoolBall();

	void render(glm::dmat4 &parent_model);
	double distance_squared(PoolBall& other);

	void friction(double time_fraction);

	void move(double time_fraction);

	static SimpleMesh mesh;

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

}