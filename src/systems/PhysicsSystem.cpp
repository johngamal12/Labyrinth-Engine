#include "Engine.hpp"
#include <cmath>
#include <algorithm> 

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
            
            //here is the potimized function of c++
            const float half_width  = (entity_bounding_box.width / 2.0f);
            
            // constexpr float window_y = 720.0f; 
            // const float half_height = (entity_bounding_box.height/ 2.0f);
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
            // If anything falls off the bottom of the giant map, kill it!
            if(entity_transform.y > 3000.0f){
                if (e == m_player) {
                    m_currentState = GameState::GameOver;
                } else {
                    m_registry.destroyentity(e);
                }
                continue; // Skip the rest of collision for this frame
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
                    { 
                        bool A_is_flying = m_registry.hasComponent<C_AI>(entity_A) && m_registry.getComponent<C_AI>(entity_A).is_flying;
                        bool B_is_flying = m_registry.hasComponent<C_AI>(entity_B) && m_registry.getComponent<C_AI>(entity_B).is_flying;

                        if(a_can_move && b_can_move)
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
                                    m_registry.getComponent<CState>(entity_A).is_grounded = true;
                                }
                                if(!A_is_flying) m_registry.getComponent<CVelocity>(entity_A).vy = 0.0f;
                            } else 
                            {
                                ent_A_Tra.y += push_value;
                                ent_B_Tra.y -= push_value;
                                if(!B_is_flying) m_registry.getComponent<CVelocity>(entity_B).vy = 0.0f;
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
                                if(!B_is_flying) m_registry.getComponent<CVelocity>(entity_B).vy = 0.0f;    
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
                                if(!A_is_flying) m_registry.getComponent<CVelocity>(entity_A).vy = 0.0f;
                            } else
                            {
                                ent_A_Tra.y += overlap_y; 
                            }
                        }
                    }
                }

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
        m_registry.addComponent(bullet, CSprite{"tex_bullet", 117, 53, 7, 5, 30, 30});

    }
}
