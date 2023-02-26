#include "stdafx.h"


// 직사각형을 회전시키는 함수
void RotateRectangle(Point p[], int n, double rotation, double cx, double cy)
{
    double theta = (rotation + 90) * (double)(PI / 180);
    double sinTheta = sin(theta);
    double cosTheta = cos(theta);

    // 회전변환 행렬을 계산
    double rotMatrix[2][2] = {
        {cosTheta, -sinTheta},
        {sinTheta, cosTheta}
    };

    // 직사각형의 각 점을 회전
    for (int i = 0; i < n; i++) {
        // 회전 중심으로 이동
        double tx = p[i].x - cx;
        double ty = p[i].y - cy;

        // 회전변환
        p[i].x = rotMatrix[0][0] * tx + rotMatrix[0][1] * ty;
        p[i].y = rotMatrix[1][0] * tx + rotMatrix[1][1] * ty;

        // 회전 중심으로 이동
        p[i].x += cx;
        p[i].y += cy;
    }
}

bool IsPointInPolygon(Point polygon[], int n, Point q)
{
    int i = 0;
    int j = 0;
    bool bIsContains = false;

    for (i = 0, j = n - 1; i < n; j = i++)
    {
        if ((((polygon[i].y <= q.y) && (q.y < polygon[j].y)) ||
            ((polygon[j].y <= q.y) && (q.y < polygon[i].y))) &&
            (q.x < (polygon[j].x - polygon[i].x) * (q.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
        {
            bIsContains = !bIsContains;
        }
    }
    return bIsContains;
}