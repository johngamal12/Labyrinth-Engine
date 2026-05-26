#pragma once
#include "miniaudio.h"
#include <string>
#include <unordered_map>
#include <iostream>

class SoundManager {

    private:
        ma_engine m_engine;
        bool m_intialized = false;

        // store loaded soundes by name for instant replays
        std::unordered_map<std::string, std::string> m_soundpaths;

        // track looping background sound separatly
        ma_sound m_musicsound;
        bool m_musicplaying = false;

    public:
        SoundManager();
        ~SoundManager();

        // regester a sound file by name
        void addsound(const std::string& name, const std::string& path);

        // play a one shot sound effect (play and forget)
        void playsound(const std::string& name);

        // music controls
        void playmusic(const std::string& name);
        void stopmusic();
        void setvolume(float volume); // 0.0f -> 1.0f

        bool isintialized() const {
            return m_intialized;
        }
};
