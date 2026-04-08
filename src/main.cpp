#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

int main() {
    // 1. Initialize the SFML Window
    std::cout << "[TRACE] Phase 1: Initializing Engine Window..." << std::endl;
    sf::RenderWindow window(sf::VideoMode(800, 600), "Project Eat Meat - Engine Test");
    window.setFramerateLimit(60);

    // 2. Initialize ImGui
    std::cout << "[TRACE] Phase 2: Booting ImGui Overlay..." << std::endl;
    ImGui::SFML::Init(window);

    sf::Clock deltaClock;

    std::cout << "[TRACE] Phase 3: Entering Main Game Loop!" << std::endl;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed)
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // ImGui test panel
        ImGui::Begin("Engine Status");
        ImGui::Text("Phase 3: Loop Running");
        ImGui::End();

        window.clear(sf::Color::Black);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}