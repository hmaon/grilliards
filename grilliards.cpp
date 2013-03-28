#include <glew.h> // must be included before all the other header garbage
#define NO_SDL_GLEXT 1

#include <vector>
#include <string>

#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/freeglut.h>
#include <stdio.h>

#include <SDL_mixer.h>
#include <SDL_image.h>

#include "PoolGame.h"

// for windows to define M_PI and such:
#define _USE_MATH_DEFINES 1

#include <cmath>
#include <math.h>
#include <string.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"

#include "final.h"

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif


// Global Variable Section

void (*finalIdleFunc)(void) = NULL;

int next_sound_channel = 0;

// the time for our animation:
double old_animation_time = 0.0; // in seconds
double animation_time = 0.0; // in seconds
double time_offset = 0.0; // time when the animation was last paused
double animation_speed = 4; 

double start_ticks = 0.0, current_ticks = 0.0; 

int frames = 0; // we totally want an fps counter, right?
double fps = 0.0;
double prev_ticks = 0.0; // time of previous frame counter reset

bool new_frame = true;

bool running = true;

#define SURF_LEN 100
double surface_height[SURF_LEN][SURF_LEN];
double surface_nx[SURF_LEN][SURF_LEN]; // we'll calculate derivatives to get normals
double surface_ny[SURF_LEN][SURF_LEN];
double surface_nz[SURF_LEN][SURF_LEN];

bool normals = false;

// Window display size
GLsizei winWidth = 1024, winHeight = 768;   



glm::dmat4 view, perspective;

void NaNtest(glm::dmat4 &m)
{
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x)
			if (m[y][x] == NAN)
				puts("NAN! :(");
}


const char *strGLError(GLenum glErr)
{
    char *err;

    switch(glErr)
    {
    case GL_INVALID_ENUM:
        err = "GL_INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        err = "GL_INVALID_VALUE";
        break;
    case GL_INVALID_OPERATION:
        err = "GL_INVALID_OPERATION";
        break;
    case GL_STACK_OVERFLOW:
        err = "GL_STACK_OVERFLOW";
        break;
    case GL_STACK_UNDERFLOW:
        err = "GL_STACK_UNDERFLOW";
        break;
    case GL_OUT_OF_MEMORY:
        err = "GL_OUT_OF_MEMORY";
        break;
    case GL_NO_ERROR:
        err = "No error! How did you even reach this code?";
        break;
    default:
        err = "Unknown error code!";
        break;
    }

    return err;
}




// Initialize method
void init (void)
{
      // Get and display your OpenGL version
	const GLubyte *Vstr;
	Vstr = glGetString (GL_VERSION);
	printf("Your OpenGL version is %s\n", Vstr);

	int n;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &n);
	printf("GL_MAX_VERTEX_ATTRIBS: %d\n", n);

	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &n);
	printf("GL_MAX_FRAGMENT_UNIFORM_COMPONENTS: %d\n", n);
	
	// black window
   	glClearColor (0.0, 0.0, 0.0, 1.0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE); 
    glCullFace(GL_BACK);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

#ifndef GL_MULTISAMPLE_ARB
#define GL_MULTISAMPLE_ARB                      0x809D
#endif
	glEnable(GL_MULTISAMPLE_ARB);	
}


void displayFPSOverlay(bool readyfortext)
{
    unsigned char fps_str[64];

	if (!readyfortext)
	{
		glDisable(GL_LIGHTING);
		//glDisable(GL_TEXTURE_2D);
		//glDisable(GL_COLOR_MATERIAL);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// draw some 2D text in Ortho projection :3
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0.0,(GLdouble) winWidth , 0.0,(GLdouble) winHeight);
	}

    glColor3ub(128, 128, 128);
    //glRasterPos2i(0,0);
    //glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)"Press spacebar to start and stop animation, L to toggle normals.");

    if (fps != 0.0)
    {
#if  defined(_WIN32) && !defined(__MINGW32__)
        sprintf_s((char*)fps_str, 63, "FPS: %g", fps);
#else
#if !defined(__MINGW32__)
		snprintf((char*)fps_str, 63, "FPS: %g", fps);
#else
        sprintf((char*)fps_str, "FPS: %g", fps);
#endif
#endif
        glRasterPos2i(0, winHeight-20);
        glutBitmapString(GLUT_BITMAP_HELVETICA_18, fps_str);
    }

    
    if (!readyfortext) glPopMatrix();

	++frames;
}


float viewangle = 45.0;

// window redraw function
void winReshapeFcn (GLint newWidth, GLint newHeight)
{
    GLfloat aspect = (float)newWidth/(float)newHeight;
    winWidth = newWidth;
    winHeight = newHeight;

#if 0
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
 	
    gluPerspective(viewangle, aspect, 5, 600.0); 
	glGetDoublev(GL_PROJECTION_MATRIX, &perspective[0][0]);
#endif


	perspective = glm::perspective(viewangle, aspect, 5.0f, 600.0f);
	printf("perspective matrix: \n");
	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			printf("%lf\t", perspective[y][x]);
		}
		printf("\n");
	}

    glViewport(0, 0, newWidth, newHeight);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
}


