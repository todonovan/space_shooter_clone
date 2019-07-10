#include "asteroids.h"
#include "entities.h"
#include "common.h"
#include "memory.h"
#include "geometry.h"

void SpawnAsteroid(game_entity_pool *Pool, game_object_info ObjInfoStruct)
{
    game_entity *New = AllocateEntity(Pool);
    New->EntityType = ObjInfoStruct.Type;
    
    game_object *Master = &New->Master;
    Master->Type = ObjInfoStruct.Type;
    Master->Midpoint = ObjInfoStruct.Midpoint;
    Master->Momentum = ObjInfoStruct.Momentum;
    Master->AngularMomentum = ObjInfoStruct.AngularMomentum;
    Master->OffsetAngle = ObjInfoStruct.OffsetAngle;
    Master->Model
}

void DemoteAsteroid(game_entity *Asteroid)
{
    if (Asteroid->EntityType == SMALL_ASTEROID)
    {
        KillAsteroid(Asteroid);
        return;
    }
    else
    {
        SplitAsteroid(Asteroid);
    }
}

void SplitAsteroid(game_entity *Asteroid)
{
    HackyAssert(!(Asteroid->EntityType == SMALL_ASTEROID));
    HackyAssert(!(Asteroid->Pool->Blocks[Asteroid->BlockIndex].IsFree));

    /* If we cared, freeing the old asteroid first would probably be better
       as it means that asteroid would not be holding up space in the pool,
       artificially reducing by one the number of asteroids that could potentially
       be spawned.

    // Get handles to current asteroid to assist in the split
    game_entity OldAsteroid = {};
    OldAsteroid.EntityType = Asteroid->EntityType;
    OldAsteroid.Master = Asteroid->Master;
    */

    // Prepare two new asteroid objects.
    game_object_info Asteroid_A_Info, Asteroid_B_Info = {};
    object_type NewAsteroidType = (Asteroid->EntityType == LARGE_ASTEROID) ? MEDIUM_ASTEROID : SMALL_ASTEROID;
    object_type Type;
    vec_2 Midpoint;
    vec_2 Momentum;
    float OffsetAngle;
    float AngularMomentum;

    Asteroid_A_Info.Type = NewAsteroidType;
    Asteroid_A_Info.Midpoint = Asteroid->Master.Midpoint;
    Asteroid_A_Info.OffsetAngle = Asteroid->Master.OffsetAngle;
    Asteroid_A_Info.AngularMomentum = Asteroid->Master.AngularMomentum + GenerateRandomFloat(-10.0f, 10.0f);
    
    Asteroid_A_Info.Momentum = Perpendicularize(Asteroid->Master.Momentum);
    Asteroid_A_Info.Momentum.X += GenerateRandomFloat(-20.0f, 20.0f);
    Asteroid_A_Info.Momentum.Y += GenerateRandomFloat(-20.0f, 20.0f);

    SpawnAsteroid(Asteroid->Pool, Asteroid_A_Info);

    Asteroid_B_Info.Type = NewAsteroidType;
    Asteroid_B_Info.Midpoint = Asteroid->Master.Midpoint;
    Asteroid_B_Info.OffsetAngle = Asteroid->Master.OffsetAngle;
    Asteroid_B_Info.AngularMomentum = Asteroid->Master.AngularMomentum + GenerateRandomFloat(-10.0f, 10.0f);

    Asteroid_B_Info.Momentum = Perpendicularize(Perpendicularize(Perpendicularize(Asteroid->Master.Momentum)));
    Asteroid_B_Info.Momentum.X += GenerateRandomFloat(-20.0f, 20.0f);
    Asteroid_B_Info.Momentum.Y += GenerateRandomFloat(-20.0f, 20.0f);

    SpawnAsteroid(Asteroid->Pool, Asteroid_B_Info);

    // Free old asteroid; see note above about maybe rearranging the order.
    KillAsteroid(Asteroid);
}

void KillAsteroid(game_entity *Asteroid)
{
    FreeEntity(Asteroid);  
}

void InitPlayer()
{

}

void SpawnLaser()
{

}

void KillLaser()
{

}

void TickLaserTimer()
{

}