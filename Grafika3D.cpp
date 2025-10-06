// Grafika3D.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

typedef sf::Event sfe;
typedef sf::Keyboard sfk;
float R;

int main()
{
    bool running = true;
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Lab 01");

    sf::Clock deltaclock;
    ImGui::SFML::Init(window);
    
    window.setVerticalSyncEnabled(true);
    
    while (running) 
    {
        sfe event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sfe::Closed) running = false;
            if (event.type == sfe::KeyPressed && event.key.code == sfk::Escape) running = false;
        }

        ImGui::SFML::Update(window, deltaclock.restart());
        ImGui::Begin("Camera");
            ImGui::SliderFloat("R", &R, 0.5f, 10.0f);
        ImGui::End();
        ImGui::SFML::Render();

        //tu cos fajnego 

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}