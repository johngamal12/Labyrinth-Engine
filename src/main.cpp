#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

int main() {
    // 1. Initialize the SFML Window (Phase 3: Render Dispatch)
    sf::RenderWindow window(sf::VideoMode(800, 600), "Project Eat Meat - Engine Test");
    window.setFramerateLimit(60); // Enforcing NFR-4.1.1 (60 FPS)

    // 2. Initialize the ImGui Developer Overlay (Phase 4: Developer Tooling)
    ImGui::SFML::Init(window);

    // Delta Time clock for the render loop
    sf::Clock deltaClock;

    // 3. The Deterministic Game Loop (UC6)
    while (window.isOpen()) {
        sf::Event event;
        
        // --- Input Phase ---
        while (window.pollEvent(event)) {
            // Pass hardware events to ImGui first
            ImGui::SFML::ProcessEvent(event);

            // Allow the user to close the window
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // --- Logic & Update Phase ---
        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        // Build the ImGui UI
        ImGui::Begin("Labyrinth Engine Debug");
        ImGui::Text("Engine Status: ONLINE");
        ImGui::Text("Compiler: 64-bit UCRT GCC");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        // --- Render Phase ---
        window.clear(sf::Color(30, 30, 30)); // Clear with a dark gray background
        
        // Draw the developer overlay on top
        ImGui::SFML::Render(window); 
        
        // Push batches to GPU
        window.display(); 
    }

    // Safely shutdown ImGui to prevent memory leaks (NFR-4.2.2)
    ImGui::SFML::Shutdown();
    std::cout << "[TRACE] Phase 1: Initializing Engine Window..." << std::endl;
    sf::RenderWindow window(sf::VideoMode(800, 600), "Project Eat Meat");
    window.setFramerateLimit(60);

    std::cout << "[TRACE] Phase 2: Booting ImGui Overlay..." << std::endl;
    ImGui::SFML::Init(window);

    sf::Clock deltaClock;

    std::cout << "[TRACE] Phase 3: Entering Main Game Loop!" << std::endl;
    while (window.isOpen()) {
        // ... your event and rendering loop stays the same ...
    }

    std::cout << "[TRACE] Phase 4: Engine Shutting Down." << std::endl;
    ImGui::SFML::Shutdown();
    return 0;
}