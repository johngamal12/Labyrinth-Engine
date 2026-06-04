#include"Engine.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>


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
                        m_registry.addComponent(Brick, CSprite{"tex_grass"});
                        gridRow.push_back(1);
                    } else if(entity_to_load == 'G'){
                    
                        Entity Brick = m_registry.createentity();
                        m_registry.addComponent(Brick, CTransform{center_x, center_y});
                        m_registry.addComponent(Brick, CBoundingBox{grid_size, grid_size});
                        m_registry.addComponent(Brick, CSprite{"tex_ground"});
                        gridRow.push_back(1); // to let A* knows its a solid object
                    }else{
                        gridRow.push_back(0); // to let it know its an empty space
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
                        m_registry.addComponent(m_player, CShader{"shd_flash"});
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
                        m_registry.addComponent(Enemy, C_AI{});
                        m_registry.addComponent(Enemy, CDamage{15.0f});
                        m_registry.addComponent(Enemy, CShader{"shd_flash"});

                    }
                    if(entity_to_load == 'F'){
                        Entity Enemy = m_registry.createentity();
                        m_registry.addComponent(Enemy, CTransform{center_x, center_y});
                        m_registry.addComponent(Enemy, CHealth{100.0f}); 
                        m_registry.addComponent(Enemy, CBoundingBox{40.0f, 40.0f}); 
                        m_registry.addComponent(Enemy, CVelocity{0.0f, 0.0f});
                        m_registry.addComponent(Enemy, CSprite{"flying_enemy_tex", 59, 33, 65, 62});
                        m_registry.addComponent(Enemy, CAnimation{15, 0.1f, 59, 33, 192}); 
                        m_registry.addComponent(Enemy, CState{"idle"});
                        m_registry.addComponent(Enemy, C_AI{{}, 0, 0.0f, true});        
                        m_registry.addComponent(Enemy, CDamage{20.0f});
                        m_registry.addComponent(Enemy, CShader{"shd_flash"}); 
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