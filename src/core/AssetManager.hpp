#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <memory>

class AssetManager{
    private:
        std::map<std::string, sf::Texture> m_textures;
        std::map<std::string, std::unique_ptr<sf::Shader>> m_shaders;

    public:
        AssetManager();
        ~AssetManager();

        // load texture from disk and store it by name
        void addtexture(const std::string& name, const std::string& path);
        void addshader(const std::string& name, const std::string& fragment_path);

        // get a refrence to a previuosly loaded texture
        sf::Texture& gettexture(const std::string& name);
        // return a pointer (cannot copy shaders)
        sf::Shader* getshader(const std::string& name);

};