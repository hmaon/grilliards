#pragma once

namespace PoolGame
{

// some stuff the implementation of which is not important
class PoolTable;
class PoolBall;

#define NUM_TEXTURES 16


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


typedef enum poolgamemode_enum
{
	amazeballs,
	grilliards
} PoolGameMode;


#define STARTGAME 0
#define CONFUSED  1
#define GAMEMODE  2
#define USEMASS  3
#define SOMEOTHERTHING 4
#define EXEUNT 5
#define MAXMENUOPTION 6


// methods and such:

void load_assets(); 

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
