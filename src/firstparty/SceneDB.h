#ifndef SCENE_DB_H
#define SCENE_DB_H

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Actor.h"
#include "Utils.h"

struct Scene {
    std::map<int, std::shared_ptr<Actor>> actors;
    std::map<int, std::shared_ptr<Actor>> actors_to_add;
    std::unordered_map<std::string, std::set<int>> actor_ids_by_name;
    std::map<int, Actor*> actors_to_start;
    std::map<int, Actor*> actors_to_update;
    std::map<int, Actor*> actors_to_late_update;
    std::vector<Actor*> updated_actors;
    std::vector<Actor*> actors_to_destroy;
};

struct SceneDB {
public:
    inline static std::unordered_map<std::string, Scene> scene_templates;
    inline static Scene current_scene;
    inline static std::string current_scene_name = "";
    inline static std::string queued_scene_name = "";
    inline static std::unordered_set<int> dont_destroy_actor_uuids;
    inline static int next_actor_id = 0;

    static void init(const std::string& initial_scene_name) {
        LoadAllScenes();
        SwitchToScene(initial_scene_name);
    }

    static void Load(const std::string& scene_name) {
        if (scene_templates.find(scene_name) == scene_templates.end()) {
            std::cout << "error: scene " << scene_name << " is missing";
            exit(0);
        }
        queued_scene_name = scene_name;
    }

    static std::string GetCurrent() {
        return current_scene_name;
    }

    static void DontDestroy(Actor* actor) {
        if (!actor || actor->destroyed) return;
        dont_destroy_actor_uuids.insert(actor->uuid);
    }

    static void ApplyQueuedSceneSwitch() {
        if (queued_scene_name.empty()) return;
        SwitchToScene(queued_scene_name);
        queued_scene_name = "";
    }

    static Actor* Find(const std::string& name) {
        auto name_it = current_scene.actor_ids_by_name.find(name);
        if (name_it == current_scene.actor_ids_by_name.end()) return nullptr;
        for (int actor_id : name_it->second) {
            Actor* actor = GetActorById(current_scene, actor_id);
            if (actor && !actor->destroyed && actor->actor_name == name) {
                return actor;
            }
        }
        return nullptr;
    }

    static luabridge::LuaRef FindAll(const std::string& name) {
        lua_State* lua_state = ComponentManager::getInstance().GetState();
        luabridge::LuaRef result_table = luabridge::LuaRef::newTable(lua_state);
        auto name_it = current_scene.actor_ids_by_name.find(name);
        if (name_it == current_scene.actor_ids_by_name.end()) {
            return result_table;
        }

        int index = 1;
        for (int actor_id : name_it->second) {
            Actor* actor = GetActorById(current_scene, actor_id);
            if (actor && !actor->destroyed && actor->actor_name == name) {
                result_table[index] = actor;
                ++index;
            }
        }
        return result_table;
    }

    static Actor* Instantiate(const std::string& template_name) {
        auto actor = std::make_shared<Actor>();
        actor->readTemplate(next_actor_id++, template_name);
        current_scene.actors_to_add[actor->uuid] = actor;
        IndexActor(current_scene, actor.get());
        return actor.get();
    }

    static void Destroy(Actor* actor) {
        if (!actor) return;
        actor->destroyed = true;
        for (auto& [key, component] : actor->components) {
            (*(component.componentRef))["enabled"] = false;
        }
        current_scene.actors_to_destroy.push_back(actor);
    }

    static void StartActors() {
        for (const auto& [uuid, actor] : current_scene.actors_to_start) {
            if (actor->destroyed) continue;
            actor->OnStart();
            if (actor->added_component) {
                current_scene.updated_actors.push_back(actor);
                actor->added_component = false;
            }
        }
        current_scene.actors_to_start.clear();
    }

    static void UpdateActors() {
        for (const auto& [uuid, actor] : current_scene.actors_to_update) {
            if (actor->destroyed) continue;
            actor->OnUpdate();
            if (actor->added_component) {
                current_scene.updated_actors.push_back(actor);
                actor->added_component = false;
            }
        }
    }

    static void LateUpdateActors() {
        for (const auto& [uuid, actor] : current_scene.actors_to_late_update) {
            if (actor->destroyed) continue;
            actor->OnLateUpdate();
            if (actor->added_component) {
                current_scene.updated_actors.push_back(actor);
                actor->added_component = false;
            }
        }
    }

    static void ProcessUpdatedActors() {
        ProcessInstantiatedActors();
        for (Actor* actor : current_scene.updated_actors) {
            actor->LateAddComponents();
            AddActorToLifecycleMaps(current_scene, actor);
        }
        current_scene.updated_actors.clear();
        for (Actor* actor : current_scene.actors_to_destroy) {
            current_scene.actors_to_start.erase(actor->uuid);
            current_scene.actors_to_update.erase(actor->uuid);
            current_scene.actors_to_late_update.erase(actor->uuid);
            dont_destroy_actor_uuids.erase(actor->uuid);

            actor->OnDestroy();

            auto active_it = current_scene.actors.find(actor->uuid);
            if (active_it != current_scene.actors.end()) {
                UnindexActor(current_scene, active_it->second.get());
                current_scene.actors.erase(active_it);
            }
        }
        current_scene.actors_to_destroy.clear();
    }

private:
    static void LoadAllScenes() {
        scene_templates.clear();
        next_actor_id = 0;

        if (!std::filesystem::exists("resources/scenes")) {
            std::cout << "error: resources/scenes missing";
            exit(0);
        }

        for (const auto& entry : std::filesystem::directory_iterator("resources/scenes")) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".scene") continue;

