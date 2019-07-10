#ifndef GEOMETRY_H
#define GEOMETRY_H

// To simplify memory management (long story), each polygon will hold
// enough space for 8 vertices.
#define MAX_NUM_VERTS 8

struct vec_2
{
    float X;
    float Y;
};

struct polygon
{
    uint32_t N;
    vec_2 Vertices[MAX_NUM_VERTS];
};

struct AABB
{
    vec_2 Min;
    vec_2 Max;
};

struct projection_vals
{
    float Min;
    float Max;
};

vec_2 AddVectors(vec_2 V1, vec_2 V2);

vec_2 ScaleVector(vec_2 V, float S);

vec_2 GetDir(vec_2 V1, vec_2 V2);

float Dot(vec_2 A, vec_2 B);

float Magnitude(vec_2 A);

vec_2 Normalize(vec_2 A);

vec_2 Perpendicularize(vec_2 A);

float CalculateVectorDistance(vec_2 P1, vec_2 P2);

projection_vals Project(polygon *Poly, vec_2 Axis);

bool Contains(float N, projection_vals Range);

bool Overlap(projection_vals A, projection_vals B);

bool SeparatingAxisTest(polygon *A, polygon *B);

#endif // geometry.h