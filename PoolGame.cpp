#include <glew.h>
#define NO_SDL_GLEXT 1

#include "objloader.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include <GL/freeglut.h>

#include <stdio.h>

// for windows to define M_PI and such:
#define _USE_MATH_DEFINES

#include <cmath>
#include <limits>
#include <math.h>
#include <string.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "PoolGame.h"
#include "PoolBall.h"
#include "PoolTable.h"
#include "final.h"
#include "shader.hpp"

namespace PoolGame
{

// various constants, kind of ad-hoc
const GLfloat mat_white[] = {1.0, 1.0, 1.0, 1.0};
const GLfloat light_position[] = {0.0, 100.0, 0.0, 1.0};
//GLfloat light_position[] = {0.0, 20.0, 0.0, 1.0};
const GLfloat light_position1[] = {0.0, 100.0, 0, 1.0};
const GLfloat nil_position[] = {0, 0, 0, 1};
// https://en.wikipedia.org/wiki/Sunset_%28color%29
const GLfloat sunset[] = {250/255.0f, 214/255.0f, 165/255.0f, 1.0f};
const GLfloat tungsten_100w[] = {255.0f/255, 214.0f/255, 170.0f/255, 1.0f};
//GLfloat dim_ambiance[] = {255.0f/1024, 214.0f/1024, 170.0f/1024, 1.0f};
const GLfloat dim_ambiance[] = {255.0f/255, 214.0f/255, 170.0f/255, 1.0f};
const GLfloat mat_blue[] = {0.2f, 0.5f, 1.0f, 1.0f};
const GLfloat mat_green[] = {0.2f, 1.0f, 0.2f, 1.0f};
const GLfloat mat_black[] = {0.0f, 0.0f, 0.0f, 1.0f};
const GLfloat mat_grey[] = {0.33f, 0.33f, 0.33f, 1.0f};
// GLfloat mat_shininess[] = {66.0f};

const GLfloat v_down[] = {0, -1, 0};

// PoolGame global variables
PoolTable *table = NULL;
bool chasecam = false;
float camera_angle = 0;
float cue_angle = 270;
float cue_power = 1.0f;
bool use_mass = false; // assign the relative mass of planets to balls
int hits = 0;
glm::vec3 eye;

// shader program id
GLuint idProgram = -1;

// ids of uniform shader vars that change with every mesh
GLuint idMVP = -1, idMV = -1, idN = -1, idTex = -1, idEye = -1;

PoolGameState state = intro; 
PoolGameMode mode = amazeballs; 


// graphics and sound assets, such as they are:
GLint textures[NUM_TEXTURED_BALLS] = {-1};
GLint tabletexture = -1;

namespace Sounds
{
// sounds?
Mix_Chunk *ballclack = NULL;
Mix_Chunk *beepverb = NULL;
Mix_Chunk *squirble = NULL;
Mix_Chunk *sadwhistle = NULL;
};



//#define NUM_TEXTURES 16
BallInfos bally[NUM_TEXTURED_BALLS] = {
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
    {"data/pool-table.png",  0.3,    "Felt"},
    {"data/melon.jpg",       0.2,    "Melon"},
    {"data/ultrapotato.jpg", 0.2,    "Potato"},
    {"data/eye.jpg",         0.2,    "Eye"},
    {"data/clouds.jpg",      0.5,    "LV426"},
    {"data/apple.jpg",       0.2,    "Apple"}
};
    



// here's a resource for some billiards physics:
// http://billiards.colostate.edu/threads/physics.html
// holy heck am I ever not using most of it
// O_o



// the speed of a cue ball on a breaking shot is 20-ish MPH; second source:
// http://www.sfbilliards.com/onoda_all_txt.pdf
// 20 mph = 894.08 centimeters per second
int max_speed = 894;



// some forward declarations
class PoolTable;
class PoolBall;



GLuint VAO_id=-1, vertexbuffer=-1, uvbuffer=-1, normalbuffer=-1;





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
        winReshapeFcn(winWidth, winHeight);
        glutPostRedisplay();
        break;

    case 'D':
    case 'd':
        camera_angle += 10;
        winReshapeFcn(winWidth, winHeight);
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


    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();


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
            if (ballfollowed == NUM_TEXTURED_BALLS) 
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
            eye = glm::vec3(-sin(M_PI*camera_angle/180.0)*66+x, 25, -cos(M_PI*camera_angle/180.0)*66+z);
            view = glm::lookAt(eye, 
                        glm::vec3(x, -5, z),
                        glm::vec3(0, 1, 0));
        } else
        {
            // look on
            eye = glm::vec3(0, 50, 0);
            view = glm::lookAt( eye,
                        glm::vec3(x, -5, z),
                        glm::vec3(0, 1, 0));
        }
            
    } else 
    {

        eye = glm::vec3(wide_view[0], wide_view[1], wide_view[2]);
        look_at(wide_view);
        if (state == intro || state == over || state == menu)
        {
            view = glm::rotate(view, (animation_time+time_offset)*10, glm::dvec3(0.0,1.0,0.0));
        } else
        {
            view = glm::rotate(view, (double)camera_angle, glm::dvec3(0.0, 1.0, 0.0));
        }
        if (state == intro && frames == 3)
        {
            printf("%d\n", countdown);
            if (!(countdown--)) chasecam = true;
        }
    }


    //
    // Draw scene
    //

    glm::dmat4 model = glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, -10.0, 0.0));
    //glTranslated(0, -10, 0);

    table->render(model); // our table and balls!

    if (!table->ballsleft) state = over;

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
                "Abandon game? (Y/N)");
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