            const std::string scene_name = entry.path().stem().string();
            Scene template_scene;

            rapidjson::Document scene_data;
            Utils::ReadJsonFile(entry.path().string(), scene_data);
            if (scene_data.HasMember("actors") && scene_data["actors"].IsArray()) {
                const rapidjson::Value& actors_array = scene_data["actors"];
                for (rapidjson::SizeType i = 0; i < actors_array.Size(); ++i) {
                    auto actor = std::make_shared<Actor>();
                    actor->readJSON(next_actor_id++, actors_array[i]);
                    template_scene.actors[actor->uuid] = actor;
                    IndexActor(template_scene, actor.get());
                    AddActorToLifecycleMaps(template_scene, actor.get());
                }
            }

            scene_templates[scene_name] = std::move(template_scene);
        }

        if (scene_templates.empty()) {
            std::cout << "error: no scenes found in resources/scenes";
            exit(0);
        }
    }

    static Scene DeepCopySceneRuntime(const Scene& template_scene) {
        Scene new_scene;
        
        // Deep copy actors with fresh UUIDs to avoid collisions with DontDestroy actors
        for (const auto& [uuid, template_actor] : template_scene.actors) {
            auto actor_copy = std::make_shared<Actor>(*template_actor);
            actor_copy->uuid = next_actor_id++;
            // Re-point each component's Lua "actor" ref to the new copy
            for (auto& [key, component] : actor_copy->components) {
                (*(component.componentRef))["actor"] = actor_copy.get();
            }
            new_scene.actors[actor_copy->uuid] = actor_copy;
            IndexActor(new_scene, actor_copy.get());
            AddActorToLifecycleMaps(new_scene, actor_copy.get());
        }

        return new_scene;
    }

    static void SwitchToScene(const std::string& scene_name) {
        auto it = scene_templates.find(scene_name);
        if (it == scene_templates.end()) {
            std::cout << "error: scene " << scene_name << " is missing";
            exit(0);
        }

        // Create fresh scene runtime by deep copying the template
        Scene new_scene = DeepCopySceneRuntime(it->second);

        for (auto actor_it : current_scene.actors) {
            if (dont_destroy_actor_uuids.count(actor_it.second->uuid) == 0) {
                actor_it.second->OnDestroy();
            }
            else {
                std::shared_ptr<Actor> dont_destroy_actor = actor_it.second;
                new_scene.actors[dont_destroy_actor->uuid] = dont_destroy_actor;
                IndexActor(new_scene, dont_destroy_actor.get());
                AddActorToLifecycleMaps(new_scene, dont_destroy_actor.get());
            }
        }

        // Store the new scene runtime
        current_scene = std::move(new_scene);
        current_scene_name = scene_name;
    }

    static void AddActorToLifecycleMaps(Scene& scene, Actor* actor) {
        if (!actor) return;
        auto has_keys = [&](Lifecycle lc) {
            return !actor->lifecycle_keys[lc].empty();
        };
        if (has_keys(Lifecycle::OnStart)) scene.actors_to_start[actor->uuid] = actor;
        if (has_keys(Lifecycle::OnUpdate)) scene.actors_to_update[actor->uuid] = actor;
        if (has_keys(Lifecycle::OnLateUpdate)) scene.actors_to_late_update[actor->uuid] = actor;
    }

    static void ProcessInstantiatedActors() {
        for (const auto& [uuid, new_actor] : current_scene.actors_to_add) {
            current_scene.actors[uuid] = new_actor;
            if (new_actor->added_component) {
                current_scene.updated_actors.push_back(new_actor.get());
                new_actor->added_component = false;
            }
            AddActorToLifecycleMaps(current_scene, new_actor.get());
        }
        current_scene.actors_to_add.clear();
    }

    static void IndexActor(Scene& scene, Actor* actor) {
        if (!actor) return;
        scene.actor_ids_by_name[actor->actor_name].insert(actor->uuid);
    }

    static void UnindexActor(Scene& scene, Actor* actor) {
        if (!actor) return;
        auto name_it = scene.actor_ids_by_name.find(actor->actor_name);
        if (name_it == scene.actor_ids_by_name.end()) return;
        name_it->second.erase(actor->uuid);
        if (name_it->second.empty()) {
            scene.actor_ids_by_name.erase(name_it);
        }
    }

    static Actor* GetActorById(Scene& scene, int actor_id) {
        auto active_it = scene.actors.find(actor_id);
        if (active_it != scene.actors.end() && active_it->second) {
            return active_it->second.get();
        }
        auto pending_it = scene.actors_to_add.find(actor_id);
        if (pending_it != scene.actors_to_add.end() && pending_it->second) {
            return pending_it->second.get();
        }
        return nullptr;
    }
};

#endif // SCENE_DB_H
