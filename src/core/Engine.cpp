#include "Engine.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm> //for std::clamp
#include <cmath> // for std::abs
#include <yaml-cpp/yaml.h>
#include <vector>
#include <imgui.h>
#include <imgui-SFML.h>
#include "DatabaseManager.hpp"

Engine::Engine(){
    // first initialize the window once we run the programm
    m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(1280,720),"game Eat Meat");
    // strictly set the frame rate to 60 fps
    m_window->setFramerateLimit(60);
    // imgui hook initialize
    ImGui::SFML::Init(*m_window);
    // start the stop watch 
    m_deltaClock = std::make_unique<sf::Clock>();
    m_isrunning = true;

    //loading the heavy assest into ram once
    DatabaseManager::getInstance().connect("data/GameData (1).db");
    DatabaseManager::getInstance().initSchema();

    m_assets.addtexture("tex_player_idle", "game_assets/player_spriteshhets/Idle.png");
    m_assets.addtexture("tex_player_jump", "game_assets/player_spriteshhets/Jump.png");
    m_assets.addtexture("tex_player_run", "game_assets/player_spriteshhets/Run.png");
    m_assets.addtexture("tex_player_fall", "game_assets/player_spriteshhets/Fall.png");
    m_assets.addtexture("tex_player_attack", "game_assets/player_spriteshhets/Attack1.png");
    m_assets.addtexture("tex_enemy_idle", "game_assets/Skeleton-Idle.png");
    m_assets.addtexture("tex_enemy_chase", "game_assets/Skeleton-Walk.png");
    m_assets.addtexture("tex_enemy_attack", "game_assets/Skeleton-Attack.png");
    m_assets.addtexture("tex_enemy_dead", "game_assets/Skeleton-Dead.png");
    m_assets.addtexture("tex_grass", "game_assets/grass_1.png");
    m_assets.addtexture("tex_ground", "game_assets/ground_1.png");
    m_assets.addtexture("tex_bullet", "game_assets/Green-Effect-and-Bullet-16x16.png");
    m_assets.addtexture("tex_background", "game_assets/Image.png");
    m_assets.addtexture("tex_attack_effect", "game_assets/PunchImp1.png");
    m_assets.addtexture("flying_enemy_tex", "game_assets/cthulu_192x112_SpriteSheet.png");

    m_sounds.addsound("music_level02", "game_assets/sounds/504_Privy_Council.mp3");
    m_sounds.addsound("sfx_player_attack", "game_assets/sounds/sfx_attack_sword_001.wav");
    m_sounds.addsound("sfx_player_jump", "game_assets/sounds/Wood Chain Jump.wav");
    m_sounds.addsound("sfx_player_death", "game_assets/sounds/death_10_alex.wav");
    m_sounds.addsound("sfx_player_take_damage", "game_assets/sounds/getting_hit_alex.wav");
    m_sounds.addsound("sfx_enemy_take_damage", "game_assets/sounds/Sword Impact Hit 2.wav");
    m_sounds.addsound("sfx_enemy_death", "game_assets/sounds/large-monster-death-01.wav");
    m_sounds.addsound("sfx_player_shooting", "game_assets/sounds/Fireball 1.wav");
    m_sounds.addsound("sfx_enemy_attack", "game_assets/sounds/15_Impact_flesh_02.wav");

    load_level("rooms/level02.yaml");
    m_sounds.playmusic("music_level02");

    std::cout<<"Engine started succefully";

}

// destructor is needed because we used unique pointer with forward declaration requires a predifined destructor in the .cpp where the full type is known
Engine::~Engine() {
    ImGui::SFML::Shutdown(); // imgui hook shutdown

 }

// the core heartbeat of the game once this called the game keeps running until until you quite
void Engine::run(){
    std::cout<<"started the game loop";

    // spins at 60 fps
    while(m_isrunning){
        //  stop the stop watch and caculate how many fractions of a second have passed since the last frame and restart the stop eatch instanly
        // for example movemnt will be multiplied by this amount by that time that has passed so it runs the same no matter the fps is
        float dt = m_deltaClock->restart().asSeconds();
        // imgui hook update
        ImGui::SFML::Update(*m_window, sf::seconds(dt));

        // the pipeline execution
        //phase 1 fetch input
        sUserInput();
        // phase 2 process that input using the logic that is implemented
        sUpdate(dt);
        sAI(dt);
        sCollision();
        sAnimation(dt);
        // phase 3 cleanup dead entities before showing in the window
        sCleanUp();
        // phase4 render your current frame to draw it on the window
        sRender();

    }
    // explaicitly destry the window immediatly if the loop ends
    if(m_window->isOpen()) m_window->close();
    std::cout<<"game loop terminated";

}


void Engine::sCleanUp(){
    // clean the dead entities that didnot survive the update system logic before sending them to be rendered
    m_registry.cleardeadentities();
}
