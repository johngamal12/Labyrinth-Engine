#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>

class AssetManager{
    private:
        std::map<std::string, sf::Texture> m_textures;

    public:
        AssetManager();
        ~AssetManager();

        // load texture from disk and store it by name
        void addtexture(const std::string& name, const std::string& path);

        // get a refrence to a previuosly loaded texture
        sf::Texture& gettexture(const std::string& name);

};