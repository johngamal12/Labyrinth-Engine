#include "Engine.hpp"

void Engine::sHealth() {
    for(Entity e : m_registry.healths.getentities()){
        if(!m_registry.hasComponent<CState>(e)) continue;
        auto& entity_health = m_registry.getComponent<CHealth>(e);
        auto& entity_state = m_registry.getComponent<CState>(e);

        if(entity_health.health <= 0.0f && entity_state.current_state != "dead"){
            entity_state.current_state = "dead";
            entity_state.is_locked = false;

            if(m_registry.hasComponent<CVelocity>(e)){
                m_registry.getComponent<CVelocity>(e).vx = 0.0f;   
            }

            if(m_registry.hasComponent<CInput>(e)){
                auto& input = m_registry.getComponent<CInput>(e);
                input.attack = input.can_jump = input.down = input.left = input.right = input.up = false;
            }

            if(e == m_player){
                m_sounds.playsound("sfx_player_death");
            }else{
                if(!m_registry.hasComponent<CLifespan>(e)){
                    m_registry.addComponent(e, CLifespan{1.5f});
                }
                m_sounds.playsound("sfx_enemy_death");
            }
        }
    }
}