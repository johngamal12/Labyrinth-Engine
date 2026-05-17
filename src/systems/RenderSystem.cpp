#include "Engine.hpp"
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>


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
            camera.setCenter(player_position.x,player_position.y);

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
            load_level("rooms/level02.yaml"); // this line what restart the game
            m_currentState = GameState::Playing;
        }
        ImGui::End();

    }
    // ==========================================
    // AI DEBUG RENDERING (Temporary Testing Code)
    // ==========================================
    for (Entity e : m_registry.ais.getentities()) {
        // Make sure we only draw for enemies that actually have an active path
        auto& enemy_ai = m_registry.getComponent<CAI>(e);
        
        if (enemy_ai.waypoints.empty()) continue;

        // Draw a small red box for every node in the current path
        for (size_t i = enemy_ai.current_waypoint; i < enemy_ai.waypoints.size(); i++) {
            const CGridPos& node = enemy_ai.waypoints[i];

            sf::RectangleShape debugSquare(sf::Vector2f(10.0f, 10.0f));
            debugSquare.setFillColor(sf::Color::Red);
            debugSquare.setOrigin(5.0f, 5.0f); // Center the origin

            // Convert the grid integer back to world float coordinates
            float world_x = (node.col * 40.0f) + 20.0f; // 40.0f is grid size, 20.0f is half_grid
            float world_y = (node.row * 40.0f) + 20.0f;
            
            debugSquare.setPosition(world_x, world_y);
            m_window->draw(debugSquare);
        }
    }

    //imgui hook render
    ImGui::SFML::Render(*m_window);
    // third push the drawn frame to the window 
    m_window->display();
    

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
        std::string check_state = new_state;
        if (check_state == "patrol") check_state = "idle"; // Skeletons use the idle sprite to patrol

        bool needs_update = false;

        // Logic (Check Y-coordinates instead of string names)
        if (entity_sprit.name == "flying_enemy_tex") {
            if (new_state == "idle"   && entity_animation.start_pixel_y != 33)  needs_update = true;
            if (new_state == "patrol" && entity_animation.start_pixel_y != 33)  needs_update = true;
            if (new_state == "chase"  && entity_animation.start_pixel_y != 257) needs_update = true;
            if (new_state == "attack" && entity_animation.start_pixel_y != 365) needs_update = true;
            if (new_state == "dead"   && entity_animation.start_pixel_y != 706) needs_update = true;
        } 
        // Player & Skeleton Logic (Use the string .find() trick)
        else {
            if (entity_sprit.name.find(check_state) == std::string::npos) {
                needs_update = true;
            }
        }

        // apply new sprites if state has changed
        if(needs_update || (new_state == "attack" && entity_animation.current_frame == 0)){

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

                if(!m_registry.getComponent<CAI>(e).is_flying){
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
                } else {
                    if(new_state == "idle" || new_state == "patrol"){
                        entity_sprit = CSprite{"flying_enemy_tex", 59, 33, 65, 62};
                        entity_animation =  CAnimation{15, 0.1f, 59, 33, 192};
                    }
                    if(new_state == "chase"){
                        entity_sprit = CSprite{"flying_enemy_tex", 47, 257, 97,62};
                        entity_animation =  CAnimation{6, 0.2f, 47, 257, 192};
                    } 
                    if(new_state == "attack"){
                        entity_sprit = CSprite{"flying_enemy_tex", 69, 365, 100, 63};
                        entity_animation =  CAnimation{7, 0.25f, 69, 365, 192};
                        entity_state.is_locked = true;
                    }
                    if(new_state == "dead"){
                        entity_sprit = CSprite{"flying_enemy_tex", 63, 706, 81, 62};
                        entity_animation =  CAnimation{11, 0.14f, 63, 706, 192};
                    }
 
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