// animate() currently just updates time counters
// perhaps calculations should happen here too, though? or does it not matter?
void animate(void)
{
	current_ticks = (double)SDL_GetTicks(); // in 1/1000s increments

	//fputc('.', stderr);


    double new_animation_time = (current_ticks - start_ticks)/1000.0;

    if (new_animation_time == animation_time)
    {
		new_frame = false;
        return;
    } else 
    {
		new_frame = true;
        old_animation_time = animation_time;
        animation_time = new_animation_time;

        if (old_animation_time == 0.0) old_animation_time = animation_time;
    }

#ifdef __linux__
    //if (animation_time < 0.0) kill(getpid(), SIGSEGV);
#endif

    if (0 != prev_ticks && (current_ticks - prev_ticks) > 1000.0)
    {
        fps = ((double)frames * 1000.0) / (current_ticks-prev_ticks);
        frames = 0;
        prev_ticks = current_ticks;
    }

    glutPostRedisplay();

    if (finalIdleFunc != NULL)  finalIdleFunc();

	SDL_Delay(1); // let the OS work if it needs to
}


void set_start_ticks()
{
	start_ticks = prev_ticks = (double)SDL_GetTicks();
	old_animation_time = animation_time = 0.0;
    frames = 0;
}


// important
void pause()
{
    running = false;
    glutIdleFunc(NULL);
    time_offset += animation_time;
    animation_time = 0;

}

void base_key_down(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 27:
            glutLeaveMainLoop(); // freeglut only!
        break;

        case ' ':
            if (running)
            {
                glutIdleFunc(NULL);
                time_offset += animation_time;
                animation_time = 0;
            } else
            {
                set_start_ticks();
                glutIdleFunc(animate);
            }
            running = !running;
            break;

        case 'l':
        case 'L':
            normals = !normals;
            break;

        default:
            //puts("\007"); // James Bond
			// that chime is so annoying on windows. I'm really sorry.
            fprintf(stderr, "scancode: %d\n", key);
        break;
    }
    // derp.
}


GLuint send_one_texture(char *image_filename)
{
	SDL_Surface *image, *proper;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff000000
#define gmask 0x00ff0000
#define bmask 0x0000ff00
#define amask 0x000000ff
#else
#define amask 0xff000000
#define bmask 0x00ff0000
#define gmask 0x0000ff00
#define rmask 0x000000ff
#endif


    SDL_PixelFormat fmt = {NULL, 32, 4, 0, 0, 0, 0, 24, 16, 8, 0, rmask, gmask, bmask, 0, 0, 0xff}; // an RGBA 8bpp format for our sanity
	GLuint tex;

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &tex);

    if (glGetError())
	{
		fprintf(stderr, "glGenTextures() error for %s.\n", image_filename);
		return 0;
	}

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

#if 1
	// an SGI extension from OpenGL 1.4 that seems to be missing
	// http://www.opengl.org/registry/specs/SGIS/generate_mipmap.txt
	if (GLEW_SGIS_generate_mipmap)
	{		
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP                0x8191
#endif
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else puts("no GL_SGIS_generate_mipmap");
#endif

    printf("loading %s\n", image_filename);
	image = IMG_Load(image_filename);
	if (image == NULL) { puts("load error!"); return 0; }
    proper = SDL_ConvertSurface(image, &fmt, SDL_SWSURFACE);
    if (proper == NULL)
    {
        SDL_FreeSurface(image);
        return 0;
    }
        

    // for lousy quality, compress textures: 
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, proper->w, proper->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, proper->pixels);

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif


    // to waste graphics memory, leave them as they are:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, proper->w, proper->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, proper->pixels);
        
    if (glGetError() != GL_NO_ERROR) fprintf(stderr, "Error! %d", glGetError());

#if 0
	// some OpenGL 3.x stuff that's not even in my damn header files
#ifdef glGenerateMipmap
	glGenerateMipmap(GL_TEXTURE_2D); glErrorCheck();
#endif
#endif

	SDL_FreeSurface(image);
	SDL_FreeSurface(proper);

	return tex;
}


// looking at things is hard sometimes, OK?
void look_at(double *at)
{
	// deprecated...
	// gluLookAt(at[0], at[1], at[2], at[3], at[4], at[5], 0, 1, 0); // up is always up, otherwise I get motion sickness; deal with it.
	view = glm::lookAt(glm::dvec3(at[0], at[1], at[2]), glm::dvec3(at[3], at[4], at[5]), glm::dvec3(0.0, 1.0, 0.0));
}


// we don't need SDL's shenanigans since we're just using it for image loading and sound:
#undef main


// Main function
int main (int argc, char** argv)
{
    glutInit (&argc, argv); // do this before constructors start to init 
                            // other GL stuff

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER);
	atexit(SDL_Quit);

	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16SYS, 2, 1024); // 1024 might be low but whatever

    int flags = IMG_INIT_JPG | IMG_INIT_PNG;
    int initted=IMG_Init(flags);
    if((initted&flags) != flags) {
        printf("IMG_Init: Failed to init required jpg and png support!\n");
        printf("IMG_Init: %s\n", IMG_GetError());
#ifdef _WIN32
        printf("Are all the DLLs in the same directory as the .EXE file?\n");
#else
        printf("Try `apt-get install libsdl-image1.2`\n");
#endif
    }

    printf("Hello and welcome!\n");
    fflush(stdout);

    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
    // Set initial Window position
    //glutInitWindowPosition (100, 100);
    // Set Window Size
    glutInitWindowSize (winWidth, winHeight);
    // Set Window Title
    glutCreateWindow ("Greg Velichansky, CMSC405 final project, a pool game of some sort.");
	
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  /* Problem: glewInit failed, something is seriously wrong. */
	  fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	  exit(1);
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	
    // Initialize
    init ( );

    // Window reshape call
    glutReshapeFunc (winReshapeFcn);
    glutKeyboardFunc (base_key_down);
	set_start_ticks();
	glutIdleFunc(animate);

	winReshapeFcn(winWidth, winHeight);

    PoolGame::gogogo(); 

    glutMainLoop ( );

	Mix_CloseAudio();
    //delete pool;
}
