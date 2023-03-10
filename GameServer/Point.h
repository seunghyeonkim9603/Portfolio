#include <stdio.h>
#include <math.h>

#define PI (3.141592)

typedef struct {
    double x;
    double y;
} Point;


void RotateRectangle(Point p[], const double rotation, const double cx, const double cy);

bool IsPointInRectangle(Point polygon[], Point q);