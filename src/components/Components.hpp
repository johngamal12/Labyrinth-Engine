#pragma once
#include <string>

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

//input component
struct CInput{
    bool up    = false;
    bool down  = false;
    bool right = false;
    bool left  = false;
    bool can_jump = false;
};

//physical box used for detection detection
struct CBoundingBox{
    float width  = 0.0f;
    float height = 0.0f;
};

//seconds until bullets disappear
struct CLifespan{
    float life_span = 0.0f;
};

//damage component
struct CDamage{
    float damage = 0.0f;
};

//hp until death component
struct CHealth{
    float health = 100.0f;
};

//sprites finally
//we keep it as simple as possible not sfml included to keep the POD
struct CSprite{
    std::string name = "";

    int tex_x = 0;
    int tex_y = 0;
    int tex_w = 0;
    int tex_h = 0;
    //default constructor are fine 
    CSprite() = default;
    // coonstructor for full image
    CSprite(const std::string& spritename) : name(spritename){}
    //constructor for sprite sheets
    CSprite(const std::string& spritename, int x, int y, int w, int h){
        name = spritename;
        tex_x = x;
        tex_y = y;
        tex_w = w;
        tex_h = h;
    }
};