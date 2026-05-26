#define MINIAUDIO_IMPLEMENTATION
#include "SoundManager.hpp"

SoundManager::SoundManager() {
    // initialze miniaudio engine
    ma_result result = ma_engine_init(nullptr, &m_engine);
    if(result != MA_SUCCESS){
        std::cerr << "soundmanager error: failed to inilaize \n";
        m_intialized = false;
        return;
    }

    m_intialized = true;
    std::cout << "soundmanager initialized \n";
}

SoundManager::~SoundManager() {
    if(m_musicplaying){
        ma_sound_uninit(&m_musicsound);
    }
    if(m_intialized){
        ma_engine_uninit(&m_engine);
    }

}

void SoundManager::addsound(const std::string & name, const std::string& path){
    if(!m_intialized) return;

    // miniaudio loads from disk on play so we just register the path
    // sounds dont need to sit in ram
    m_soundpaths[name] = path;
    std::cout << "sound registerd" << name << "from path ->" << path << "\n";
    
}

void SoundManager::playsound(const std::string& name){
    if(!m_intialized) return;

    auto it = m_soundpaths.find(name);
    if (it == m_soundpaths.end()){
        std::cerr << "soundmanager warning (sound)" << name << "not found \n";
        return;
    }

    // play and forget (miniaudio manages the lifetime internally)
    ma_engine_play_sound(&m_engine, it->second.c_str(), nullptr);
}

void SoundManager::playmusic(const std::string& name){
    if(!m_intialized) return;

    auto it = m_soundpaths.find(name);
    if(it == m_soundpaths.end()){
        std::cerr << "soundmanager warning (music)" << name << " not found \n";
        return;
    }

    // stop prevoius music if playing
    if(m_musicplaying){
        ma_sound_stop(&m_musicsound);
        ma_sound_uninit(&m_musicsound);
        m_musicplaying = false;
    }

    // intialize a presistent looping sound
    ma_result result = ma_sound_init_from_file(
        &m_engine,
        it->second.c_str(),
        MA_SOUND_FLAG_STREAM, // stream from disk (good for long music files)
        nullptr,
        nullptr,
        &m_musicsound
    );

    if(result != MA_SUCCESS){
        std::cerr << "soundmanager error failed to load music" << name <<"\n";
        return;
    }

    ma_sound_set_looping(&m_musicsound, MA_TRUE);
    ma_sound_start(&m_musicsound);
    m_musicplaying = true;

    std::cout<< "music " << name <<" playing \n";
}

void SoundManager::stopmusic(){
    if(!m_intialized || !m_musicplaying) return;

    ma_sound_stop(&m_musicsound);
    ma_sound_uninit(&m_musicsound);
    m_musicplaying = false;
}

void SoundManager::setvolume(float volume){
    if(!m_intialized) return;
    // clamp between 0 and 1
    volume = std::max(0.0f, std::min(1.0f, volume));
    ma_engine_set_volume(&m_engine, volume);
}
