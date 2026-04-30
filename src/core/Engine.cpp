#include "Engine.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm> //for std::clamp
#include <cmath> // for std::abs
#include <yaml-cpp/yaml.h>


Engine::Engine(){
    // first initialize the window once we run the programm
    m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(1280,720),"game Eat Meat");
    // strictly set the frame rate to 60 fps
    m_window->setFramerateLimit(60);
    // start the stop watch 
    m_deltaClock = std::make_unique<sf::Clock>();
    m_isrunning = true;

    //loading the heavy assest into ram once

    m_assets.addtexture("tex_player", "game_assets/adventurer-idle-00-1.3.png");
    m_assets.addtexture("tex_enemy", "game_assets/FR_Slime4_Attack_000.png");
    m_assets.addtexture("tex_brick", "game_assets/UI_Lifebar.png");
    m_assets.addtexture("tex_bullet", "build/game_assets/Green-bullet.png");

    load_level("rooms/level.yaml");
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

        //feature needed for designinig the map
        if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R){
            load_level("rooms/level.yaml");
        }

        if(event.type == sf::Event::Closed){
            m_isrunning = false;
            // we dont call (m_window->close();) because it is getting destroyed automaticly when the loop stops and the destructor is called and to prevent crashes

        }

        // we keep escape separated from realeased because its cleaner
        if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            m_isrunning = false;

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

        if(event.type == sf::Event::MouseButtonPressed){
            // create projectile on each left click
            if(event.mouseButton.button == sf::Mouse::Left){
                // you need to get the world position of the mouse because the relative position of the window will give wrong decisions when you move a distance by the window size
                sf::Vector2f mouse_world = m_window->mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                sSpawnBullet(m_player, mouse_world.x, mouse_world.y);

            }

        }

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
        
        constexpr float player_speed = 200.0f;
        entity_velocity.vx = 0;
        if(entity_input.right) entity_velocity.vx += player_speed;
        if(entity_input.left)  entity_velocity.vx -= player_speed;
        
        // the (0,0) coordinates in sfml are in the top left and the y axis is downward 
        // so if we want to move down we actually increase the y value 
        constexpr float jump_power = -980.0f;
        if(entity_input.up && entity_input.can_jump){
            entity_velocity.vy    = jump_power;
            entity_input.can_jump = false;
        }

        //for variable jumb height
        if(!entity_input.up && entity_velocity.vy < 0.0f){
            entity_velocity.vy *= 0.5f; // for smooth stop not sudden
        }

    }

    // add velocity to the position to move entities
    for(Entity e : m_registry.velocities.getentities()){
        if(!(m_registry.hasComponent<CTransform>(e))) continue;

        auto& entity_transform = m_registry.getComponent<CTransform>(e);
        auto& entity_velocity = m_registry.getComponent<CVelocity>(e);

        if(!m_registry.hasComponent<CLifespan>(e)){
        entity_velocity.vy += m_gravity * dt;
        
        // you dont want to keep accelerating to the level that you bypass the floor
        constexpr float max_fall_speed =  735.0f;
        entity_velocity.vy = std::min(entity_velocity.vy, max_fall_speed);
        }

        entity_transform.x += entity_velocity.vx * dt;
        entity_transform.y += entity_velocity.vy * dt;

    }

    // destroy bullets if they exoire thier ifesapn or hit an enemy
    for(Entity bullet : m_registry.lifespans.getentities()){

        auto& bul_life = m_registry.getComponent<CLifespan>(bullet);
        bul_life.life_span -= dt;
        if(bul_life.life_span <= 0.0f){
            m_registry.destroyentity(bullet);
            continue; // to stop processing the dead bullet and prevent hitting an enemy with dead bullet
        }

        for(Entity enemy : m_registry.healths.getentities()){
            if(enemy == m_player) continue;

            if(isColliding(bullet, enemy)){
                auto& enemy_health = m_registry.getComponent<CHealth>(enemy);
                auto& bullet_damage = m_registry.getComponent<CDamage>(bullet);
                enemy_health.health -= bullet_damage.damage;
                m_registry.destroyentity(bullet);

                if(enemy_health.health <= 0){
                    m_registry.destroyentity(enemy);
                }

                break; // to stop the bullet from damaging many enemies
            }
        }
    }
}

