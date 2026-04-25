#pragma once

// pure data component the POD plain old data (no logic)
// position
struct CTransform{
    float x = 0.0f;
    float y = 0.0f;
};

//speed
struct CVelocity{
    float vx = 0.0f;
    float vy = 0.0f;
};

//visual representation
struct CShape{
    float radius = 20.0f;
};