#pragma once
#include "Registry.hpp"
#include "AssetManager.hpp"
#include <memory>


// forward declaration for sfml classes so we dont need to include that library in the header
namespace sf {
    class RenderWindow; 
    class Clock;
}

class Engine{
    private:
        // smart pointer to our sfml window unique pointer means that the engine is the owner of the window so when the engine dies the window automatically destroys (no memory leakes)
        std::unique_ptr<sf::RenderWindow> m_window;
        // smart pointer to a stopwatch that measures exactly how long a frame takes
        std::unique_ptr<sf::Clock> m_deltaClock;

        // the engine owns the ecs registry 
        Registry m_registry;
        // the condition that keeps our loop spining
        bool m_isrunning = true;
        // crate the player unique entity
        Entity m_player;
        // making the global gravity constant to be read from yaml
        float m_gravity = 0.0f;
        // the engine owns the asset library
        AssetManager m_assets;
        // navigation grid for A*
        std::vector<std::vector<int>> m_navGrid; // 0-> empty , 1-> wall
        // tracting the state of the game
        GameState m_currentState = GameState::MainMenu;


        // the phases of our loop
        void sUserInput();
        void sUpdate(float dt);
        void sCollision();
        void sCleanUp();
        void sRender();
        //others
        void sSpawnBullet(Entity creator, float position_mouse_x, float position_mouse_y);
        bool isColliding(Entity a, Entity b);
        void load_level(const std::string& path);
        void sAnimation(float dt);
        void sAI(float dt);
        std::vector<CGridPos> calculatePath(CGridPos start, CGridPos target);


    public:
        
        Engine();
        ~Engine();

        // the run function has the while loop amd the systems of the loop
        void run();

        // getter method that allows our game to ask the engin to borrow the memory manager to spawn an entity
        Registry& registry(){
            return m_registry;
        }

};