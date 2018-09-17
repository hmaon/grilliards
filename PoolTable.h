#pragma once
#include <GL/glew.h>

#include "PoolGame.h"
#include "PoolBall.h"
#include <glm/glm.hpp>

namespace PoolGame
{

//
// PoolTable!
//
class PoolTable
{
public:
    static SimpleMesh cube;

    int ballsleft;
    int balls;

    GLint texture;

    static const int maxballs = 20;

    PoolBall *ball[maxballs];

    void clear_balls();



    static const double width; // these two are important
    static const double length;
    static const double height; // but this is for visuals, no gameplay effect

    GLfloat shadowmap[138*2][138]; // bleh, just... these are obviously width and length but we need int constants. It just has to fit the table dimensions.
    // TODO, replace with real shadows :/

    PoolTable();
    void render(glm::dmat4 &parent_model);

    void update(void);


    friend class PoolBall;
    //friend class PoolGame;
};

}
