#include "stdafx.h"


// ���簢���� ȸ����Ű�� �Լ�
void RotateRectangle(Point p[], int n, double rotation, double cx, double cy)
{
    double theta = (rotation + 90) * (double)(PI / 180);
    double sinTheta = sin(theta);
    double cosTheta = cos(theta);

    // ȸ����ȯ ����� ���
    double rotMatrix[2][2] = {
        {cosTheta, -sinTheta},
        {sinTheta, cosTheta}
    };

    // ���簢���� �� ���� ȸ��
    for (int i = 0; i < n; i++) {
        // ȸ�� �߽����� �̵�
        double tx = p[i].x - cx;
        double ty = p[i].y - cy;

        // ȸ����ȯ
        p[i].x = rotMatrix[0][0] * tx + rotMatrix[0][1] * ty;
        p[i].y = rotMatrix[1][0] * tx + rotMatrix[1][1] * ty;

        // ȸ�� �߽����� �̵�
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