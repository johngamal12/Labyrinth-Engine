#include "Engine.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm> //for std::clamp
#include <cmath> // for std::abs
#include <yaml-cpp/yaml.h>
#include <vector>
#include <imgui.h>
#include <imgui-SFML.h>


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

    m_assets.addtexture("tex_player_idle", "game_assets/player_spriteshhets/Idle.png");
    m_assets.addtexture("tex_player_jump", "game_assets/player_spriteshhets/Jump.png");
    m_assets.addtexture("tex_player_run", "game_assets/player_spriteshhets/Run.png");
    m_assets.addtexture("tex_player_fall", "game_assets/player_spriteshhets/Fall.png");
    m_assets.addtexture("tex_player_attack", "game_assets/player_spriteshhets/Attack1.png");
    m_assets.addtexture("tex_enemy_idle", "game_assets/Skeleton-Idle.png");
    m_assets.addtexture("tex_enemy_chase", "game_assets/Skeleton-Walk.png");
    m_assets.addtexture("tex_enemy_attack", "game_assets/Skeleton-Attack.png");
    m_assets.addtexture("tex_enemy_dead", "game_assets/Skeleton-Dead.png");
    m_assets.addtexture("tex_brick", "game_assets/UI_Lifebar.png");
    m_assets.addtexture("tex_bullet", "game_assets/Green-Effect-and-Bullet-16x16.png");
    m_assets.addtexture("tex_background", "game_assets/pixellab-2D-RPG-environment-background--1775311982174.png");
    m_assets.addtexture("tex_attack_effect", "game_assets/PunchImp1.png");
    load_level("rooms/level01.yaml");
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

void Engine::sUserInput(){
    sf::Event event;
    // the logic for now is to close the window if the red cross clicked or the esc butoon pressed
    while(m_window->pollEvent(event)){
        // imgui hook prcess events
        ImGui::SFML::ProcessEvent(*m_window, event);
        // prevent player from shooting or moving if the mouse or keyboardis interacting with the imgui window
        if(ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) continue;

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
    switch(m_currentState){
        case GameState::Playing:
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
                if(m_registry.hasComponent<CState>(e)){
                    m_registry.getComponent<CState>(e).is_grounded = false;
                }
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
            if(!m_registry.hasComponent<CDamage>(bullet)) continue; 
            if(m_registry.hasComponent<CHealth>(bullet)) continue; // thats a dead enemy 

            for(Entity enemy : m_registry.healths.getentities()){
                if(enemy == m_player) continue;

                if(isColliding(bullet, enemy)){
                    auto& enemy_health = m_registry.getComponent<CHealth>(enemy);
                    auto& bullet_damage = m_registry.getComponent<CDamage>(bullet);
                    enemy_health.health -= bullet_damage.damage;
                    m_registry.destroyentity(bullet);

                    if(enemy_health.health <= 0){
                        if(!m_registry.hasComponent<CLifespan>(enemy)){
                            // give it 1.5 seconds to play dead and then it destroys automatically because it now has a life span
                            m_registry.addComponent(enemy, CLifespan{1.5f});
                            m_registry.getComponent<CState>(enemy).current_state = "dead";
                            m_registry.getComponent<CState>(enemy).is_locked = false;
                        }
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
                            if(!m_registry.hasComponent<CLifespan>(enemy)){
                                m_registry.addComponent(enemy, CLifespan{1.5f});
                                m_registry.getComponent<CState>(enemy).current_state = "dead";
                                m_registry.getComponent<CState>(enemy).is_locked = false;

                            }
                        }
                        break; // save you time you already hiy one (unless you want to hit more in one splash)
                    }

                }
            }
            
        }
        if(m_registry.hasComponent<CHealth>(m_player)){
            if(m_registry.getComponent<CHealth>(m_player).health <= 0.0f){
                m_currentState = GameState::GameOver;
            }
        }
        break;

        case GameState::MainMenu:
        //game logic is paused in the menu 
        break;

        case GameState::GameOver:
        //the player is dead we just wait to click on restart
        break;

    }
}

