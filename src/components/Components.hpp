#pragma once
#include <string>
#include <vector>

// pure data component the POD plain old data (no logic)
// position
struct CTransform{
    float x = 0.0f;
    float y = 0.0f;
    float previuos_x = 0.0f, previuos_y = 0.0f;
    bool facing_left = false;

    CTransform() = default;
    CTransform(float px, float  py) :x(px), y(py), previuos_x(px), previuos_y(py) {}
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
    bool attack =false;// attack using space
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

//animation from sprite sheets
struct CAnimation{
    int frame_count   = 1;     // how many frames make this animation
    int current_frame = 0;     // which frame we are currently at
    float frame_speed = 0.15f; // how many seconds a single frame should stay on screen
    float timer = 0.0f;        // stopwatch for the animation

    // the starting pixel coordinates of the animation on the sprite sheet
    int start_pixel_x = 0;
    int start_pixel_y = 0;

    //how many pixels from the start of the first to the start of the second frame
    int offset_x = 0;

    CAnimation() = default;
    CAnimation(int frames, float speed, int x_coordinate, int y_coordinate, int off_x)
    : frame_count(frames), frame_speed(speed),
    start_pixel_x(x_coordinate), start_pixel_y(y_coordinate), offset_x(off_x) {}

};

//finite state machine
struct CState{
    std::string current_state = "idle";
    bool has_hit = false;
    bool is_locked = false;
    bool is_grounded = false;
    float jump_cooldown = 0.0f;
};

//node for the A*
struct CPathNode{
    int x, y; //grid coordinate (rpw, column)
    float gCost; //distance from start
    float hCost; // distance to goal
    float fCost; // G + H
    CPathNode* parent = nullptr; // the node we came from
    // A* needs to find the lowest fcostso we need a way to compare nodes
    bool operator<(const CPathNode& other) const{
        return fCost > other.fCost;
    }
};

//x,y grid coordinate
struct CGridPos{
    int col, row;
    bool operator == (const CGridPos& other){
        return col == other.col && row == other.row;
    }
};



struct CAI{
    std::vector<CGridPos> waypoints;
    size_t current_waypoint = 0;
    float path_update_timer = 0.0f;
    bool is_flying = false;
    float attack_timer = 0.0f; // limites the number of attacks per second
};

//game state
enum class GameState{
    MainMenu,
    Playing,
    GameOver
};