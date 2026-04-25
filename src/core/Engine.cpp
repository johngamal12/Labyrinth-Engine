#include "Engine.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

#define player_speed 100.0f

Engine::Engine(){
    // first initialize the window once we run the programm
    m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(1280,720),"game Eat Meat");
    // strictly set the frame rate to 60 fps
    m_window->setFramerateLimit(60);
    // start the stop watch 
    m_deltaClock = std::make_unique<sf::Clock>();
    m_isrunning = true;

    // create the player entity
    Entity player = m_registry.createentity();
    // give it some components
    m_registry.addComponent(player, CTransform{640.0f, 360.0f});
    m_registry.addComponent(player, CVelocity{0.0f, 0.0f});
    m_registry.addComponent(player, CShape{20.0f});
    m_registry.addComponent(player, CInput{});


    std::cout<<"Engine started succefully";

}

// destructor is needed because we used unique pointer with forward declaration requires a predifined destructor in the .cpp where the full type is known
Engine::~Engine() = default;

// the core heartbeat of the game once this called the game keeps running until until you quite
void Engine::run(){
    std::cout<<"started the game loop";

    // spins at 60 fps
    while(m_isrunning){
        //  stop the stop watch and caculate how many fractions of a second have passed since the last frame and restart the stop eatch instanly
        // for example movemnt will be multiplied by this amount by that time that has passed so it runs the same no matter the fps is
        float dt = m_deltaClock->restart().asSeconds();

        // the pipeline execution
        //phase 1 fetch input
        sUserInput();
        // phase 2 process that input using the logic that is implemented
        sUpdate(dt);
        // phase 3 cleanup dead entities before showing in the window
        sCleanUp();
        // phase4 render your current frame to draw it on the window
        sRender();

    }
    // explaicitly destry the window immediatly if the loop ends
    if(m_window->isOpen()) m_window->close();
    std::cout<<"game loop terminated";

}

void Engine::sUserInput(){
    sf::Event event;
    // the logic for now is to close the window if the red cross clicked or the esc butoon pressed
    while(m_window->pollEvent(event)){
        if(event.type == sf::Event::Closed){
            m_isrunning = false;
            // we dont call (m_window->close();) because it is getting destroyed automaticly when the loop stops and the destructor is called and to prevent crashes

        }
        if(event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased){
            bool is_pressed = (event.type == sf::Event::KeyPressed);

            for(Entity e : m_registry.inputs.getentities()){

                auto& entity_input = m_registry.getComponent<CInput>(e);
                if(event.key.code == sf::Keyboard::W) entity_input.up    = is_pressed;
                if(event.key.code == sf::Keyboard::S) entity_input.down  = is_pressed;
                if(event.key.code == sf::Keyboard::D) entity_input.right = is_pressed;
                if(event.key.code == sf::Keyboard::A) entity_input.left  = is_pressed;

            }
            
        }
        // we keep escape separated from realeased because its cleaner
        if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            m_isrunning = false;

    }

}

// we pass dt to the update function because if for some reason the machine is not good enough to produce 60 fps at least the movement stays the same
// for example if the fps is 30 and the update depends on the frames it will run in half speed but because it is time dependent it will run less smothly but the same speed
void Engine::sUpdate(float dt){
    //translate the wasd keyboard (input) to real movement
    for(Entity e : m_registry.inputs.getentities()){
        if(!(m_registry.hasComponent<CVelocity>(e))) continue;

        auto& entity_input    = m_registry.getComponent<CInput>(e);
        auto& entity_velocity = m_registry.getComponent<CVelocity>(e);
        entity_velocity.vx = 0;
        entity_velocity.vy = 0;
        // the (0,0) coordinates in sfml are in the top left and the y axis is downward 
        // so if we want to move down we actually increase the y value 
        if(entity_input.up)    entity_velocity.vy -= player_speed;
        if(entity_input.down)  entity_velocity.vy += player_speed;
        if(entity_input.right) entity_velocity.vx += player_speed;
        if(entity_input.left)  entity_velocity.vx -= player_speed;

    }
    // add velocity to the position to move entities
    for(Entity e : m_registry.velocities.getentities()){
        if(!(m_registry.hasComponent<CTransform>(e))) continue;

        auto& entity_transform = m_registry.getComponent<CTransform>(e);
        auto& entity_velocity = m_registry.getComponent<CVelocity>(e);
        entity_transform.x += entity_velocity.vx * dt;
        entity_transform.y += entity_velocity.vy * dt;

    }


    (void)dt; // remove when more systems exist
}

void Engine::sCleanUp(){
    // clean the dead entities that didnot survive the update system logic before sending them to be rendered
    m_registry.cleardeadentities();
}

void Engine::sRender(){
    // the three phases of rendering
    // first wipe the old frame (black window) to avoid drawing the player in every place he goes to
    m_window->clear(sf::Color::Black);
    // second draw all entities
    const auto& shapeentities = m_registry.shapes.getentities();
    for(Entity e : shapeentities){
        if(!(m_registry.hasComponent<CTransform>(e))) continue;

        auto& entity_shape     = m_registry.getComponent<CShape>(e);
        auto& entity_transform = m_registry.getComponent<CTransform>(e);

        // create circiul just to represent our player
        sf::CircleShape circle(entity_shape.radius);
        // center the circules anchor pinot 
        circle.setOrigin(entity_shape.radius, entity_shape.radius);
        // smove the circle to the ecs memory cooardinate
        circle.setPosition(entity_transform.x, entity_transform.y);
        // give it a color
        circle.setFillColor(sf::Color::Green);

        m_window->draw(circle);

    }

    // third push the drawn frame to the window 
    m_window->display();

}
