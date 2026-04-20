#ifndef ACTOR_H
#define ACTOR_H

#include <map>
#include <string>
#include <vector>
#include <array>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "Utils.h"
#include "ComponentManager.h"

class TemplateDB;

class Actor {
public:
    // Basic properties
    int uuid = -1;
    std::string actor_name = "";
    std::map<std::string, Component> components;
    std::array<std::set<std::string>, LIFECYCLE_COUNT> lifecycle_keys;
    std::unordered_set<std::string> keys_to_add;
    std::vector<Component> comps_to_remove;
    bool destroyed = false;
    bool added_component = false;
    static inline int ComponentsAdded = 0;

    Actor() {}

    void readJSON(int uuid, const rapidjson::Value& actor_json);
    void readTemplate(int uuid, const std::string& template_name);

    bool operator==(const Actor& other) const {
        return uuid == other.uuid;
    }

    void OnStart() { RunLifecycle(Lifecycle::OnStart); lifecycle_keys[Lifecycle::OnStart].clear(); }
    void OnUpdate() { RunLifecycle(Lifecycle::OnUpdate); }
    void OnLateUpdate() { RunLifecycle(Lifecycle::OnLateUpdate); }
    void OnCollisionEnter(Collision collision) { RunLifecycle(Lifecycle::OnCollisionEnter, &collision); }
    void OnCollisionExit(Collision collision) { RunLifecycle(Lifecycle::OnCollisionExit, &collision); }
    void OnTriggerEnter(Collision collision) { RunLifecycle(Lifecycle::OnTriggerEnter, &collision); }
    void OnTriggerExit(Collision collision) { RunLifecycle(Lifecycle::OnTriggerExit, &collision); }
    void OnDestroy() { RunLifecycle(Lifecycle::OnDestroy); }

    void RunLifecycle(Lifecycle lc, Collision* collision = nullptr) {
        auto& keys = lifecycle_keys[lc];
        for (const std::string& key : keys) {
            auto it = components.find(key);
            if (it != components.end()) {
                TryRunComponentFunction(it->second, lc, collision);
            }
        }
    }

    void TryRunComponentFunction(const Component& component, Lifecycle lc, Collision* collision = nullptr) {
        if (!component.isEnabled() && lc != Lifecycle::OnDestroy) return;
        try {
            const char* function_name = LIFECYCLE_NAMES[lc].c_str();
            if (collision) {
                (*component.componentRef)[function_name](*component.componentRef, *collision);
            }
            else {
                (*component.componentRef)[function_name](*component.componentRef);
            }
        }
        catch (const luabridge::LuaException& e) {
            ComponentManager::ReportError(actor_name, e);
        }
    }

    void LateAddComponents() {
        for (const std::string& key : keys_to_add) {
            auto comp_it = components.find(key);
            if (comp_it != components.end()) {
                RegisterComponentLifecycle(key, comp_it->second);
            }
        }
        keys_to_add.clear();
        // run on destroy for removed components
        for (const auto& comp : comps_to_remove) {
            if (comp.hasLifecycle(Lifecycle::OnDestroy)) {
                TryRunComponentFunction(comp, Lifecycle::OnDestroy);
            }
        }
        comps_to_remove.clear();
    }

    std::string GetName() const {
        return actor_name;
    }

    int GetID() const {
        return uuid;
    }

    luabridge::LuaRef GetComponentByKey(const std::string& key) const {
        auto it = components.find(key);
        if (it != components.end()) {
            return *(it->second.componentRef);
        }
        return luabridge::LuaRef(ComponentManager::getInstance().GetState());
    }

    luabridge::LuaRef GetComponent(const std::string& type_name) const {
        for (const auto& entry : components) {
            if (entry.second.type == type_name) {
                return *(entry.second.componentRef);
            }
        }
        return luabridge::LuaRef(ComponentManager::getInstance().GetState());
    }

    luabridge::LuaRef GetComponents(const std::string& type_name) const {
        lua_State* lua_state = ComponentManager::getInstance().GetState();
        luabridge::LuaRef result_table = luabridge::LuaRef::newTable(lua_state);
        int index = 1;
        for (const auto& entry : components) {
            if (entry.second.type == type_name) {
                result_table[index] = *(entry.second.componentRef);
                ++index;
            }
        }

        return result_table;
    }

    luabridge::LuaRef AddComponent(const std::string& type_name) {
        std::string key = "r" + std::to_string(ComponentsAdded);
        added_component = true;
        ComponentsAdded++;

        // Create component with generated key
        auto emplace_result = components.emplace(key, Component(key, type_name));
        auto component_it = emplace_result.first;
        
        // Set actor reference on component
        (*(component_it->second.componentRef))["actor"] = this;
        
        keys_to_add.insert(key);

        return *(component_it->second.componentRef);
    }

