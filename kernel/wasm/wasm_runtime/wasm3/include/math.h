#ifndef _MATH_H_
#define _MATH_H_

#define NAN (0.0 / 0.0)
#define INFINITY (1.0 / 0.0)

double sqrt(double x);
float sqrtf(float x);
double pow(double x, double y);
double floor(double x);
float floorf(float x);
double ceil(double x);
float ceilf(float x);
double trunc(double x);
float truncf(float x);
double rint(double x);
float rintf(float x);
double fabs(double x);
float fabsf(float x);
double sin(double x);
double cos(double x);
double exp(double x);
double log(double x);
double copysign(double x, double y);
float copysignf(float x, float y);
/* Add others as needed by linker errors */

#endif
