#ifndef TEMPLATEDB_H
#define TEMPLATEDB_H

#include <unordered_map>
#include <filesystem>
#include "Utils.h"

class Actor;

class TemplateDB {
public:
    std::unordered_map<std::string, Actor> templates;

    static TemplateDB& getInstance() {
        static TemplateDB instance;
        return instance;
    }

    TemplateDB(const TemplateDB&) = delete;
    TemplateDB& operator=(const TemplateDB&) = delete;

private:
    TemplateDB() {
        if (!std::filesystem::exists("resources/actor_templates")) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator("resources/actor_templates")) {
            if (entry.path().extension() == ".template") {
                rapidjson::Document template_data;
                Utils::ReadJsonFile(entry.path().string(), template_data);
                Actor actor;
                actor.readJSON(-1, template_data);
                templates.emplace(entry.path().stem().string(), std::move(actor));
            }
        }
    }
};

#include "Actor.h"

#endif // TEMPLATEDB_H