#include "stdafx.h"


static Point vect2d(Point p1, Point p2)
{
    Point temp;
    temp.x = (p2.x - p1.x);
    temp.y = -1 * (p2.y - p1.y);
    return temp;
}

static bool isPointInRectangle(Point A, Point B, Point C, Point D, Point m)
{
    Point AB = vect2d(A, B);
    float C1 = -1 * (AB.y * A.x + AB.x * A.y);
    float  D1 = (AB.y * m.x + AB.x * m.y) + C1;
    Point AD = vect2d(A, D);
    float C2 = -1 * (AD.y * A.x + AD.x * A.y);
    float D2 = (AD.y * m.x + AD.x * m.y) + C2;
    Point BC = vect2d(B, C);
    float C3 = -1 * (BC.y * B.x + BC.x * B.y);
    float D3 = (BC.y * m.x + BC.x * m.y) + C3;
    Point CD = vect2d(C, D);
    float C4 = -1 * (CD.y * C.x + CD.x * C.y);
    float D4 = (CD.y * m.x + CD.x * m.y) + C4;

    return     0 >= D1 && 0 >= D4 && 0 <= D2 && 0 >= D3;
}

static void rotate(Point* p, double radian)
{
    double x = p->x;
    double y = p->y;

    p->x = x * cos(radian) - y * sin(radian);
    p->y = x * sin(radian) + y * cos(radian);
}

void RotateRectangle(Point p[], const double rotation, const double cx, const double cy)
{
    double radian = (180 - rotation) * PI / 180.0;

    for (int i = 0; i < n; ++i)
    {
        Point temp;

        temp.x = p[i].x - cx;
        temp.y = p[i].y - cy;

        rotate(&temp, radian);

        p[i].x = cx + temp.x;
        p[i].y = cy + temp.y;
    }
}

bool IsPointInRectangle(Point polygon[], Point q)
{
    return isPointInRectangle(polygon[0], polygon[1], polygon[2], polygon[3], q);
}