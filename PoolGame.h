#pragma once
#include "glm/glm.hpp"
#include <SDL_mixer.h>

namespace PoolGame
{

// some stuff the implementation of which is not important
class PoolTable;
class PoolBall;

#define NUM_TEXTURED_BALLS 16

// shader program id
extern GLuint idProgram;

// MVP matrix id, needed everywhere...
extern GLuint idMVP, idMV, idN, idTex, idEye;

// constants, global for now
extern const GLfloat mat_white[];
extern const GLfloat light_position[];
extern const GLfloat light_position1[];
extern const GLfloat nil_position[];
extern const GLfloat sunset[];
extern const GLfloat tungsten_100w[];
extern const GLfloat dim_ambiance[];
extern const GLfloat mat_blue[];
extern const GLfloat mat_green[];
extern const GLfloat mat_black[];
extern const GLfloat mat_grey[];

extern int max_speed;

extern glm::vec3 eye;

extern bool use_mass; // assign the relative mass of planets to balls

typedef enum poolgamestate_enum
{
    intro,  // some kind of intro screen?
    help,   // a help screen of some sort
    menu,   // just the menu
    cue,    // positioning the stick!
    watch,  // watch the balls roll
    over,   // game over screen?
    beer,   // the point where you realize you're bad at pool and just drink
    achiev, // achievements screen, like quantity of beer consumed,
            // number of times scratched in a single game, etc.
    fight,  // cues aren't the best melee weapons but they'll do in a pinch
    confirm,// you sure you want to quit?
    nada
} PoolGameState;

extern PoolGameState state;

typedef enum poolgamemode_enum
{
    amazeballs,
    grilliards
} PoolGameMode;

extern PoolGameMode mode;

typedef struct BallInfos_struct
{
    char *texture;
    double mass;
    char *name;
} BallInfos;

extern BallInfos bally[NUM_TEXTURED_BALLS];

extern GLint textures[NUM_TEXTURED_BALLS];

// the number of the beef:
#define NUM_BEEF 7

#define STARTGAME 0
#define CONFUSED  1
#define GAMEMODE  2
#define USEMASS  3
#define SOMEOTHERTHING 4
#define EXEUNT 5
#define MAXMENUOPTION 6

namespace Sounds
{
// sounds?
extern Mix_Chunk *ballclack;
extern Mix_Chunk *beepverb;
extern Mix_Chunk *squirble;
extern Mix_Chunk *sadwhistle;
};


// methods and such:

void load_assets(); 

// these play sounds:
void squirble();
void sadwhistle();

// event handlers for freeglut:
void idle1(void);
void menu_keyboard(unsigned char key, int x, int y);
void keyboard1(unsigned char key, int x, int y);
void special1(int key, int x, int y);
void menu_special(int key, int x, int y);
void display1(void);

void gogogo(); // run this to make the class sink its hooks into glut
void newgame(); // reset table, rack balls, derp derp.
};
