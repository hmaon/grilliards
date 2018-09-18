#include <GL/glew.h>
#include <GL/glut.h>

#include "PoolGame.h"
#include "PoolBall.h"

#include "PoolTable.h"

#include "final.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>

namespace PoolGame
{

void PoolTable::clear_balls()
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

PoolTable::PoolTable()
{
    fprintf(stderr, "PoolTable() reporting...\n");

    balls = 0;
    memset(ball, 0, sizeof(void*)*maxballs);
    texture = -1;
}



void PoolTable::render(glm::dmat4 &parent_model)
{

    glDisable(GL_TEXTURE_2D);

    glMaterialf(GL_FRONT, GL_SHININESS, 110.0); // not too shiny! hurgh. 0 is max shininess, 128 is min.

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_grey);

    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // default really
    glEnable(GL_COLOR_MATERIAL);


    glColor3ub(63, 255, 63);

#if 0
    //
    // stupid fake shadows :P
    // I don't think I have time to implement anything proper.
    // 

    memset((void*)shadowmap, 0, sizeof(GLfloat) * 138 * 138 * 2);

    int sx, sz;
    for (int i = 0; i < balls; ++i)
    {
        if (!ball[i]->inplay) continue;
        double bx = ball[i]->x + width / 2;
        double bz = ball[i]->z + length / 2;

        for (int x = 0; x < (PoolBall::diameter + 1) / 2; ++x)
        {
            for (int z = 0; z < (PoolBall::diameter + 1) / 2; ++z)
            {
                double d = x*x + z*z;

                d -= (PoolBall::diameter * PoolBall::diameter * 0.4);

                if (d > 0.0) continue;

                d /= -(PoolBall::diameter * PoolBall::diameter);

                d += 0.01;
                if (d > 1.0) d = 1.0;

                sx = bx + x; sz = bz + z;
                if (sx > 0 && sx < 138 && sz > 0 && sz < 138 * 2)
                    shadowmap[sz][sx] = d;

                sx = bx - x; sz = bz + z;
                if (sx > 0 && sx < 138 && sz > 0 && sz < 138 * 2)
                    shadowmap[sz][sx] = d;

                sx = bx - x; sz = bz - z;
                if (sx > 0 && sx < 138 && sz > 0 && sz < 138 * 2)
                    shadowmap[sz][sx] = d;

                sx = bx + x; sz = bz - z;
                if (sx > 0 && sx < 138 && sz > 0 && sz < 138 * 2)
                    shadowmap[sz][sx] = d;
            }
        }
    }
#endif

#if 1
    for (int i = 0; i < NUM_TEXTURED_BALLS; ++i)
    {
        ballPosData[i] = ball[i]->in_play ? ball[i]->fpos4() : glm::vec4(100, 0, 0, 0);
    }
    update_shadow_buffers(); // send positions to shader as UBO
    // in a more general solution, this might be a named buffer object with the implementation contained to PoolTable or the renderer

    // this is a really quick way of drawing a slab of the proper dimensions but the polygons won't get lit up well by positional lights
    //glm::dmat4 table_model = parent_model * glm::scale(glm::dmat4(1.0), glm::dvec3(width/2, 10.0, length/2)); // cube mesh is 2x2x2

    glUseProgram(idProgram);
    glUniform1i(PoolGame::idSkipSphere, -1); glErrorCheck();
    glUniform1f(PoolGame::idSpecular, 0.05f); glErrorCheck();

    auto table_model = parent_model; // model should be of the correct dimensions in the mesh file
    cube.render(texture, table_model);

#endif
    glm::dmat4 model = parent_model * glm::translate(glm::dmat4(1.0), glm::dvec3(-width / 2, 5.0, -length / 2));


    // this draws a table surface out of ~38000 triangles which is really
    // pretty stupid. The correct approach would be to use two triangles
    // and a good fragment shader. That's something for the next level
    // of the class, though, right?
#if 0
    for (int x = 0; x < width - 1; x++)
    {
        glBegin(GL_TRIANGLE_STRIP); // GL_BACON_STRIP, mmm, delicious
        glNormal3d(0, 1, 0);
        for (int z = 0; z < length; z++)
        {
            // OH YEAH, VERTEX-BASED SHADOWS LIKE IT'S 1995!
            GLfloat s = 1 - shadowmap[z][x + 1];
            glColor3f(0.25*s, 1.0*s, 0.25*s);
            glVertex3d(x + 1, 0, z);

            s = 1 - shadowmap[z][x];
            glColor3f(0.25*s, 1.0*s, 0.25*s);
            glVertex3d(x, 0, z);
        }
        glEnd();
    }
#endif


    //glDisable(GL_COLOR_MATERIAL);

    // draw "pockets"
    // except the table should really be a mesh that we load and I'm not spending a lot of time on these :/

    glColor3ub(255, 255, 0);
#if 1        
    glPushMatrix();
    glTranslated(width / 2, 10 - PoolBall::diameter * 2, 0);
    glutSolidCube(PoolBall::diameter * 2);
    glPopMatrix();

    glPushMatrix();
    glTranslated(-width / 2, 10 - PoolBall::diameter * 2, 0);
    glutSolidCube(PoolBall::diameter * 2);
    glPopMatrix();

    glPushMatrix();
    glTranslated(width / 2, 10 - PoolBall::diameter * 2, length / 2);
    glutSolidCube(PoolBall::diameter * 2);
    glPopMatrix();

    glPushMatrix();
    glTranslated(-width / 2, 10 - PoolBall::diameter * 2, length / 2);
    glutSolidCube(PoolBall::diameter * 2);
    glPopMatrix();

    glPushMatrix();
    glTranslated(width / 2, 10 - PoolBall::diameter * 2, -length / 2);
    glutSolidCube(PoolBall::diameter * 2);
    glPopMatrix();

    glPushMatrix();
    glTranslated(-width / 2, 10 - PoolBall::diameter * 2, -length / 2);
    glutSolidCube(PoolBall::diameter * 2);
    glPopMatrix();

#endif
    // 
    // draw the balls
    //

    model = parent_model * glm::translate(glm::dmat4(1.0), glm::dvec3(0, 4.5f + ball[0]->diameter, 0));

    if (balls > 0)
    {
        ballsleft = NUM_TEXTURED_BALLS;

        //glTranslated(0, 5 + ball[0]->diameter, 0);

        for (int i = 0; i < balls; ++i)
        {
            if (ball[i]->in_play) ball[i]->render(model);
            else --ballsleft;
        }

        ballsleft--; // don't count the cue ball, eh
    }
}

void PoolTable::update(void)
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

SimpleMesh PoolTable::cube;
const double PoolTable::width = 137; // 4.5 feet = 137.16cm
const double PoolTable::length = PoolTable::width * 2;
const double PoolTable::height = 10;
}
