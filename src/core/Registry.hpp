#pragma once
#include "Types.hpp"
#include "SparseSet.hpp"
#include <queue>

class Registry {
    private:
        uint32_t m_entitiescount  = 0;
        std::queue<Entity> m_reusableIDs;

    public:
        Entity createentity(){
            if(!m_reusableIDs.empty()){
                // if there is recycable materials why create new!
                Entity e = m_reusableIDs.front();
                m_reusableIDs.pop();
                // we cant type Entity e = m_reusableIDs.pop(); because c++ forbidends it because
                // (if we tried to copy but there is no available memory and we tried to pop the data is lost but not copied but when we do it in two lines its more safe "c++ to the rescue")
                return e;

            }
            else {
                assert(m_entitiescount < 10000 && "entity count limit reached not allowed to create more"); 
                // check if exceeded the limit
                Entity e = m_entitiescount++;
                return e;
                // those two lines can be compressed to 'return m_entitiescount++;'  but i write it like that to keep it similar to condition true statments

            }
        }

        void destroyentity(Entity e){
            m_reusableIDs.push(e);

        }

};
