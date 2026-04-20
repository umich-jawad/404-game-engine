#ifndef AUDIODB_H
#define AUDIODB_H

#include <filesystem>
#include <unordered_map>
#include "Utils.h"

class AudioDB {
public:
    static void init() {
        // Initialize audio system
        AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
        AudioHelper::Mix_AllocateChannels(50);
        
        // Preload all audio files from resources/audio
        if (std::filesystem::exists("resources/audio")) {
            for (const auto& entry : std::filesystem::directory_iterator("resources/audio")) {
                if (entry.is_regular_file()) {
                    std::string path = entry.path().string();
                    std::string filename = entry.path().filename().string();
                    std::string clip_name = filename.substr(0, filename.find_last_of('.'));
                    
                    Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(path.c_str());
                    if (chunk) {
                        audio_clips[clip_name] = chunk;
                    } else {
                        std::cout << "error: failed to load audio clip " << path << std::endl;
                    }
                }
            }
        }
    }

    static void Play(int channel, const std::string& clip_name, bool does_loop) {
        int loops = does_loop ? -1 : 0;
        AudioHelper::Mix_PlayChannel(channel, audio_clips[clip_name], loops);
    }

    static void Halt(int channel) {
        AudioHelper::Mix_HaltChannel(channel);
    }

    static void SetVolume(int channel, float volume) {
        int vol = static_cast<int>(volume);
        AudioHelper::Mix_Volume(channel, vol);
    }

private:
    inline static std::unordered_map<std::string, Mix_Chunk*> audio_clips;
};

#endif