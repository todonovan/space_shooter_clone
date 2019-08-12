#pragma once

#ifndef GEOMETRY_CPP
#define GEOMETRY_CPP

#include <float.h>

#include "common.h"
#include "geometry.h"

// This function makes use of float-equals to avoid the standard float issues.
bool32_t VecEq(vec_2 A, vec_2 B)
{
    return FLT_EQ(A.X, B.X) && FLT_EQ(A.Y, B.Y);
}

vec_2 AddVectors(vec_2 A, vec_2 B)
{
    vec_2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

vec_2 ScaleVector(vec_2 V, float S)
{
    vec_2 Result;
    Result.X = V.X * S;
    Result.Y = V.Y * S;

    return Result;
}

// The difference between this func and AddVectors is this func
// mutates the first argument passed in.
void TranslateVector(vec_2 *Point, vec_2 TransVec)
{
    Point->X = Point->X + TransVec.X;
    Point->Y = Point->Y + TransVec.Y;
}

void RotatePolygon(polygon *Poly, float Theta)
{
    for (uint32_t i = 0; i < Poly->N; ++i)
    {
        float X_Orig = Poly->Vertices[i].X;
        float Y_Orig = Poly->Vertices[i].Y;
        Poly->Vertices[i].X = (X_Orig * cosf(Theta)) - (Y_Orig * sinf(Theta));
        Poly->Vertices[i].Y = (X_Orig * sinf(Theta)) + (Y_Orig * cosf(Theta));
    }
}

vec_2 GetDir(vec_2 A, vec_2 B)
{
    vec_2 Dir;
    Dir.X = (B.X - A.X);
    Dir.Y = (B.Y - A.Y);

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

vec_2 GetDirUnit(vec_2 A, vec_2 B)
{
    return Normalize(GetDir(A, B));
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

void FurthestPointsAlongDirection(vec_2 Dir, vec_2 *Verts, uint32_t N, vec_2 *max)
{
    float maxproj = -FLT_MAX;
    for (uint32_t i = 0; i < N; i++)
    {
        float proj = Dot(Verts[i], Dir);

        if (proj > maxproj)
        {
            maxproj = proj;
            max = &Verts[i];
        }
    }
}

// Have to construct *each frame* and off of the 
// *already rotated* vertices. ORDER IS IMPORTANT.
// Obviously, can't construct off of initial verts & then rotate
// because then it's not AA.
AABB ConstructAABB(polygon *P)
{
    AABB result = {};

    vec_2 posX = {1, 0};
    vec_2 posY = {0, 1};
    vec_2 negX = {-1, 0};
    vec_2 negY = {0, -1};

    vec_2 minX, minY, maxX, maxY;

    FurthestPointsAlongDirection(posX, P->Vertices, P->N, &maxX);
    FurthestPointsAlongDirection(posY, P->Vertices, P->N, &maxY);
    FurthestPointsAlongDirection(negX, P->Vertices, P->N, &minX);
    FurthestPointsAlongDirection(negY, P->Vertices, P->N, &minY);

    result.Min.X = minX.X;
    result.Min.Y = minY.Y;
    result.Lengths.X = maxX.X - minX.X;
    result.Lengths.Y = maxY.Y - minY.Y;

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

bool32_t Contains(float N, projection_vals Range)
{
    float A = Range.Min; float B = Range.Max;
    if (B < A)
    {
        A = B;
        B = Range.Min;
    }
    // As opposed to a simple N >= A, we use FLT_EQ to first check 'equality'
    return ((FLT_EQ(N, A) || (N > A)) && (FLT_EQ(N, B) || (N <= B)));
}

bool32_t Overlap(projection_vals A, projection_vals B)
{
    if (Contains(A.Min, B)) return true;
    if (Contains(A.Max, B)) return true;
    if (Contains(B.Min, A)) return true;
    if (Contains(B.Max, A)) return true;

    return false;
}

// I'm pretty sure this needs entirely rewritten
bool32_t SeparatingAxisTest(polygon *A, polygon *B)
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