// our physics systems that give entities their rigid bodies
void Engine::sCollision(){
    const auto& entities_with_BB = m_registry.bounding_boxes.getentities();
    for(Entity e : entities_with_BB){
        // ensure that entity also has position to draw that bounding-box on
        // and ensure that bullets also dont collide
        if(!m_registry.transforms.has(e) || m_registry.hasComponent<CLifespan>(e)) continue;

        auto& entity_transform = m_registry.getComponent<CTransform>(e);
        auto& entity_bounding_box = m_registry.getComponent<CBoundingBox>(e);
        constexpr float window_y = 720.0f; 

        //here is the potimized function of c++
        const float half_width  = (entity_bounding_box.width / 2.0f);
        const float half_height = (entity_bounding_box.height/ 2.0f);

        // if needed feature
        // stop entities from getting stuck when they hit the wall (allow for bing bong like mechanics)
        // if(m_registry.hasComponent<CVelocity>(e)){
        //     auto& entity_velocity = m_registry.getComponent<CVelocity>(e);
        //     if(entity_transform.x <= half_width || entity_transform.x >= window_x - half_width){
        //         entity_velocity.vx *= -1;
        //     } 
        //     if(entity_transform.y <= half_height || entity_transform.y >= window_y - half_height){
        //         entity_velocity.vy *= -1;
        //     }

        // if needed featuer 
        // can go left but not back right
        if(entity_transform.x < half_width){
            entity_transform.x = half_width;
        }
        
        // }
        // we dont need to clamp the x axes
        // stop entities from getting off the y edges unless you want them to fall to the abyss 
        // entity_transform.y = std::clamp(entity_transform.y, half_height, window_y - half_height);
        
        // treat the last pixel in the y axes as a floor
        float floor = window_y - half_height;
        if(entity_transform.y  >= floor){
            entity_transform.y = floor;
            
            if(m_registry.hasComponent<CVelocity>(e)){
                m_registry.getComponent<CVelocity>(e).vy = 0.0f;
            }
            if(m_registry.hasComponent<CInput>(e)){
                m_registry.getComponent<CInput>(e).can_jump = true;
            }
        }
    }
    // check for collisions between an entity and other entities
    for(size_t i = 0; i < entities_with_BB.size(); i++){
        Entity entity_A = entities_with_BB[i];
        // stop bullets also from pushing entities away
        if(!m_registry.hasComponent<CTransform>(entity_A) || m_registry.hasComponent<CLifespan>(entity_A)) continue;

        auto& ent_A_Tra = m_registry.getComponent<CTransform>(entity_A);
        auto& ent_A_Box = m_registry.getComponent<CBoundingBox>(entity_A);

        for(size_t j = (i + 1); j < entities_with_BB.size(); j++){
            Entity entity_B = entities_with_BB[j];
            if(!m_registry.hasComponent<CTransform>(entity_B) || m_registry.hasComponent<CLifespan>(entity_B)) continue;

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

                bool a_can_move = m_registry.hasComponent<CVelocity>(entity_A);
                bool b_can_move = m_registry.hasComponent<CVelocity>(entity_B);

                // applying logic for making bodies rigid 
                // (shitest if else chain i have ever done)
                if(overlap_x < overlap_y){
                    if(a_can_move && b_can_move)
                    {
                        float push_value = overlap_x / 2.0f;
                        if(ent_A_Tra.x < ent_B_Tra.x)
                        {
                            ent_A_Tra.x -= push_value;
                            ent_B_Tra.x += push_value;
                        } else
                        {
                            ent_A_Tra.x += push_value;
                            ent_B_Tra.x -= push_value;                                
                        }
                    } else if(!a_can_move && b_can_move)
                    {
                        if(ent_A_Tra.x < ent_B_Tra.x)
                        {
                            ent_B_Tra.x += overlap_x;
                        } else
                        {
                            ent_B_Tra.x -= overlap_x;

                            if(m_registry.hasComponent<CInput>(entity_B))
                            {
                                m_registry.getComponent<CInput>(entity_B).can_jump = true;
                            }
                            
                        }
                    } else if(a_can_move && !b_can_move)
                    {
                        if(ent_A_Tra.x < ent_B_Tra.x)
                        {
                            ent_A_Tra.x -= overlap_x;
                        } else
                        {
                            ent_A_Tra.x += overlap_x;
                        }
                    }  
                } else
                { if(a_can_move && b_can_move)
                    {
                        float push_value = overlap_y / 2.0f;
                        if(ent_A_Tra.y < ent_B_Tra.y)
                        {
                            ent_A_Tra.y -= push_value;
                            ent_B_Tra.y += push_value;
                            if(m_registry.hasComponent<CInput>(entity_A))
                            {
                                m_registry.getComponent<CInput>(entity_A).can_jump = true;
                            }
                            m_registry.getComponent<CVelocity>(entity_A).vy = 0.0f;
                        } else 
                        {
                            ent_A_Tra.y += push_value;
                            ent_B_Tra.y -= push_value;
                            if(m_registry.hasComponent<CInput>(entity_B))
                            {
                                m_registry.getComponent<CVelocity>(entity_B).vy = 0.0f;
                            }
                        }
                    } else
                     if(!a_can_move && b_can_move) 
                    {
                        if(ent_A_Tra.y < ent_B_Tra.y)
                        {
                        ent_B_Tra.y += overlap_y;
                        } else
                        {
                            ent_B_Tra.y -= overlap_y;  
                            if(m_registry.hasComponent<CInput>(entity_B))
                            {
                                m_registry.getComponent<CInput>(entity_B).can_jump = true;
                            }
                            m_registry.getComponent<CVelocity>(entity_B).vy = 0.0f;    
                        }
                    } else if(a_can_move && !b_can_move)
                    {
                        if(ent_A_Tra.y < ent_B_Tra.y)
                        {
                            ent_A_Tra.y -= overlap_y;                        
                            if(m_registry.hasComponent<CInput>(entity_A))
                            {
                                m_registry.getComponent<CInput>(entity_A).can_jump = true ;
                            }
                            m_registry.getComponent<CVelocity>(entity_A).vy = 0.0f;
                        } else
                        {
                            ent_A_Tra.y += overlap_y; 
                        }
                    }
                }
            }

        }

    }

}

