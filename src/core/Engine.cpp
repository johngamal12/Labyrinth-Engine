#include "Engine.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm> //for std::clamp
#include <cmath> // for std::abs

constexpr float player_speed = 200.0f;
constexpr float window_x = 1280.0f;
constexpr float window_y = 720.0f; 

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
    m_registry.addComponent(player, CBoundingBox{40.0f, 40.0f});

    // create an enimy to test the AABB on
    Entity enemy_1 = m_registry.createentity();
    m_registry.addComponent(enemy_1, CTransform{50.0f,50.0f});
    m_registry.addComponent(enemy_1, CVelocity{100.0f,250.0f});
    m_registry.addComponent(enemy_1, CShape{40.0f});
    m_registry.addComponent(enemy_1, CBoundingBox{80.0f, 80.0f});


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
        sCollision();
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

        // if the player moving in diagonal then the magnitude of speed from vx and vy are sqr(vx*2 +vy*2) which is sqr((10000 + 10000)) which is 141 not 100!
        // check if the player is moving in both the x and y directions
        if(entity_velocity.vx && entity_velocity.vy){
            // remember from trignometry unit circle that the xamd y components of 45 degrees are exactly (sqrt(2) / 2) which is a mathematical constant of 0.7071067
            // we make constexpr to calculate it only once during compilation time
            constexpr float Diagonal_45_Multiplier = 0.7071067f;

            entity_velocity.vx *= Diagonal_45_Multiplier;
            entity_velocity.vy *= Diagonal_45_Multiplier;
            // so this way when we do the math we get diagonal speed of exactly the player speed
        }

        

    }
    // add velocity to the position to move entities
    for(Entity e : m_registry.velocities.getentities()){
        if(!(m_registry.hasComponent<CTransform>(e))) continue;

        auto& entity_transform = m_registry.getComponent<CTransform>(e);
        auto& entity_velocity = m_registry.getComponent<CVelocity>(e);
        entity_transform.x += entity_velocity.vx * dt;
        entity_transform.y += entity_velocity.vy * dt;

    }

}

// our physics systems that give entities their rigid bodies
void Engine::sCollision(){
    const auto& entities_with_BB = m_registry.bounding_boxes.getentities();
    for(Entity e : entities_with_BB){
        // ensure that entity also has position to draw that bounding-box on
        if(!m_registry.transforms.has(e)) continue;

        auto& entity_transform = m_registry.getComponent<CTransform>(e);
        auto& entity_bounding_box = m_registry.getComponent<CBoundingBox>(e);


        // that code i wright myself i just discoverd i better way of doing thing so iam letting it commented because i dont want to delete my work :)
        // // remember that we set the origin of the entity to the center of it
        // if((entity_transform.x - ( entity_bounding_box.width / 2.0f)) < 0.0f ){
        //     entity_transform.x = entity_bounding_box.width / 2.0f;
        // } else if ((entity_transform.x + (entity_bounding_box.width / 2.0f)) > 1280.0f){
        //     entity_transform.x = 1280.0f - (entity_bounding_box.width / 2.0f);
        // }
        // // note that we cannot chain if else with both x and y axis because if we are in the cornor and try to move diagonal 
        // // only the x axes will get clapped because only one if will be executed and the x comes first
        // if ((entity_transform.y - (entity_bounding_box.height / 2.0f)) < 0.0f){
        //     entity_transform.y = entity_bounding_box.height / 2.0f;
        // } else if ((entity_transform.y +( entity_bounding_box.height / 2.0f)) > 720.0f){
        //     entity_transform.y = 720.0f - (entity_bounding_box.height / 2.0f);
        // }

        //here is the potimized function of c++
        const float half_width  = (entity_bounding_box.width / 2.0f);
        const float half_height = (entity_bounding_box.height/ 2.0f);

        // stop entities from getting stuck when they hit the wall (allow for bing bong like mechanics)
        if(m_registry.hasComponent<CVelocity>(e)){
            auto& entity_velocity = m_registry.getComponent<CVelocity>(e);
            if(entity_transform.x <= half_width || entity_transform.x >= window_x - half_width){
                entity_velocity.vx *= -1;
            } 
            if(entity_transform.y <= half_height || entity_transform.y >= window_y - half_height){
                entity_velocity.vy *= -1;
            }

        }

        // stop entities from getting off the edges
        entity_transform.x = std::clamp(entity_transform.x, half_width,  window_x - half_width);
        entity_transform.y = std::clamp(entity_transform.y, half_height, window_y - half_height);

    }
    // check for collisions between an entity and other entities
    for(size_t i = 0; i < entities_with_BB.size(); i++){
        Entity entity_A = entities_with_BB[i];
        if(!m_registry.hasComponent<CTransform>(entity_A)) continue;

        auto& ent_A_Tra = m_registry.getComponent<CTransform>(entity_A);
        auto& ent_A_Box = m_registry.getComponent<CBoundingBox>(entity_A);

        for(size_t j = (i + 1); j < entities_with_BB.size(); j++){
            Entity entity_B = entities_with_BB[j];
            if(!m_registry.hasComponent<CTransform>(entity_B)) continue;

            auto& ent_B_Tra = m_registry.getComponent<CTransform>(entity_B);
            auto& ent_B_Box = m_registry.getComponent<CBoundingBox>(entity_B);

            // getting the real difrence space between the two entities
            float diff_x = std::abs(ent_A_Tra.x - ent_B_Tra.x);
            float diff_y = std::abs(ent_A_Tra.y - ent_B_Tra.y);
            // getting the minimum space if the real space gets smaller than then coliision
            float dx = ((ent_A_Box.width / 2.0f) + (ent_B_Box.width / 2.0f));
            float dy = ((ent_A_Box.height / 2.0f) + (ent_B_Box.height / 2.0f));

            if((diff_x < dx) && (diff_y < dy)){
                // function to keep entities far apart
                float overlap_x = dx - diff_x;
                float overlap_y = dy - diff_y;

                // to be changed later to not let the ststic bodies move
                if(overlap_x < overlap_y){

                    float push_value = overlap_x / 2.0f;
                    if(ent_A_Tra.x < ent_B_Tra.x){
                        ent_A_Tra.x -= push_value;
                        ent_B_Tra.x += push_value;

                    } else {
                        ent_B_Tra.x -= push_value;
                        ent_A_Tra.x += push_value;
                    }
                } else{
                    float push_value = overlap_y / 2.0f;

                    if(ent_A_Tra.y < ent_B_Tra.y){
                        ent_A_Tra.y -= push_value;
                        ent_B_Tra.y += push_value;

                    } else{
                        ent_B_Tra.y -= push_value;
                        ent_A_Tra.y += push_value;
                    }

                }

                std::cout<< "collide!";
            }

        }

    }

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
        if(m_registry.hasComponent<CInput>(e)){
            circle.setFillColor(sf::Color::Green);
        } else {
            circle.setFillColor(sf::Color::Red);
        }

        m_window->draw(circle);

    }

    // third push the drawn frame to the window 
    m_window->display();

}
