#include "AssetManager.hpp"
#include <iostream>

AssetManager::AssetManager() = default;
AssetManager::~AssetManager() = default;


void AssetManager::addtexture(const std::string& name, const std::string& path){

    sf::Texture texture;
    if(!texture.loadFromFile(path)){
        std::cerr<< "error failed to load texture"<< path <<"\n";
        return;
    }

    m_textures[name] = std::move(texture);

    std::cout<< "asset loaded"<< name << "->"<< path<< "\n";

}


sf::Texture& AssetManager::gettexture(const std::string& name){
    auto pointertotexture = m_textures.find(name); // search only no insertion

    if(pointertotexture == m_textures.end()){ // if the key is not found an m_textures.end() is returned
        std::cerr<< "warning texture"<< name<< "not found"<< "\n";

        // create a temporary texture to replace it
        static sf::Texture missingtexture;
        static bool iscreated = false; // static to vreate the dummy texture only once
        
        if(!iscreated){
            missingtexture.create(40, 40);
            sf::Image image;
            image.create(40, 40, sf::Color::Magenta);
            missingtexture.update(image);
            iscreated = true;
        }
        return missingtexture;

    }
    return pointertotexture->second; //this return the actual texture
}
