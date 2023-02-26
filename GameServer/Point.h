#include <stdio.h>
#include <math.h>

#define PI (3.141592)

typedef struct {
    double x;
    double y;
} Point;

// ���簢���� ȸ����Ű�� �Լ�
void RotateRectangle(Point p[], int n, double rotation, double cx, double cy);

bool IsPointInPolygon(Point polygon[], int n, Point q);