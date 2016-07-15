#pragma once

// TODO

#define INFINITY	0

#define NAN	0

#define HUGE_VAL 0
#define HUGE_VALF 0
#define HUGE_VALL 0

// Float
float fabsf(float x);

float cosf(float x);
float sinf(float x);
float tanf(float x);

float acosf(float x);
float asinf(float x);
float atan2f(float y, float x);

float floorf(float x);
float ceilf(float x);
float fmodf(float x, float y);

float sqrtf(float x);
float logf(float x);
float log2f(float x);
float log10f(float x);
float expf(float x);
float frexpf(float x, int *exp);
float powf(float x, float y);

// Double
double fabs(double x);

double cos(double x);
double sin(double x);
double tan(double x);

double acos(double x);
double asin(double x);
double atan2(double y, double x);

double floor(double x);
double ceil(double x);
double fmod(double x, double y);

double sqrt(double x);
double log(double x);
double log2(double x);
double log10(double x);
double exp(double x);
double frexp(double x, int *exp);
double pow(double x, double y);

/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
