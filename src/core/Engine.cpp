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

    m_assets.addtexture("tex_player_idle", "game_assets/player_spriteshhets/Idle.png");
    m_assets.addtexture("tex_player_jump", "game_assets/player_spriteshhets/Jump.png");
    m_assets.addtexture("tex_player_run", "game_assets/player_spriteshhets/Run.png");
    m_assets.addtexture("tex_player_fall", "game_assets/player_spriteshhets/Fall.png");
    m_assets.addtexture("tex_player_attack", "game_assets/player_spriteshhets/Attack1.png");
    m_assets.addtexture("tex_enemy_idle", "game_assets/Skeleton-Idle.png");
    m_assets.addtexture("tex_brick", "game_assets/UI_Lifebar.png");
    m_assets.addtexture("tex_bullet", "game_assets/Green-Effect-and-Bullet-16x16.png");
    m_assets.addtexture("tex_background", "game_assets/pixellab-2D-RPG-environment-background--1775311982174.png");

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
                if(event.key.code == sf::Keyboard::W) entity_input.up          = is_pressed;
                if(event.key.code == sf::Keyboard::S) entity_input.down        = is_pressed;
                if(event.key.code == sf::Keyboard::D) entity_input.right       = is_pressed;
                if(event.key.code == sf::Keyboard::A) entity_input.left        = is_pressed;
                if(event.key.code == sf::Keyboard::Space)  entity_input.attack = is_pressed; 
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
        auto& entity_transform = m_registry.getComponent<CTransform>(e);
        
        constexpr float player_speed = 200.0f;
        entity_velocity.vx = 0;
        if(entity_input.right){
            entity_velocity.vx += player_speed;
            entity_transform.facing_left = false;  // remember we are lookinh right
        }
        if(entity_input.left){
            entity_velocity.vx -= player_speed;
            entity_transform.facing_left = true;  // remember we are looking left     
        }
        
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

    // mele combat
    if(m_registry.hasComponent<CState>(m_player)){
        auto& player_state = m_registry.getComponent<CState>(m_player);
        auto& player_transform = m_registry.getComponent<CTransform>(m_player);
        auto& player_velocity = m_registry.getComponent<CVelocity>(m_player);
        
        if(!player_state.has_hit && player_state.current_state == "attack"){

            float reach = 50.0f; // how far the imaginary sword gets
            float sword_width = 100.0f;
            float sword_height = 80.0f;


         
            float sword_x = player_transform.facing_left ? (player_transform.x - reach) : (player_transform.x + reach);
            float sword_y = player_transform.y;


            for(Entity enemy : m_registry.healths.getentities()){
                if(enemy == m_player) continue;

                auto& enemy_BB = m_registry.getComponent<CBoundingBox>(enemy);
                auto& enemy_transform = m_registry.getComponent<CTransform>(enemy);

                // check is the imaginary box hit the enemy
                float diff_x = std::abs(sword_x - enemy_transform.x);
                float diff_y = std::abs(sword_y - enemy_transform.y);
                float min_x = (sword_width + enemy_BB.width) / 2.0f;
                float min_y = (sword_height + enemy_BB.height) / 2.0f;

                if(diff_x < min_x && diff_y < min_y){
                    auto& enemy_health = m_registry.getComponent<CHealth>(enemy);
                    enemy_health.health -= m_registry.getComponent<CDamage>(m_player).damage;
                    player_state.has_hit = true;

                    if(enemy_health.health <= 0.0f){
                        m_registry.destroyentity(enemy);
                    }
                    break; // save you time you already hiy one (unless you want to hit more in one splash)
                }

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
    m_window->clear(sf::Color(100, 149, 137)); // sky blue background

    
    // draw the background fixed to the screen 
    // reset the view to the default so the background doesnot move with the player
    sf::View default_view = m_window->getDefaultView();
    m_window->setView(default_view);
    
    
    sf::Sprite background_sprite;
    background_sprite.setTexture(m_assets.gettexture("tex_background"));
    
    // scale the background to fill the window size
    float background_size_x = 1280.0f / background_sprite.getLocalBounds().width;
    float background_size_y = 720.0f / background_sprite.getLocalBounds().height;
    background_sprite.setScale(background_size_x, background_size_y);
    
    m_window->draw(background_sprite);
    
    //spawning the cameraman to follow our player
    // the camera in world style
    // now we apply the camera so the rest of entities are drawn to the game world
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
        sf::FloatRect texture_bounds = rendersprite.getLocalBounds();

        // if width or height are greater than zero that means that we want to crop
        if(entity_sprite.tex_w > 0 || entity_sprite.tex_h > 0){
            // then we only look at the wanted rectangle
            rendersprite.setTextureRect(sf::IntRect(entity_sprite.tex_x, entity_sprite.tex_y, entity_sprite.tex_w, entity_sprite.tex_h));
            //update the width and the height to our cropped rect
            texture_bounds.width = entity_sprite.tex_w;
            texture_bounds.height = entity_sprite.tex_h;
        }

        // calculate the exact scale needed to match the physical bounding box
        float scale_x = entity_boundingbox.width  / texture_bounds.width;
        float scale_y = entity_boundingbox.height / texture_bounds.height;

        if(entity_transform.facing_left){
            scale_x *= -1;
        }

        rendersprite.setScale(scale_x, scale_y);

        // align the origin the be the center of the texture
        rendersprite.setOrigin(texture_bounds.width / 2.0f, texture_bounds.height / 2.0f);

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
        m_registry.addComponent(bullet, CSprite{"tex_bullet", 117, 53, 7, 5});

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
                        m_registry.addComponent(m_player, CBoundingBox{80.f, 80.f});
                        m_registry.addComponent(m_player, CSprite{"tex_player_idle", 66, 57, 38, 43});
                        m_registry.addComponent(m_player, CAnimation{10, .2f, 66, 57, 162});
                        m_registry.addComponent(m_player, CState{"idle", false});
                        m_registry.addComponent(m_player, CDamage{50.0f});
                    }

                    if(entity_to_load == 'E'){
                        Entity Enemy = m_registry.createentity();
                        m_registry.addComponent(Enemy, CTransform{center_x, center_y});
                        m_registry.addComponent(Enemy, CShape{20.0f});
                        m_registry.addComponent(Enemy, CHealth{100.0f});
                        m_registry.addComponent(Enemy, CBoundingBox{40.f, 40.f});
                        m_registry.addComponent(Enemy, CVelocity{0.0f, 0.0f});
                        m_registry.addComponent(Enemy, CSprite{"tex_enemy_idle", 0, 0, 24, 32});
                        m_registry.addComponent(Enemy, CAnimation{11, 0.2f, 0, 0, 24});
                        m_registry.addComponent(Enemy, CState{"idle"});
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

void Engine::sAnimation(float dt){
      // FSM animations
    for(Entity e : m_registry.states.getentities()){
        if(!m_registry.hasComponent<CAnimation>(e) || !m_registry.hasComponent<CSprite>(e) || !m_registry.hasComponent<CVelocity>(e)) continue;
        if(e != m_player) continue;
        auto& entity_state = m_registry.getComponent<CState>(e);
        auto& entity_animation = m_registry.getComponent<CAnimation>(e);
        auto& entity_sprit = m_registry.getComponent<CSprite>(e);
        auto& entity_velocity = m_registry.getComponent<CVelocity>(e); 

        // locking the states for attacking for example to prevent spamming
        if(entity_state.is_locked){
            if(entity_animation.current_frame < entity_animation.frame_count -1){
                continue;
            } else {
                entity_state.is_locked = false;
            }

        }       

        std::string new_state = "idle";

        if(entity_velocity.vx > 0.0f || entity_velocity.vx < 0.0f){
            new_state = "run";
        } 
        if(entity_velocity.vy > 0.0f){
            new_state = "fall";
        } else if(entity_velocity.vy < 0.0f){
            new_state = "jump";
        }
        if(m_registry.hasComponent<CInput>(e)){
            auto& player_input = m_registry.getComponent<CInput>(e);
            if(player_input.attack){
            new_state = "attack";
            }
        }

        if(entity_state.current_state != new_state){
            entity_state.current_state = new_state;
            entity_state.has_hit = false;
            if(new_state == "idle"){
                entity_sprit = CSprite{"tex_player_idle", 66, 57, 38, 43};
                entity_animation = CAnimation{10, .2f, 66, 57, 162};
            }
            if(new_state == "run"){
                entity_sprit = CSprite{"tex_player_run", 52, 58, 51, 43};
                entity_animation = CAnimation{8, .2f, 52, 58, 162};
            }
            if(new_state == "fall"){
                entity_sprit = CSprite{"tex_player_fall", 58, 35, 45, 66};
                entity_animation = CAnimation{3, .2f, 58, 35, 162};
            }
            if(new_state == "jump"){
                entity_sprit = CSprite{"tex_player_jump", 63, 59, 41, 44};
                entity_animation = CAnimation{3, .2f, 63, 59, 162};
            }
            if(new_state == "attack"){
                entity_sprit = CSprite{"tex_player_attack", 55, 45, 75, 55};
                entity_animation = CAnimation{7, .2f, 55, 45, 162};
                entity_state.is_locked = true; // to stop player from spamming the attack if the animation didnot finish

            }
            // updateEntitySpriteAndAnimation(e, new_state); // Helpful helper function to clean up code (to be added later if i survived this)
            entity_animation.current_frame = 0;
            entity_animation.timer = 0.0f;

        }

    }

    for(Entity e : m_registry.animations.getentities()){
        if(!m_registry.hasComponent<CSprite>(e)) continue;

        auto& entity_sprite    = m_registry.getComponent<CSprite>(e);
        auto& sprite_animation = m_registry.getComponent<CAnimation>(e);

        sprite_animation.timer += dt;

        // keep looping the frames to make the animation
        if(sprite_animation.timer >= sprite_animation.frame_speed){
            sprite_animation.timer -= sprite_animation.frame_speed;
            sprite_animation.current_frame++;

            // if (end of animation) -> start from begginig
            if(sprite_animation.current_frame >= sprite_animation.frame_count){
                sprite_animation.current_frame = 0;
            }
        }
        // move chosen sprite coordintes to the desired needed one
        entity_sprite.tex_x = sprite_animation.start_pixel_x + (sprite_animation.current_frame * sprite_animation.offset_x);
    }
}