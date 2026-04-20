#ifndef CONFIG_DB_H
#define CONFIG_DB_H

#include <string>
#include <iostream>
#include <filesystem>
#include "Utils.h"

struct ConfigDB {
public:
    inline static std::string initial_scene;
    inline static std::string game_title;

    static void init() {
        if (!std::filesystem::exists("resources")) {
            std::cout << "error: resources/ missing";
            exit(0);
        }
        rapidjson::Document config;
        if (!std::filesystem::exists("resources/game.config")) {
            std::cout << "error: resources/game.config missing";
            exit(0);
        }
        Utils::ReadJsonFile("resources/game.config", config);
        if (config.HasMember("initial_scene") && config["initial_scene"].IsString()) {
            initial_scene = config["initial_scene"].GetString();
        }
        else {
            std::cout << "error: initial_scene unspecified";
            exit(0);
        }

        if (config.HasMember("game_title") && config["game_title"].IsString()) {
            game_title = config["game_title"].GetString();
        }
    }
};

#endif // CONFIG_DB_H