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


void AssetManager::addshader(const std::string& name, const std::string& fragment_path){
    // check if the user's GPU actually supports shaders
    if(!sf::Shader::isAvailable()){
        std::cerr<< "shaders are not supported by this GPU";
        return;
    }
    auto shader = std::make_unique<sf::Shader>();

    // sf::Shader::Fragment tells sfml that we are changing pixel color not vertex geometry
    if(!shader->loadFromFile(fragment_path, sf::Shader::Fragment)){
        std::cerr<< "error falied to load shader from "<< fragment_path <<"\n";
        return;
    }

    m_shaders[name] = std::move(shader);
    std::cout<< "asste loaded (shader)"<< name<< "->"<< fragment_path<< "\n";
}


// retrun a pointer so we dont actually copy it
sf::Shader* AssetManager::getshader(const std::string& name){
    auto pointertoshader = m_shaders.find(name);
    if(pointertoshader == m_shaders.end()){
        std::cerr<< "warning shader"<< name<< "not found\n";
        return nullptr;
    }
    // .get() is a builtin function it reaches inside the smart pointer and grabs the raw memory address
    return pointertoshader->second.get();
}