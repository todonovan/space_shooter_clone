#ifndef GEOMETRY_CPP
#define GEOMETRY_CPP

#include <float.h>

#include "common.h"
#include "geometry.h"


vec_2 AddVectors(vec_2 V1, vec_2 V2)
{
    vec_2 Result;
    Result.X = V1.X + V2.X;
    Result.Y = V1.Y + V2.Y;

    return Result;
}

vec_2 ScaleVector(vec_2 V, float S)
{
    vec_2 Result;
    Result.X = V.X * S;
    Result.Y = V.Y * S;

    return Result;
}

vec_2 GetDir(vec_2 V1, vec_2 V2)
{
    vec_2 Dir;
    Dir.X = (V2.X - V1.X);
    Dir.Y = (V2.Y - V1.Y);

    return Dir;
}

float Dot(vec_2 A, vec_2 B)
{
    return (A.X * B.X) + (A.Y * B.Y); 
}

float Magnitude(vec_2 A)
{
    return sqrtf((A.X * A.X) + (A.Y * A.Y));
}

vec_2 Normalize(vec_2 A)
{
    float mag = sqrtf((A.X * A.X) + (A.Y * A.Y));
    vec_2 Normed;
    Normed.X = A.X / mag;
    Normed.Y = A.Y / mag;
    
    return Normed;
}

vec_2 Perpendicularize(vec_2 A)
{
    vec_2 Perped;
    Perped.X = -(A.Y);
    Perped.Y = A.X;

    return Perped;
}

float CalculateVectorDistance(vec_2 P1, vec_2 P2)
{
    float d = sqrtf(((P2.X - P1.X) * (P2.X - P1.X)) + ((P2.Y - P1.Y) * (P2.Y - P1.Y)));
    return d;
}

void FurthestPointsAlongDirection(vec_2 Dir, polygon *P, vec_2 *max)
{
    float maxproj = -FLT_MAX;
    for (uint32_t i = 0; i < P->N; i++)
    {
        float proj = Dot(P->Vertices[i], Dir);

        if (proj > maxproj)
        {
            maxproj = proj;
            max = &P->Vertices[i];
        }
    }
}

// Will need to be reconstructed each frame after rotation
AABB ConstructAABB(polygon *P)
{
    AABB result = {};

    vec_2 posX = {1, 0};
    vec_2 posY = {0, 1};
    vec_2 negX = {-1, 0};
    vec_2 negY = {0, -1};

    vec_2 minX, minY, maxX, maxY;

    FurthestPointsAlongDirection(posX, P, &maxX);
    FurthestPointsAlongDirection(posY, P, &maxY);
    FurthestPointsAlongDirection(negX, P, &minX);
    FurthestPointsAlongDirection(negY, P, &minY);

    result.Min.X = negX.X;
    result.Min.Y = negY.Y;
    result.Max.X = posX.X;
    result.Max.Y = posY.Y;

    return result;
}

projection_vals Project(polygon *Poly, vec_2 Axis)
{
    Axis = Normalize(Axis);
    vec_2 *PolyVerts = Poly->Vertices;
    float Min = Dot(PolyVerts[0], Axis);
    float Max = Min;

    for (uint32_t i = 0; i < Poly->N; i++)
    {
        float Proj = Dot(PolyVerts[i], Axis);
        if (Proj < Min)
        {
            Min = Proj;
        }
        if (Proj > Max)
        {
            Max = Proj;
        }
    }

    projection_vals Result;
    Result.Min = Min;
    Result.Max = Max;

    return Result;
}

bool Contains(float N, projection_vals Range)
{
    float A = Range.Min; float B = Range.Max;
    if (B < A)
    {
        A = B;
        B = Range.Min;
    }

    return (N >= A && N <= B);
}

bool Overlap(projection_vals A, projection_vals B)
{
    if (Contains(A.Min, B)) return true;
    if (Contains(A.Max, B)) return true;
    if (Contains(B.Min, A)) return true;
    if (Contains(B.Max, A)) return true;

    return false;
}

bool SeparatingAxisTest(polygon *A, polygon *B)
{
    vec_2 Axis;
    for (uint32_t i = 0; i < A->N; i++)
    {
        vec_2 VertCur, VertNext;
        VertCur = A->Vertices[i];
        if (i == A->N - 1)
        {
            VertNext = A->Vertices[0];
        }
        else
        {
            VertNext = B->Vertices[i+1];
        }
        Axis = GetDir(VertCur, VertNext);
        Axis = Perpendicularize(Axis);
        projection_vals ProjA = Project(A, Axis);
        projection_vals ProjB = Project(B, Axis);
        if (!Overlap(ProjA, ProjB)) return false;
    }

    for (uint32_t i = 0; i < B->N; i++)
    {
        vec_2 VertCur, VertNext;
        VertCur = B->Vertices[i];
        if (i == B->N - 1)
        {
            VertNext = B->Vertices[0];
        }
        else
        {
            VertNext = B->Vertices[i+1];
        }
        Axis = GetDir(VertCur, VertNext);
        Axis = Perpendicularize(Axis);
        projection_vals ProjA, ProjB;
        ProjA = Project(A, Axis);
        ProjB = Project(B, Axis);
        if (!Overlap(ProjA, ProjB)) return false;
    }

    return true;
}

#endif