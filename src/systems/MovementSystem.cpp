#include "Engine.hpp"
// i dont remeber i fi need these two here but i will just let them be here
// #include <algorithm> //for std::clamp
// #include <cmath> // for std::abs 


// we pass dt to the update function because if for some reason the machine is not good enough to produce 60 fps at least the movement stays the same
// for example if the fps is 30 and the update depends on the frames it will run in half speed but because it is time dependent it will run less smothly but the same speed
void Engine::sUpdate(float dt){
    switch(m_currentState){
        case GameState::Playing:{
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
            std::vector<Entity> bullets = m_registry.lifespans.getentities();
            for(Entity bullet : bullets){
                if(!m_registry.hasComponent<CLifespan>(bullet)) continue;

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

            // melee combat
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
        }

        case GameState::MainMenu:{
        //game logic is paused in the menu 
        break;
        }

        case GameState::GameOver:{
        //the player is dead we just wait to click on restart
        break;
        }

    }
}