void set_scene_uniform_vars()
{
    //
    // Set light positions and related scene stuff
    // 
    
    glUseProgram(idProgram);
    glUniform4fv(glGetUniformLocation(idProgram, "sceneColor"), 1, mat_black); glErrorCheck();
    glUniform1f(glGetUniformLocation(idProgram, "ambient"), 0.3f); glErrorCheck();
    glUniform1f(glGetUniformLocation(idProgram, "diffuse"), 0.7f); glErrorCheck();
    glUniform1i(glGetUniformLocation(idProgram, "shininess"), 115); glErrorCheck();
    glUniform1f(glGetUniformLocation(idProgram, "specular"), 1.0f); glErrorCheck();
    glUniform4fv(glGetUniformLocation(idProgram, "lightPosition_worldspace[0]"), 1, light_position); glErrorCheck();
    glUniform4fv(glGetUniformLocation(idProgram, "light_ambient[0]"), 1, dim_ambiance); glErrorCheck();
    glUniform4fv(glGetUniformLocation(idProgram, "light_diffuse[0]"), 1, tungsten_100w); glErrorCheck();
    glUniform4fv(glGetUniformLocation(idProgram, "light_specular[0]"), 1, tungsten_100w); glErrorCheck();
    glUniform1f(glGetUniformLocation(idProgram, "light_quadraticAttenuation[0]"), 0.1f); glErrorCheck();
}


void gogogo()
{

    load_assets();
    set_scene_uniform_vars();

    if (table == NULL) 
    {
        table = new PoolTable();
        table->texture = tabletexture;
        newgame();
    }

    glutDisplayFunc(&display1);
    glutKeyboardFunc(&keyboard1);
    glutSpecialFunc(&special1);
    finalIdleFunc = &idle1;
}


void load_assets()
{
    idProgram = LoadShaders("Fixed.vert.glsl", "TextureFragmentShader.glsl"); glErrorCheck();
    idMVP = glGetUniformLocation(idProgram, "MVP"); glErrorCheck();	
    idMV = glGetUniformLocation(idProgram, "MV"); glErrorCheck();	
    idN = glGetUniformLocation(idProgram, "N"); glErrorCheck();	
    idTex = glGetUniformLocation(idProgram, "tex_id"); glErrorCheck();
    idEye = glGetUniformLocation(idProgram, "eye_position"); glErrorCheck();


    if (!PoolBall::mesh.loaded) PoolBall::mesh.load("data/uvsphere.obj");

    if (textures[0] == -1)
    {
        for (int i = 0; i < NUM_TEXTURED_BALLS; ++i)
        {
            textures[i] = send_one_texture(bally[i].texture);
        }
        //tabletexture = send_one_texture("data/sharecg_pool_table_cloth.jpg");
        //tabletexture = send_one_texture("data/noise.jpg");
        tabletexture = send_one_texture("data/pool-table.png");
    }

    //glm::vec2 uv_scale = glm::vec2(16,8);
    glm::vec2 uv_scale = glm::vec2(1,1);
    PoolTable::cube.load("data/table.obj", &uv_scale);


    if (!Sounds::ballclack) Sounds::ballclack = Mix_LoadWAV("data/lonepoolballhit.wav"); // o< klak klak
    if (!Sounds::ballclack) fprintf(stderr, ":( %s \n", SDL_GetError());

    if (!Sounds::beepverb) Sounds::beepverb = Mix_LoadWAV("data/beepverb.wav");
    if (!Sounds::beepverb) fprintf(stderr, ":( %s \n", SDL_GetError());

    if (!Sounds::squirble) Sounds::squirble = Mix_LoadWAV("data/lowsquirble.wav");
    if (!Sounds::squirble) fprintf(stderr, ":( %s \n", SDL_GetError());

    if (!Sounds::sadwhistle) Sounds::sadwhistle = Mix_LoadWAV("data/sadwhistle.wav");
    if (!Sounds::sadwhistle) fprintf(stderr, ":( %s \n", SDL_GetError());	
}


// reset all the things?
void newgame()
{
    hits = 0; // this is how we keep score

    table->clear_balls();

    table->ball[0] = new PoolBall(table, 0, -table->length/4, 1, 0);

    for (int i = 1, row = 0, j = 0; i < NUM_TEXTURED_BALLS; ++i)
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

    table->balls = NUM_TEXTURED_BALLS;
}

}; // namespace
