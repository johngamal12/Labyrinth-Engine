#include <iostream>
#include "core/Registry.hpp"
#include "core/SparseSet.hpp"

// define strict POD data to test (no logic just data)

struct transform{
    float x,y;
};

struct health {
    int hp;
};

int main(){
    std::cout<<"ECS basic test";

    // instantiate our infrastructure managers
    Registry registery;
    SparseSet <transform> transform;
    SparseSet <health> health;

    // test O(1) entity create
    std::cout<<"\ncreating entities";
    Entity player = registery.createentity();
    Entity enemy1 = registery.createentity();
    Entity enemy2 = registery.createentity();

    std::cout<<"\nplayer id "<< player<< " enemy1 id "<< enemy1<< " enemy2 id "<< enemy2;

    //test data-oriented component insertion
    std::cout<<"\ntest2 adding compopnents\n";

    transform.insert(player, {10.0f,10.0f});
    health.insert(player,{20});

    transform.insert(enemy1, {5.0f,5.0f});
    health.insert(enemy1,{10});

    std::cout<<"player's health: "<< health.get(player).hp <<"\n";
    std::cout<<"player's position "<< transform.get(player).x<< ", "<< transform.get(player).y <<"\n";

    std::cout<<"first enemy's health: "<< health.get(enemy1).hp <<"\n";
    std::cout<<"first enemy's position "<< transform.get(enemy1).x<< ", "<< transform.get(enemy1).y <<"\n";

    //test O(1) swap and pop component removal
    std::cout<<"test3 deleting enemy's health\n";
    health.remove(enemy1);
    if (!health.has(enemy1)){
        std::cout<<"succes enemy's helath is removed contiguous memory maintained \n";

    }

    //test id recycling
    std::cout<<"test4 destroing player notice it has id of "<< player<< "\n";
    registery.destroyentity(player);

    std::cout<<"spawning new bullet\n";
    Entity bullet = registery.createentity();
    std::cout<<"the bullet's id "<< bullet << "\t notice it has id of 0 because recycled the player's id";


    std::cin.get(); // pauses the game so you can see the external consol;
    return 0;
}