    void RemoveComponent(luabridge::LuaRef component_ref) {
        for (auto it = components.begin(); it != components.end(); ++it) {
            if (it->second.componentRef && *(it->second.componentRef) == component_ref) {
                added_component = true; // Mark as changed to trigger lifecycle re-checks
                comps_to_remove.push_back(it->second);
                it->second.setEnabled(false);
                for (auto& keys : lifecycle_keys) {
                    keys.erase(it->first);
                }
                components.erase(it);
                break;
            }
        }
    }

    std::string toString() const {
        return "Actor(uuid: " + std::to_string(uuid) + ", name: " + actor_name + ")";
    } //

    void RegisterComponentLifecycle(const std::string& key, const Component& component) {
        for (int i = 0; i < LIFECYCLE_COUNT; ++i) {
            if (component.lifecycle_functions[i]) {
                lifecycle_keys[i].insert(key);
            }
        }
    }

    void AddComponentFromJSON(const std::string& key, const rapidjson::Value& component_json) {
        auto component_it = components.find(key);
        if (component_it != components.end()) {
            // Key exists — recreate component with same type (template override)
            std::string component_type = component_it->second.type;
            component_it->second = Component(key, component_type);
        }
        else {
            // New component — type must be specified in JSON
            std::string component_type = component_json["type"].GetString();
            auto emplace_result = components.emplace(key, Component(key, component_type));
            component_it = emplace_result.first;
        }
        component_it->second.PopulateFromJSON(component_json);
        (*(component_it->second.componentRef))["actor"] = this;
        RegisterComponentLifecycle(key, component_it->second);
    }
};

#include "TemplateDB.h"

inline void Actor::readTemplate(int uuid, const std::string& template_name) {
    this->uuid = uuid;
    auto template_it = TemplateDB::getInstance().templates.find(template_name);
    if (template_it != TemplateDB::getInstance().templates.end()) {
        const Actor& template_actor = template_it->second;
        *this = template_actor;
        this->uuid = uuid;
        // Re-point cloned components' Lua "actor" ref to this actor
        for (auto& [key, component] : components) {
            (*(component.componentRef))["actor"] = this;
        }
    }
    else {
        std::cout << "error: template " << template_name << " is missing";
        exit(0);
    }
}

inline void Actor::readJSON(int uuid, const rapidjson::Value& actor_json) {
    this->uuid = uuid;
    if (actor_json.HasMember("template") && actor_json["template"].IsString()) {
        std::string template_name = actor_json["template"].GetString();
        readTemplate(uuid, template_name);
    }
    if (actor_json.HasMember("name") && actor_json["name"].IsString()) {
        actor_name = actor_json["name"].GetString();
    }
    if (actor_json.HasMember("components") && actor_json["components"].IsObject()) {
        for (auto& component_entry : actor_json["components"].GetObject()) {
            std::string component_key = component_entry.name.GetString();
            const rapidjson::Value& component_value = component_entry.value;
            // Allow template-derived overrides that omit "type" when the component key already exists.
            if ((component_value.HasMember("type") && component_value["type"].IsString()) ||
                components.find(component_key) != components.end()) {
                AddComponentFromJSON(component_key, component_value);
            }
        }
    }
}

inline void ContactListener::ProcessContact(b2Contact* contact, bool is_begin) {
    auto fixtureA = contact->GetFixtureA();
    auto fixtureB = contact->GetFixtureB();
    Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetUserData().pointer);
    Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetUserData().pointer);
    if (!actorA || !actorB) return;
    if (fixtureA->IsSensor() != fixtureB->IsSensor()) return; // Ignore collisions between a sensor and a non-sensor
    bool isSensor = fixtureA->IsSensor(); // Both fixtures will have the same sensor status due to the above check

    Collision collision;
    if (isSensor || !is_begin) {
        collision.point = b2Vec2(-999.0f, -999.0f);
        collision.normal = b2Vec2(-999.0f, -999.0f);
    } else {
        b2WorldManifold worldManifold;
        contact->GetWorldManifold(&worldManifold);
        collision.point = worldManifold.points[0];
        collision.normal = worldManifold.normal;
    }

    collision.other = actorB;
    collision.relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
    if (is_begin) {
        if (isSensor) actorA->OnTriggerEnter(collision);
        else actorA->OnCollisionEnter(collision);
    } else {
        if (isSensor) actorA->OnTriggerExit(collision);
        else actorA->OnCollisionExit(collision);
    }

    collision.other = actorA;
    if (is_begin) {
        if (isSensor) actorB->OnTriggerEnter(collision);
        else actorB->OnCollisionEnter(collision);
    } else {
        if (isSensor) actorB->OnTriggerExit(collision);
        else actorB->OnCollisionExit(collision);
    }
}

#endif // ACTOR_H