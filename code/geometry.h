#pragma once

#include "common.h"

// To simplify memory management (long story), each polygon will hold
// enough space for 8 vertices.
#define MAX_NUM_VERTS 8

struct vec_2
{
    float X;
    float Y;
};

struct AABB
{
    vec_2 Min;
    vec_2 Lengths;
};

struct polygon
{
    uint32_t N;
    vec_2 C;
    AABB BoundingBox;
    vec_2 Vertices[MAX_NUM_VERTS];
    vec_2 BaseVertices[MAX_NUM_VERTS]; // non-rotated/scaled/skewed/so on
};

struct projection_vals
{
    float Min;
    float Max;
};

vec_2 AddVectors(vec_2 A, vec_2 B);

vec_2 ScaleVector(vec_2 A, float S);

void TranslateVector(vec_2 *ToTranslate, vec_2 TranslateBy);

vec_2 GetDir(vec_2 A, vec_2 B);

vec_2 GetDirUnit(vec_2 A, vec_2 B);

float Dot(vec_2 A, vec_2 B);

float Magnitude(vec_2 A);

vec_2 Normalize(vec_2 A);

vec_2 Perpendicularize(vec_2 A);

float CalculateVectorDistance(vec_2 P1, vec_2 P2);

void RotatePolygon(polygon *Poly, float Angle);

projection_vals Project(polygon *Poly, vec_2 Axis);

bool32_t Contains(float N, projection_vals Range);

bool32_t Overlap(projection_vals A, projection_vals B);

bool32_t SeparatingAxisTest(polygon *A, polygon *B);

AABB ConstructAABB(polygon *P);