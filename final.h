#pragma once

int isExtensionSupported(const char *extension);

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

void pause();
void animate(void);
void displayFPSOverlay(bool readyfortext);
void base_key_down(unsigned char key, int x, int y);
GLuint send_one_texture(char *bmp_filename);

void normalize3d(double*, double*, double*);

static double dotProduct(double A[2], double B[2])
{
	return A[0]*B[0] + A[1]*B[1];
}

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
