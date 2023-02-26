#include <stdio.h>
#include <math.h>

#define PI (3.141592)

typedef struct {
    double x;
    double y;
} Point;

// 직사각형을 회전시키는 함수
void RotateRectangle(Point p[], int n, double rotation, double cx, double cy);

bool IsPointInPolygon(Point polygon[], int n, Point q);