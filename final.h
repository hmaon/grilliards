#pragma once

#include <glm/glm.hpp>

extern GLuint MVP_id;
extern GLuint program_id;
extern glm::dmat4 perspective, view;

extern int winWidth;
extern int winHeight;
extern float viewangle;
extern void winReshapeFcn(GLint, GLint);

extern int frames; // don't use this for much except special effects

extern int next_sound_channel;
#define NUM_CHANNELS 8
#define next_channel() next_sound_channel++; next_sound_channel%=NUM_CHANNELS;

extern double current_ticks;
extern double prev_ticks;
extern double animation_time, old_animation_time, time_offset;

extern bool running;

void NaNtest(glm::dmat4 &m);


void pause();
void animate(void);
void displayFPSOverlay(bool readyfortext);
void base_key_down(unsigned char key, int x, int y);
GLuint send_one_texture(char *bmp_filename);

#if 0
static double dotProduct(double A[2], double B[2])
{
	return A[0]*B[0] + A[1]*B[1];
}
#endif

static double dotProduct(double A0, double A1, double B0, double B1)
{
	return A0*B0 + A1*B1;
}

static long double quadratic_formula0(long double a, long double b, long double c)
{
    return (0.0-b + sqrt(b*b - 4*a*c))/(2*a);
    //return (-b - sqrt(b*b - 4*a*c))/(2*a)
}

static long double quadratic_formula1(long double a, long double b, long double c)
{
    //return (-b + sqrt(b*b - 4*a*c))/(2*a);
    return (0.0-b - sqrt(b*b - 4*a*c))/(2*a);
}

void look_at(double *params); // pointer to 6 doubles, the first 6 arguments to gluLookAt()
void interpolated_camera(double *start, double *end, double fraction);

extern void (*finalIdleFunc)(void);

const char *strGLError(GLenum glErr);


#define glErrorCheck() {\
    GLenum glErr;\
    if ((glErr = glGetError()) != GL_NO_ERROR)\
    {\
        \
        do \
        { \
            printf("%s:%d %s %s\n", __FILE__, __LINE__, ": glGetError() complaint: ", strGLError(glErr));\
        } while (((glErr = glGetError()) != GL_NO_ERROR));\
    }\
}

inline glm::mat4 &operator << (glm::mat4 &a, glm::dmat4 b)
{
	float *_a = &a[0][0];
	double *_b = &b[0][0];
	
	for (int i = 0; i < 16; ++i) _a[i] = _b[i];
	
	return a;
}