bool Engine::isColliding(Entity a, Entity b){
    if(!m_registry.hasComponent<CTransform>(a)   || !m_registry.hasComponent<CTransform>(b)  ) return false;
    if(!m_registry.hasComponent<CBoundingBox>(a) || !m_registry.hasComponent<CBoundingBox>(b)) return false;

    auto& a_t  = m_registry.getComponent<CTransform>(a) ;
    auto& b_t  = m_registry.getComponent<CTransform>(b) ;
    auto& a_bb = m_registry.getComponent<CBoundingBox>(a);
    auto& b_bb = m_registry.getComponent<CBoundingBox>(b);
    
    // real diffrence
    float diff_x = std::abs(a_t.x - b_t.x);
    float diff_y = std::abs(a_t.y - b_t.y);   
    // minimum diffrence before collision
    float dx = (a_bb.width  + b_bb.width ) / 2;
    float dy = (a_bb.height + b_bb.height) / 2;

    return (diff_x < dx) && (diff_y < dy);

    
}

void Engine::sCleanUp(){
    // clean the dead entities that didnot survive the update system logic before sending them to be rendered
    m_registry.cleardeadentities();
}

void Engine::sRender(){
    // the three phases of rendering
    // first wipe the old frame (black window) to avoid drawing the player in every place he goes to
    m_window->clear(sf::Color::Black);

    //spawning the cameraman to foloow our player
    if(m_registry.hasComponent<CTransform>(m_player)){
        auto& player_position = m_registry.getComponent<CTransform>(m_player);
    
        // get the window's current camera view
        sf::View camera = m_window->getView();

        //center the camera on the player
        //360 on y to lock the camera on the y so it doesnot bounce with jumps
        camera.setCenter(player_position.x, 360.0f);

        m_window->setView(camera);
    }

    // now iterate for all entities that have sprites not shapes
    const auto& spriteentities = m_registry.sprites.getentities();
    for(Entity e : spriteentities){
        if(!(m_registry.hasComponent<CTransform>(e))) continue;

        auto& entity_sprite      = m_registry.getComponent<CSprite>(e);
        auto& entity_transform   = m_registry.getComponent<CTransform>(e);
        auto& entity_boundingbox = m_registry.getComponent<CBoundingBox>(e);

        // fetch the texture from ram using its name
        sf::Texture& texture = m_assets.gettexture(entity_sprite.name);

        //create an sfml sprite object to render it
        sf::Sprite rendersprite;
        rendersprite.setTexture(texture);

        //get the original size of the downloaded image
        sf::FloatRect bounds = rendersprite.getLocalBounds();

        // calculate the exact scale needed to match the physical bounding box
        float scale_x = entity_boundingbox.width  / bounds.width;
        float scale_y = entity_boundingbox.height / bounds.height;
        rendersprite.setScale(scale_x, scale_y);

        // align the origin the be the center of the texture
        rendersprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);

        // move the sprite to our math coordinates
        rendersprite.setPosition(entity_transform.x, entity_transform.y);

        m_window->draw(rendersprite);

    }

    // third push the drawn frame to the window 
    m_window->display();

}