// our physics systems that give entities their rigid bodies
void Engine::sCollision(){
    switch (m_currentState){
        case GameState::Playing:{

        for(Entity e : m_registry.states.getentities()){
            m_registry.getComponent<CState>(e).is_grounded = false;
        }


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
                if(m_registry.hasComponent<CState>(e)){
                    m_registry.getComponent<CState>(e).is_grounded = true;
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
                                if(m_registry.hasComponent<CState>(entity_B))
                                {
                                    m_registry.getComponent<CState>(entity_B).is_grounded = true;
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
                                if(m_registry.hasComponent<CState>(entity_A))
                                {
                                    m_registry.getComponent<CState>(entity_B).is_grounded = true;
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
                                if(m_registry.hasComponent<CState>(entity_B))
                                {
                                    m_registry.getComponent<CState>(entity_B).is_grounded = true;
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
                                if(m_registry.hasComponent<CState>(entity_A))
                                {
                                    m_registry.getComponent<CState>(entity_A).is_grounded = true;
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
        if(m_registry.hasComponent<CHealth>(m_player)){
            if(m_registry.getComponent<CHealth>(m_player).health <= 0.0f){
                m_currentState = GameState::GameOver;
            }
        }
        break;
    }

        case GameState::MainMenu: {
        // nothing
        break;
        }

        case GameState::GameOver: {
        // nothing
        break;
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

    // only draw the world if we are playing or dead (so that you can see the evidence of how bad you are)
    if(m_currentState == GameState::GameOver || m_currentState == GameState::Playing){
        
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
    }
     if(m_currentState == GameState::MainMenu){
        // center the imgui window
        ImGui::SetNextWindowPos(ImVec2(1280.0f / 2.0f - 100.0f, 720.0f / 2.0f - 50.0f));
        ImGui::Begin("##MainMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::Text("Eat Meat");
        ImGui::Spacing();

        if(ImGui::Button("start game", ImVec2(200, 50))){
            m_currentState = GameState::Playing;
        }
        ImGui::End();
    } else if(m_currentState == GameState::GameOver){
        ImGui::SetNextWindowPos(ImVec2(1280.0f / 2.0f - 100.0f, 720.0f / 2.0f - 50.0f));
        ImGui::Begin("##Game Over", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("you died");
        ImGui::Spacing();

        if(ImGui::Button("retsart game", ImVec2(200, 50))){
            load_level("rooms/level01.yaml"); // this line what restart the game
            m_currentState = GameState::Playing;
        }
        ImGui::End();

    }

    //imgui hook render
    ImGui::SFML::Render(*m_window);
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
            m_navGrid.clear();
            int row = 0 ;
            constexpr float grid_size = 40.0f;
            constexpr float half_grid = grid_size / 2.0f;

            // loop through each row in the layout
            for(std::size_t i = 0; i < layout.size(); i++){
                std::string line = layout[i].as<std::string>();
                std::vector<int> gridRow;

                for(size_t col = 0; col < line.size(); col++){
                    char entity_to_load = line[col];

                    // find the center of this entity to spawn in 
                    float center_x = (col * grid_size) + (half_grid);
                    float center_y = (row * grid_size) + (half_grid);

                    if(entity_to_load == 'B'){
                        Entity Brick = m_registry.createentity();
                        m_registry.addComponent(Brick, CTransform{center_x, center_y});
                        m_registry.addComponent(Brick, CBoundingBox{grid_size, grid_size});
                        m_registry.addComponent(Brick, CSprite{"tex_brick"});
                        gridRow.push_back(1);
                    } else{
                        gridRow.push_back(0);
                    }

                    if(entity_to_load == 'P'){
                        m_player = m_registry.createentity();
                        m_registry.addComponent(m_player, CTransform{center_x, center_y});
                        m_registry.addComponent(m_player, CVelocity{100.0f, 100.0f});
                        m_registry.addComponent(m_player, CInput{});
                        m_registry.addComponent(m_player, CHealth{100.0f});
                        m_registry.addComponent(m_player, CBoundingBox{75.0f, 75.0f});
                        m_registry.addComponent(m_player, CSprite{"tex_player_idle", 66, 57, 38, 43});
                        m_registry.addComponent(m_player, CAnimation{10, .2f, 66, 57, 162});
                        m_registry.addComponent(m_player, CState{"idle", false});
                        m_registry.addComponent(m_player, CDamage{50.0f});
                    }

                    if(entity_to_load == 'E'){
                        Entity Enemy = m_registry.createentity();
                        m_registry.addComponent(Enemy, CTransform{center_x, center_y});
                        m_registry.addComponent(Enemy, CHealth{100.0f});
                        m_registry.addComponent(Enemy, CBoundingBox{40.f, 40.f});
                        m_registry.addComponent(Enemy, CVelocity{0.0f, 0.0f});
                        m_registry.addComponent(Enemy, CSprite{"tex_enemy_idle", 0, 0, 24, 32});
                        m_registry.addComponent(Enemy, CAnimation{11, 0.2f, 0, 0, 24});
                        m_registry.addComponent(Enemy, CState{"idle"});
                        m_registry.addComponent(Enemy, CAI{});
                        m_registry.addComponent(Enemy, CDamage{15.0f});
                    }
                    if(entity_to_load == 'F'){
                        // should have been a flying enemy but iam too lazy to get a new texture for these guyes
                        Entity Enemy = m_registry.createentity();
                        m_registry.addComponent(Enemy, CTransform{center_x, center_y});
                        m_registry.addComponent(Enemy, CHealth{100.0f});
                        m_registry.addComponent(Enemy, CBoundingBox{40.f, 40.f});
                        m_registry.addComponent(Enemy, CVelocity{0.0f, 0.0f});
                        m_registry.addComponent(Enemy, CSprite{"tex_enemy_idle", 0, 0, 24, 32});
                        m_registry.addComponent(Enemy, CAnimation{11, 0.2f, 0, 0, 24});
                        m_registry.addComponent(Enemy, CState{"idle"});
                        m_registry.addComponent(Enemy, CAI{{}, 0, 0.0f, true});        
                        m_registry.addComponent(Enemy, CDamage{15.0f});                
                    }
                }
                m_navGrid.push_back(gridRow);
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

        std::string new_state = entity_state.current_state;

        // player logic
        if(e == m_player){
            new_state = "idle"; // to reset the state if no movement
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
        }

        // enemy logic is drived intirley by sAI() so no need to adjust anything

        // apply new sprites if state has changed
        // check if sprite name matches the state to know if we need to swap
        if(entity_sprit.name.find(new_state) == std::string::npos || (new_state == "attack" && entity_animation.current_frame == 0)){

            entity_state.current_state = new_state;
            entity_state.has_hit = false;
            if(e == m_player){
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

                    float reach = 50.0f;
                    auto& player_tra = m_registry.getComponent<CTransform>(m_player);

                    float weapon_x = player_tra.facing_left ? (player_tra.x - reach) : (player_tra.x + reach);
                    float weapon_y = player_tra.y;

                    Entity attack_effect = m_registry.createentity();
                    m_registry.addComponent(attack_effect, CTransform{weapon_x, weapon_y});
                    m_registry.addComponent(attack_effect,CLifespan{1.5f});
                    m_registry.addComponent(attack_effect, CBoundingBox{70.0f, 70.0f});
                    m_registry.addComponent(attack_effect, CSprite{"tex_attack_effect", 11, 15, 48, 16});
                    m_registry.addComponent(attack_effect, CAnimation{5, 0.15f, 11, 15, 48});
                    m_registry.getComponent<CTransform>(attack_effect).facing_left = player_tra.facing_left;
                    
                }
            } else{ 
                if(new_state == "idle" || new_state == "patrol"){
                    entity_sprit = CSprite{"tex_enemy_idle", 0, 0, 24, 32};
                    entity_animation = CAnimation{11, 0.2f, 0, 0, 24};
                }
                if(new_state == "chase"){
                    entity_sprit = CSprite{"tex_enemy_chase", 0, 0, 22, 33};
                    entity_animation = CAnimation{13, 0.2f, 0, 0, 22};
                } 
                if(new_state == "attack"){
                    entity_sprit = CSprite{"tex_enemy_attack", 0, 0, 43, 37};
                    entity_animation = CAnimation{18, 0.1f, 0, 0, 43};
                    entity_state.is_locked = true;
                }
                if(new_state == "dead"){
                    entity_sprit = CSprite{"tex_enemy_dead", 0, 0, 33, 32};
                    entity_animation = CAnimation{15, 0.1f, 0, 0, 33};
                }

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

// our smart NPCs
void Engine::sAI(float dt){
    switch (m_currentState){
        case GameState::Playing:{

        // if the player doesnot exist do not do anything
        if(m_player == -1 || !m_registry.hasComponent<CTransform>(m_player)) return;
        auto& player_position = m_registry.getComponent<CTransform>(m_player);

        // enigne constants for translating world float coordinates to grid int coordinates
        constexpr float grid_size = 40.0f;
        constexpr float half_grid = grid_size / 2.0f;

            for(Entity e : m_registry.ais.getentities()){
                
                auto& enemy_position = m_registry.getComponent<CTransform>(e);
                auto& enemy_velocity = m_registry.getComponent<CVelocity>(e);
                auto& enemy_state = m_registry.getComponent<CState>(e);

                // if the enemy is dead stop thinking
                if(enemy_state.current_state == "dead"){
                    enemy_velocity.vx = 0.0f;
                    if(m_registry.getComponent<CAI>(e).is_flying){
                        enemy_velocity.vy = 0.0f - (m_gravity * dt);
                    }
                    continue;
                }

                // fetch ai component so we can sore the path
                auto& enemy_ai = m_registry.getComponent<CAI>(e);
                if(!m_registry.hasComponent<CBoundingBox>(e)) continue;
                auto& enemy_bb = m_registry.getComponent<CBoundingBox>(e);

                float diff_x = player_position.x - enemy_position.x;
                float diff_y = player_position.y - enemy_position.y;
                float distance = std::sqrt((diff_x * diff_x) + (diff_y * diff_y));

                if(distance <= 70.0f){
                    enemy_state.current_state = "attack";
                } else if(distance <= 450.f){
                    enemy_state.current_state = "chase";
                } else {
                    enemy_state.current_state = "patrol";
                }

                // flying enemy logic (A*)
                if(enemy_ai.is_flying){
                    if(enemy_state.current_state == "chase"){
                        enemy_ai.path_update_timer -= dt;

                    // calculate A* path every 0.1 second
                    // only run the heavy math every 0.1 seconds
                    if(enemy_ai.path_update_timer <= 0.0f){
                        // convert float world coordinates to integer grid cells (eg. 105.5f -> cell2)
                        CGridPos startGrid = {static_cast<int>(enemy_position.x / grid_size), static_cast<int>(enemy_position.y / grid_size)};
                        CGridPos targetGrid = {static_cast<int>(player_position.x / grid_size), static_cast<int>(player_position.y / grid_size)};
                        
                        // calculate the node path
                        enemy_ai.waypoints = calculatePath(startGrid, targetGrid);
                        enemy_ai.current_waypoint = 0;     // start at the first step
                        enemy_ai.path_update_timer = 0.1f; // reset the cooldown
                    }    
                    

                    // waypoint navigation (fly towards the waypoint)
                    if(!enemy_ai.waypoints.empty() && enemy_ai.current_waypoint < enemy_ai.waypoints.size()){

                        // get the grid cell we want to walk to, and convert it back to world float coordinates
                        CGridPos targetGrid = enemy_ai.waypoints[enemy_ai.current_waypoint];
                        float target_x = (targetGrid.col * grid_size) + half_grid;
                        float target_y = (targetGrid.row * grid_size) + half_grid;

                        float dir_x = target_x - enemy_position.x;
                        float dir_y = target_y - enemy_position.y;

                        // if we are close to the target tile target the next tile
                        // if the compare value gets smaller the enemeis get more laggy
                        // if the compare value gets larger the enemy movemets gets smother
                        if(std::abs(dir_x) < 25.0f && std::abs(dir_y) < 25.0f){
                            enemy_ai.current_waypoint++;
                        } else {
                            // normalize vector for smooth diagonal flying
                            float lenght = std::sqrt(dir_x * dir_x + dir_y * dir_y);
                            constexpr float flight_speed = 250.0f;

                            if(lenght > 0.0f) {
                                enemy_velocity.vx = (dir_x / lenght) * flight_speed;
                                // we pypass the gravity in flying enemies by subtracting it again after adding it in sColiision
                                enemy_velocity.vy = ((dir_y / lenght) * flight_speed) - (m_gravity * dt);
                            }
                        } 
                    } else{
                            // no path found -> hover in place
                            enemy_velocity.vx = 0.0f;
                            enemy_velocity.vy = 0.0f - (m_gravity * dt);
                        }
                    } else if(enemy_state.current_state == "patrol"){
                        // again stop flying and just hover when the player is out of range
                        enemy_velocity.vx = 0.0f;
                        enemy_velocity.vy = 0.0f - (m_gravity * dt);
                    } else if(enemy_state.current_state == "attack"){
                        // hover while the attack
                        enemy_velocity.vx = 0.0f;
                        enemy_velocity.vy = 0.0f - (m_gravity * dt);
                        enemy_ai.attack_timer -= dt;
                        if(enemy_ai.attack_timer <= 0.0f){
                            // imaginary bounding box
                            float reach = 40.0f;
                            float weapon_height = 50.0f;
                            float weapon_width = 50.0f;
                            // face the player to ensure the enemy is facing the player
                            enemy_position.facing_left = (player_position.x < enemy_position.x);
                            float weapon_x = enemy_position.facing_left ? (enemy_position.x - reach) : (enemy_position.x + reach);
                            float weapon_y = enemy_position.y;

                            // visual attck entity
                            Entity visual_attack = m_registry.createentity();
                            m_registry.addComponent(visual_attack, CTransform{weapon_x, weapon_y});
                            m_registry.addComponent(visual_attack, CLifespan{0.75f});
                            m_registry.addComponent(visual_attack, CBoundingBox{weapon_width, weapon_height});
                            m_registry.addComponent(visual_attack, CSprite{"tex_attack_effect", 11, 15, 48, 16});
                            m_registry.addComponent(visual_attack, CAnimation{5, 0.15f, 11, 15, 48});
                            m_registry.getComponent<CTransform>(visual_attack).facing_left = enemy_position.facing_left;

                            auto& player_bb = m_registry.getComponent<CBoundingBox>(m_player);
                            // check coliision agaimst the player
                            float diff_x_hit = std::abs(weapon_x - player_position.x);
                            float diff_y_hit = std::abs(weapon_y - player_position.y);
                            float min_x = (weapon_width + player_bb.width) / 2.0f;
                            float min_y = (weapon_height + player_bb.height) / 2.0f;

                            // process if hit
                            if(diff_x_hit < min_x && diff_y_hit < min_y){
                                auto& enemy_damage = m_registry.getComponent<CDamage>(e).damage;
                                m_registry.getComponent<CHealth>(m_player).health -= enemy_damage;
                            }
                            // reset the timer so the enemy dont spam damage
                            enemy_ai.attack_timer = 2.0f;
                        }

                    }
                }
                // ground enemy logic
                else{
                    if(enemy_state.current_state == "chase"){
                        // move horizontaly with small deadxone to allow a space for attack later
                        if(diff_x > 20.0f){
                            enemy_velocity.vx = 200.0f;
                        } else if(diff_x < -20.0f){
                            enemy_velocity.vx = -200.0f;
                        } else{
                            enemy_velocity.vx = 0.0f; //stop vibrating if uner or top of the enemy
                        }
                        if(diff_y < -40.0f && enemy_state.is_grounded && enemy_ai.attack_timer <= 0.0f){
                            enemy_velocity.vy = - 400.0f;
                            enemy_state.is_grounded = false;
                        }
                    
                } else if(enemy_state.current_state == "patrol"){
                    // kickstart movement if they are standing still
                    if(enemy_velocity.vx == 0.0f) enemy_velocity.vx = 50.0f;
                    
                    // detect if there is a ground a head of the enemy or it can fall
                    float checkdistance = (enemy_velocity.vx > 0.0f) ? (enemy_bb.width / 2.0f +  5.0f) : -(enemy_bb.width / 2.0f +5.0f);
                    float check_x = enemy_position.x + checkdistance;
                    float check_y_floor = enemy_position.y + (enemy_bb.height / 2.0f) + 2.0f;
                    float check_y_wall = enemy_position.y;

                    bool has_ground = false;
                    bool hit_wall = false;

                    // ask if there is any solid entity near the check point
                    for(Entity o : m_registry.bounding_boxes.getentities()){
                        if(e == o || o == m_player ||!m_registry.hasComponent<CBoundingBox>(o)) continue;
                        auto& other_tra = m_registry.getComponent<CTransform>(o);
                        auto& other_bb = m_registry.getComponent<CBoundingBox>(o);


                        float left    = other_tra.x -( other_bb.width / 2.0f);
                        float right   = other_tra.x + (other_bb.width / 2.0f);
                        float top    = other_tra.y - (other_bb.height / 2.0f);
                        float bottom = other_tra.y + (other_bb.height / 2.0f);

                        if(check_x > left && check_x < right && check_y_floor < bottom && check_y_floor > top){
                            has_ground = true;
                        }
                        if(check_x > left && check_x < right && check_y_wall > top && check_y_wall < bottom){
                            hit_wall = true;
                        }

                    }
                    if(!has_ground || hit_wall){
                        enemy_velocity.vx *= -1;
                    }

                } else if(enemy_state.current_state == "attack"){
                    // stand still while attacking
                    enemy_velocity.vx = 0.0f;
                    enemy_ai.attack_timer -= dt;
                    if(enemy_ai.attack_timer <= 0.0f){
                        float reach = 50.0f;
                        float weapon_width = 60.0f;
                        float weapon_height = 60.0f;

                        enemy_position.facing_left = (enemy_position.x > player_position.x);

                        float weapon_x = enemy_position.facing_left ? (enemy_position.x - reach) : (enemy_position.x + reach);
                        float weapon_y = enemy_position.y;
                        
                        Entity attack_effect = m_registry.createentity();
                        m_registry.addComponent(attack_effect, CTransform{weapon_x, weapon_y});
                        m_registry.addComponent(attack_effect, CBoundingBox{weapon_width, weapon_height});
                        m_registry.addComponent(attack_effect, CLifespan{0.75f});
                        m_registry.addComponent(attack_effect, CSprite{"tex_attack_effect", 11, 15, 48, 16});
                        m_registry.addComponent(attack_effect, CAnimation{5, 0.15f, 11, 15, 48});
                        m_registry.getComponent<CTransform>(attack_effect).facing_left = enemy_position.facing_left;

                        auto& player_bb = m_registry.getComponent<CBoundingBox>(m_player);

                        float diff_x_hit = std::abs(weapon_x - player_position.x);
                        float diff_y_hit = std::abs(weapon_y - player_position.y);
                        float min_x = (weapon_width + player_bb.width) / 2.0f;
                        float min_y = (weapon_height + player_bb.height) / 2.0f;

                        if(diff_x_hit < min_x && diff_y_hit < min_y){
                            m_registry.getComponent<CHealth>(m_player).health -= m_registry.getComponent<CDamage>(e).damage;
                        }
                        enemy_ai.attack_timer = 2.0f;
                    }
                }

            }
        }
        if(m_registry.hasComponent<CHealth>(m_player)){
            if(m_registry.getComponent<CHealth>(m_player).health <= 0.0f){
                m_currentState = GameState::GameOver;
            }
        }
        break;
        }

        case GameState::GameOver: {
        // nothing
        break;
        }

        case GameState::MainMenu: {
        // nothing
        break;
        }

    }
}

// our beloved fancy A*
std::vector<CGridPos> Engine::calculatePath(CGridPos start, CGridPos target){
    std::vector<CGridPos> path;

    if(m_navGrid.empty()) return path;
    int rows = m_navGrid.size();
    if (rows == 0) return path;
    int cols = m_navGrid[0].size();

    //bounds check ensure start / target rows have enough columns
    if(start.col < 0 || start.col >= cols || start.row < 0 || start.row >= m_navGrid[start.row].size()
    || target.col < 0 || target.col >= cols || target.row < 0 || target.row >= m_navGrid[start.row].size()){
        return path;
    }

    std::priority_queue<CPathNode> openList;
    std::vector<std::vector<bool>> closedList(rows, std::vector<bool>(cols, false));

    // to avoid row pointer mangment issues with CPathNodes* parent
    // we use a 2D array to map where each node came from.
    std::vector<std::vector<CGridPos>> parentMap(rows, std::vector<CGridPos>(cols, {-1, -1}));

    CPathNode startNode{start.col, start.row, 0.0f, 0.0f, 0.0f, nullptr};
    openList.push(startNode);

    // up, down, left, right
    int dRow[] = {-1, 1, 0, 0};
    int dCol[] = {0, 0, -1, 1};

    while (!openList.empty()) {
        CPathNode current = openList.top();
        openList.pop();

        if (current.x == target.col && current.y == target.row){
            // reconstruct the path
            CGridPos currPos = {current.x, current.y};
            while (currPos.col != -1 && currPos.row != -1){
                path.push_back(currPos);
                currPos = parentMap[currPos.row][currPos.col];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if(closedList[current.y][current.x]) continue;
        closedList[current.y][current.x] = true;

        for(int i = 0; i < 4; i++) {
            int newCol = current.x + dCol[i];
            int newRow = current.y + dRow[i];

            // grid boundry check
            if(newCol >= 0 &&newCol < cols && newRow >= 0 && newRow < rows){

                // check if the specific yaml row was drawn shorter than the others 
                if(newCol < m_navGrid[newRow].size()){

                    // now its safe to check the vector
                    if(m_navGrid[newRow][newCol] == 0 && !closedList[newRow][newCol]){
                        
                        float gCost = current.gCost + 1.0f;
                        float hCost = std::abs(newCol - target.col) + std::abs(newRow - target.row);
                        float fCost = gCost + hCost;

                        parentMap[newRow][newCol] = {current.x, current.y}; // Track parent safely
                        openList.push(CPathNode{newCol, newRow, gCost, hCost, fCost, nullptr});
                    }
                }
            }
        }
    }
    return path;
}