#include "Engine.hpp"
#include <cmath>
#include <algorithm>
#include <vector>
#include <queue>

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
                } else if(distance <= 600.f){
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

                        // on long distances move at least for 3 tiles before recalaculating A* again to avoid vibrating enemies
                        bool allowed_to_calculate = true;

                        if(enemy_ai.waypoints.size() > 15){
                            // since we are starting at waypoint 1 then moving 3 tiles means that index must be 4
                            if(enemy_ai.waypoints.size() > 70 && enemy_ai.current_waypoint < 11){
                                allowed_to_calculate = false;
                            
                            }                            
                            if(enemy_ai.waypoints.size() > 35 && enemy_ai.current_waypoint < 10){
                                allowed_to_calculate = false;
                            
                            }
                            else if(enemy_ai.waypoints.size() > 25 && enemy_ai.current_waypoint < 7){
                                allowed_to_calculate = false;
                            
                            } 
                            else if(enemy_ai.current_waypoint < 4){
                                allowed_to_calculate = false;
                            
                            }                                                       
                            // keep the timer at 0 so the eact millisecond the enemy hit the next tile it triggers on the next frame 
                            enemy_ai.path_update_timer = 0.0f;
                        }
                        
                        if(allowed_to_calculate){
                            CGridPos startGrid = {static_cast<int>(enemy_position.x / grid_size), static_cast<int>(enemy_position.y / grid_size)};
                            CGridPos targetGrid = {static_cast<int>(player_position.x / grid_size), static_cast<int>(player_position.y / grid_size)};

                            
                            // calculate the node path
                            enemy_ai.waypoints = calculatePath(startGrid, targetGrid);
                            if(enemy_ai.waypoints.size() > 1){
                                enemy_ai.current_waypoint = 1;
                            } else {
                                enemy_ai.current_waypoint = 0;
                            }
                            enemy_ai.path_update_timer = 0.2f; // reset the cooldown and since for long pathes the 3 tiles blocks will override it anyway so its a decent number
                        }    
                    }
                    // waypoint navigation (fly towards the waypoint)
                    if(!enemy_ai.waypoints.empty() && enemy_ai.current_waypoint < enemy_ai.waypoints.size()){

                        // get the grid cell we want to walk to, and convert it back to world float coordinates
                        CGridPos targetGrid = enemy_ai.waypoints[enemy_ai.current_waypoint];
                        float target_x = (targetGrid.col * grid_size) + half_grid;
                        float target_y = (targetGrid.row * grid_size) + half_grid;

                        float dir_x = target_x - enemy_position.x;
                        float dir_y = target_y - enemy_position.y;

                        // normalize vector for smooth diagonal flying
                        float lenght = std::sqrt(dir_x * dir_x + dir_y * dir_y);
                        constexpr float flight_speed = 250.0f;
                        float distance_this_frame = flight_speed * dt;

                        // if the enemy is close to the center of the grid by 1 pixels teleport him to the center of the grid so it doesnot vibrate
                        if(lenght <= distance_this_frame + 1.0f){

                            // snap it to the center
                            enemy_position.x = target_x;
                            enemy_position.y = target_y;
                            enemy_ai.current_waypoint++;

                            // fetch the next waypoint to redirect momentum this frame
                            if(enemy_ai.current_waypoint < enemy_ai.waypoints.size()){
                                targetGrid = enemy_ai.waypoints[enemy_ai.current_waypoint];
                                target_x = (targetGrid.col * grid_size) + half_grid;
                                target_y = (targetGrid.row * grid_size) + half_grid;

                                dir_x = target_x - enemy_position.x;
                                dir_y = target_y - enemy_position.y;
                                lenght = std::sqrt(dir_x * dir_x + dir_y * dir_y);
                            } else{
                                enemy_velocity.vx = 0.0f;
                                enemy_velocity.vy = 0.0f - (m_gravity * dt);
                                lenght = 0.0f;
                            }
                        }

                        if(lenght > 0.0f) {
                            enemy_velocity.vx = (dir_x / lenght) * flight_speed;
                            // we pypass the gravity in flying enemies by subtracting it again after adding it in sColiision
                            enemy_velocity.vy = ((dir_y / lenght) * flight_speed) - (m_gravity * dt);
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

    // track the best gcost to prevent vibrating
    std::vector<std::vector<float>> gcostmap(rows, std::vector<float>(cols, std::numeric_limits<float>::max()));
    gcostmap[start.row][start.col] = 0.0f;

    CPathNode startNode{start.col, start.row, 0.0f, 0.0f, 0.0f, nullptr};
    openList.push(startNode);

    // up, down, left, right and diagonals
    int dRow[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dCol[] = {0, 0, -1, 1, -1, 1, -1, 1};
    float move_cost[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.414f, 1.414f, 1.414f, 1.414f};

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

        for(int i = 0; i < 8; i++) {
            int newCol = current.x + dCol[i];
            int newRow = current.y + dRow[i];

            // grid boundry check
            if(newCol >= 0 &&newCol < cols && newRow >= 0 && newRow < rows){

                // check if the specific yaml row was drawn shorter than the others 
                if(newCol < m_navGrid[newRow].size()){

                    // now its safe to check the vector
                    if(m_navGrid[newRow][newCol] == 0 && !closedList[newRow][newCol]){


                        // if the next move is diagonal you cant sqeeze through it
                        if(i >= 4){
                            int checkrow1 = current.y + dRow[i];
                            int checkcol1 = current.x;
                            int checkrow2 = current.y;
                            int checkcol2 = current.x + dCol[i];

                            if(m_navGrid[checkrow1][checkcol1] != 0 || m_navGrid[checkrow2][checkcol2] != 0){
                                continue;
                            }
                        }
                        float newgcost = current.gCost + move_cost[i];

                        if(newgcost < gcostmap[newRow][newCol]){
                                                    
                            gcostmap[newRow][newCol] = newgcost;
                            parentMap[newRow][newCol] = {current.x, current.y};
                            
                            float dx = std::abs(newCol - target.col);
                            float dy = std::abs(newRow - target.row);
                            // use pythagorean theorm to calculate the exact straight line from current node to player
                            // note that is for giving an approximation for the lenght it doesnot count walls or any real path just the shortest line
                            float hcost = std::sqrt(dx * dx + dy * dy);
                            float fcost = hcost + newgcost;
                            openList.push(CPathNode{newCol, newRow, newgcost, hcost, fcost, nullptr});

                        }
                    }
                }
            }
        }
    }
    return path;
}