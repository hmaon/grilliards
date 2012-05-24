#pragma once

#include"game_interface.h"

// some stuff the implementation of which is not important
class PoolTable;
class PoolBall;

#define NUM_TEXTURES 16

typedef enum poolgamestate_enum
{
    intro,  // some kind of intro screen?
    help,   // a help screen of some sort
    menu,   // the stupid dumb thing that's dumb
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


// the class that's invoked from elsewhere to run this game:

class PoolGame : public Game
{
public: // Blah... This is apparently not a C++ pattern
        // and this class ought to be a namespace instead. I don't know.
        // It's not a big deal for this project.

    static bool chasecam; // fixed view cam otherwise

    static float camera_angle; // rotation of the camera about Y axis
    
    static float cue_angle; // orientation of the cue stick
	static float cue_power; // (0..1]

    static PoolGameState state;

	static PoolGameMode mode;
    static bool use_mass;

    static int menu_option;

    static PoolTable *table;

	static int hits;

	// graphics and sound assets:
	static GLuint textures[NUM_TEXTURES];
	static Mix_Chunk *ballclack;
	static Mix_Chunk *beepverb;
	static Mix_Chunk *squirble;
	static Mix_Chunk *sadwhistle;

	// methods and such:
	PoolGame(); // constructor

	static void load_assets(); // can't load things when the constructor runs *shrug*

	// event handlers for freeglut:
	static void idle1(void);
    static void menu_keyboard(unsigned char key, int x, int y);
    static void keyboard1(unsigned char key, int x, int y);
    static void special1(int key, int x, int y);
    static void menu_special(int key, int x, int y);
    static void display1(void);

    void gogogo(); // run this to make the class sink its hooks into glut
	static void newgame(); // reset table, rack balls, derp derp.
};
