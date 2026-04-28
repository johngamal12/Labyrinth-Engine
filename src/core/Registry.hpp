#pragma once
#include "Types.hpp"
#include "SparseSet.hpp"
#include "components/Components.hpp"
#include <queue>
#include <cassert>
#include <type_traits> // required for std::is_same_v

class Registry {
    private:
        uint32_t m_entitiescount  = 0;    // the count of the current alive entities
        std::vector<Entity> m_todestroy;  // the waiting room for the entities that died this frame
        std::queue<Entity> m_reusableIDs; // the dead entities that we can recycle thier ids

    public:
        // the registry owns the memory arrays 
        SparseSet<CTransform> transforms;
        SparseSet<CVelocity> velocities;
        SparseSet<CShape> shapes;
        SparseSet<CInput> inputs;
        SparseSet<CBoundingBox> bounding_boxes;
        SparseSet<CLifespan> lifespans;
        SparseSet<CDamage> damages;
        SparseSet<CHealth> healths;

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
            if(velocities.has(e)) velocities.remove(e);
            if(shapes.has(e)) shapes.remove(e);
            if(inputs.has(e)) inputs.remove(e);
            if(bounding_boxes.has(e)) bounding_boxes.remove(e);
            if(lifespans.has(e)) lifespans.remove(e);
            if(damages.has(e)) damages.remove(e);
            if(healths.has(e)) healths.remove(e);

            // push the clean empty id to the recycling graveyard
            m_reusableIDs.push(e);

       }
       // empty the waiting room for the next frame
       m_todestroy.clear();

    }

    // compoenet managment helpers (clean API for all systems)
    //these forward to the correct SparseSetusing compile-time constexpr
    // it has zero run-time cost its nicer to use in systems and more readable

    template <typename T>
    void addComponent (Entity e,T component){
        if constexpr (std::is_same_v<T, CTransform>){
            transforms.insert(e, std::move(component));
        } else if constexpr (std::is_same_v<T, CVelocity>){
            velocities.insert(e, std::move(component));
        } else if constexpr (std::is_same_v<T, CShape>){
            shapes.insert(e, std::move(component));
        } else if constexpr (std::is_same_v<T, CInput>){
            inputs.insert(e, std::move(component));
        } else if constexpr(std::is_same_v<T, CBoundingBox>){
            bounding_boxes.insert(e, std::move(component));
        } else if constexpr(std::is_same_v<T, CLifespan>){
            lifespans.insert(e, std::move(component));
        } else if constexpr (std::is_same_v<T, CDamage>){
            damages.insert(e, std::move(component));
        } else if constexpr (std::is_same_v<T, CHealth>){
            healths.insert(e, std::move(component));
        } else{
            static_assert(sizeof(T) == 0, "Unknown component type add it to addComponent template");
        }
    }

    template <typename T>
    T& getComponent (Entity e){
        if constexpr (std::is_same_v<T, CTransform>){
            return transforms.get(e);
        } else if constexpr (std::is_same_v<T, CVelocity>){
            return velocities.get(e);
        } else if constexpr (std::is_same_v<T, CShape>){
            return shapes.get(e);
        } else if constexpr (std::is_same_v<T, CInput>){
            return inputs.get(e);
        } else if constexpr (std::is_same_v<T, CBoundingBox>){
            return bounding_boxes.get(e);
        } else if constexpr (std::is_same_v<T, CLifespan>){
            return lifespans.get(e);
        } else if constexpr (std::is_same_v<T, CDamage>){
            return damages.get(e);
        } else if constexpr (std::is_same_v<T, CHealth>){
            return healths.get(e);
        } else {
        static_assert(sizeof(T) == 0, "Unknown component type add it to getComponent template");
        }
    }

    template <typename T>
    bool hasComponent(Entity e) const{
        if constexpr (std::is_same_v<T, CTransform>){
            return transforms.has(e);
        } else if constexpr (std::is_same_v<T, CVelocity>){
            return velocities.has(e);
        } else if constexpr (std::is_same_v<T, CShape>){
            return shapes.has(e);
        } else if constexpr (std::is_same_v<T, CInput>){
            return inputs.has(e);
        } else if constexpr (std::is_same_v<T, CBoundingBox>){
            return bounding_boxes.has(e);
        } else if constexpr (std::is_same_v<T, CLifespan>){
            return lifespans.has(e);
        } else if constexpr (std::is_same_v<T, CDamage>){
            return damages.has(e);
        } else if constexpr (std::is_same_v<T, CHealth>){
            return healths.has(e);
        }
        return false;
    }

};
