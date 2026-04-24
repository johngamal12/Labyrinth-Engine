#pragma once
#include "Types.hpp"
#include "SparseSet.hpp"
#include "components/Components.hpp"
#include <queue>
#include <cassert>

class Registry {
    private:
        uint32_t m_entitiescount  = 0;    // the count of the current alive entities
        std::vector<Entity> m_todestroy;  // the waiting room for the entities that died this frame
        std::queue<Entity> m_reusableIDs; // the dead entities that we can recycle thier ids

    public:
        // the registry owns the memory arrays 
        SparseSet<CTransform> transforms;
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
            // put the daad entities in the waiting room not the graveyard yet (wait here until the frame ends)
            m_todestroy.push_back(e);

        }

    // this function safely clean the data before getting to the render manager
    void cleardeadentities(){
        // loop through all entities in the waiting room
       for(Entity e : m_todestroy){
            //strip the flesh (delete the date from them)
            if(transforms.has(e)) transforms.remove(e);
            // push the clean empty id to the recycling graveyard
            m_reusableIDs.push(e);

       }
       // empty the waiting room for the next frame
       m_todestroy.clear();

    }

};