void Engine::sSpawnBullet(Entity creator, float position_mouse_x, float position_mouse_y){
    if(!m_registry.hasComponent<CTransform>(creator)) return;

    auto& creator_transform = m_registry.getComponent<CTransform>(creator);
    float diff_x = position_mouse_x - creator_transform.x;
    float diff_y = position_mouse_y - creator_transform.y;



    float lenght = std::sqrt(diff_x * diff_x + diff_y * diff_y);
    // multiply the normalized vector by the bullet velocity
    if(lenght > 0.0f){
        constexpr float bullet_speed = 500.0f;
        float vx = (diff_x / lenght) * bullet_speed;
        float vy = (diff_y / lenght) * bullet_speed;

        Entity bullet = m_registry.createentity();
        m_registry.addComponent(bullet, CTransform{creator_transform.x, creator_transform.y});
        m_registry.addComponent(bullet, CVelocity{vx,vy});
        m_registry.addComponent(bullet, CShape{5.0f});
        m_registry.addComponent(bullet, CBoundingBox{10.0f, 10.0f});
        m_registry.addComponent(bullet, CLifespan{2.0f});
        m_registry.addComponent(bullet, CDamage{20.f});
        m_registry.addComponent(bullet, CSprite{"tex_bullet"});

    }
}

void Engine::load_level(const std::string& path){
    try {
        for(Entity e : m_registry.bounding_boxes.getentities()){
            m_registry.destroyentity(e); // to avoide overlab when restarting            }
        }
        m_registry.cleardeadentities(); // to force cleanup instanteniously not a must but having the conditionof the if must not be sked with sf::ispressed 
        
        //first load the yaml file
        YAML::Node config = YAML::LoadFile(path);

        // read global level setteings
        if(config["level"]["name"]){
            std::cout<< "loading level"<< config["level"]["name"].as<std::string>() << "\n";
        }

        // load gravity
        if(config["level"]["gravity"]){
            m_gravity = config["level"]["gravity"].as<float>();
            std::cout<< "level gravity set to: "<< m_gravity << "\n";

        }

        //layout grid parsing
        if(config["level"]["layout"]){
            const YAML::Node layout = config["level"]["layout"];
            int row = 0 ;
            constexpr float grid_size = 40.0f;
            constexpr float half_grid = grid_size / 2.0f;

            // loop through each row in the layout
            for(std::size_t i = 0; i < layout.size(); i++){
                std::string line = layout[i].as<std::string>();

                for(size_t col = 0; col < line.size(); col++){
                    char entity_to_load = line[col];

                    // find the center of this entity to spawn in 
                    float center_x = (col * grid_size) + (half_grid);
                    float center_y = (row * grid_size) + (half_grid);

                    if(entity_to_load == 'B'){
                        Entity Brick = m_registry.createentity();
                        m_registry.addComponent(Brick, CTransform{center_x, center_y});
                        m_registry.addComponent(Brick, CBoundingBox{grid_size, grid_size});
                        m_registry.addComponent(Brick, CShape{40.0f});
                        m_registry.addComponent(Brick, CSprite{"tex_brick"});
                    }

                    if(entity_to_load == 'P'){
                        m_player = m_registry.createentity();
                        m_registry.addComponent(m_player, CTransform{center_x, center_y});
                        m_registry.addComponent(m_player, CVelocity{100.0f, 100.0f});
                        m_registry.addComponent(m_player, CInput{});
                        m_registry.addComponent(m_player, CHealth{100.0f});
                        m_registry.addComponent(m_player, CShape{20.f});
                        m_registry.addComponent(m_player, CBoundingBox{40.f, 40.f});
                        m_registry.addComponent(m_player, CSprite{"tex_player"});
                    }


                    if(entity_to_load == 'E'){
                        Entity Enemy = m_registry.createentity();
                        m_registry.addComponent(Enemy, CTransform{center_x, center_y});
                        m_registry.addComponent(Enemy, CShape{20.0f});
                        m_registry.addComponent(Enemy, CHealth{100.0f});
                        m_registry.addComponent(Enemy, CBoundingBox{40.f, 40.f});
                        m_registry.addComponent(Enemy, CVelocity{0.0f, 0.0f});
                        m_registry.addComponent(Enemy, CSprite{"tex_enemy"});
                    }
                }
                row++;
            }  
        }
    }
    catch(const YAML::Exception& e){
        std::cerr<< "yaml error"<< e.what()<< "\n";
    }

}