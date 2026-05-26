#include "Engine.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <imgui.h>
#include <imgui-SFML.h>
#include "DatabaseManager.hpp"


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
            load_level("rooms/level01.yaml");
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
                m_sounds.playsound("sfx_player_shooting");
                

            }

        }

    }
// F5: Save Game
            if(event.key.code == sf::Keyboard::F5) {
                if(m_player != -1 && m_registry.hasComponent<CTransform>(m_player) && m_registry.hasComponent<CHealth>(m_player)) {
                    
                    auto& pTransform = m_registry.getComponent<CTransform>(m_player);
                    auto& pHealth = m_registry.getComponent<CHealth>(m_player);

                    PlayerSaveData saveData;
                    saveData.saveID = 1;               
                    saveData.classID = 1;              
                    saveData.currentLevel = 1;         // NEW: Set this to match the DB!
                    saveData.currentHealth = pHealth.health; 
                    saveData.playerX = pTransform.x;   
                    saveData.playerY = pTransform.y;   
                    saveData.dungeonFloor = 1;

                    if (DatabaseManager::getInstance().savePlayerState(saveData)) {
                        std::cout << "-> GAME SAVED TO DATABASE SUCCESSFULLY!\n";
                    }
                }
            }

            // F9: Load Game
            if(event.key.code == sf::Keyboard::F9) {
                PlayerSaveData loadedData = DatabaseManager::getInstance().loadPlayerState(1);
                
                if(loadedData.saveID != 0 && m_player != -1) {
                    auto& pTransform = m_registry.getComponent<CTransform>(m_player);
                    auto& pHealth = m_registry.getComponent<CHealth>(m_player);

                    pTransform.x = loadedData.playerX;
                    pTransform.y = loadedData.playerY;
                    pHealth.health = loadedData.currentHealth;

                    // --- THE FIX: Wipe out falling momentum! ---
                    if (m_registry.hasComponent<CVelocity>(m_player)) {
                        m_registry.getComponent<CVelocity>(m_player).vx = 0.0f;
                        m_registry.getComponent<CVelocity>(m_player).vy = 0.0f;
                    }

                    std::cout << "-> GAME LOADED FROM DATABASE SUCCESSFULLY! Welcome back.\n";
                } else {
                    std::cout << "-> NO SAVE FILE FOUND!\n";
                }
            }
            
  
}