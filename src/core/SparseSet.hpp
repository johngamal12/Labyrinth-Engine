#pragma once
#include "Types.hpp"
#include <vector>
#include <cassert>

// this class guarantees that our POD data structs stays in contgiuos L1 cahce arrays
// guarantees sontigiuos memory for componenrs for type T
template <typename T>
class SparseSet {
    private:
        std::vector<T> m_data;               // dense array of actual component data
        std::vector<Entity> m_densetoentity; // maps dense array index back to entity id
        std::vector<size_t> m_entitytodense; // sparse array: maps entity id to dense array index

    public:
       SparseSet() {
        //preallocate sparse array to prevent reallocation
        m_entitytodense.resize(MAX_ENTITIES, (size_t)-1); 
       }

    void insert(Entity e, T component){
        assert(e < MAX_ENTITIES);              // check to not exceed the max size
        assert(!has(e));                       // check if that entity already has that component
        size_t index = m_data.size();          // finds the next empty slot in the end of the dense data array
        m_entitytodense[e] = index;            // updates the sparse arrays to tell the entity e that its data is now located at 'index'
        m_densetoentity.push_back(e);          // updates the reverse map to say that the data at this new slot now belongs to entity e
        m_data.push_back(std::move(component));// move the actual data into the empty slot

    }

    void remove(Entity e){
        assert(e < MAX_ENTITIES);                    // again check if the entity's id is valid
        assert(has(e));                              // check if this entity has that data before we try to delete it 
        size_t index = m_entitytodense[e];           // looks up where the data to be removed is inside the m_data
        Entity lastentity = m_densetoentity.back();  // finds the entity that wons the last piece of data in dense array
        if(e != lastentity){                         // only swap if the entity to remove is not already the last entity
        m_data[index] = std::move(m_data.back());    // takes the last piece of data in the array and overwrites it on the data to be removed
        m_densetoentity[index] = lastentity;         // updates the reverse map to say the data we just overwrote now benlongs to the last entity
        m_entitytodense[lastentity] = index;         // updates the sparse array to tell it that its data just moved to to the 'index'
        }
        m_data.pop_back();                           // delets the empty slot at the very end of the array by 1
        m_densetoentity.pop_back();                  // shrinks the reverse map by 1 to match
        m_entitytodense[e] = (size_t)-1;             // it tells that entity that it no longer has that component

    }

    T& get(Entity e){
        assert(e < MAX_ENTITIES);           // normal check
        assert(has(e));                     // again check as prevoius
        return m_data[m_entitytodense[e]];  // return the component of that entity

    }

    bool has(Entity e) const{
        return e < MAX_ENTITIES && m_entitytodense[e] != (size_t)-1; // check if that entity has that component 

    }

    //return the dense array of entities that has that component so we can loop through them
    const std::vector<Entity>& getentities() const {
        return m_densetoentity;
    }

};