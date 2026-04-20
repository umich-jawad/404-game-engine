#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include "Helper.h"
#include "Utils.h"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "box2d/box2d.h"
#include "Rigidbody.h"
#include "ParticleSystem.h"

enum Lifecycle {
    OnStart = 0,
    OnUpdate,
    OnLateUpdate,
    OnCollisionEnter,
    OnCollisionExit,
    OnTriggerEnter,
    OnTriggerExit,
    OnDestroy,
    LIFECYCLE_COUNT
};

static const inline std::string LIFECYCLE_NAMES[LIFECYCLE_COUNT] = {
    "OnStart", "OnUpdate", "OnLateUpdate",
    "OnCollisionEnter", "OnCollisionExit",
    "OnTriggerEnter", "OnTriggerExit", "OnDestroy"
};

class ComponentType {
public:
    std::string type_name;
    std::shared_ptr<luabridge::LuaRef> lua_table_ref;
    bool lifecycle_functions[LIFECYCLE_COUNT] = {};

    bool hasLifecycle(Lifecycle lc) const {
        return lifecycle_functions[lc];
    }
};

class ComponentManager {
public:
    static ComponentManager& getInstance() {
        static ComponentManager instance;
        return instance;
    }

    ComponentManager(const ComponentManager&) = delete;
    ComponentManager& operator=(const ComponentManager&) = delete;

    static void Print(const std::string& message) {
        std::cout << message << std::endl;
    }

    static void PrintError(const std::string& message) {
        std::cout << message << std::endl;
    }

    static void Quit() {
        exit(0);
    }

    static void Sleep(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    static int GetFrame() {
        return Helper::GetFrameNumber();
    }

    static void OpenURL(const std::string& url) {
        std::string command;
#ifdef _WIN32
        command = "start \"\" \"" + url + "\"";
#elif __APPLE__
        command = "open \"" + url + "\"";
#else
        command = "xdg-open \"" + url + "\"";
#endif
        std::system(command.c_str());
    }

    static void ReportError(const std::string& actor_name, const luabridge::LuaException& e)
    {
        std::string error_message = e.what();

        /* Normalize file paths across platforms */
        std::replace(error_message.begin(), error_message.end(), '\\', '/');

        /* Display (with color codes) */
        std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
    }

    lua_State* GetState() const {
        return lua_state;
    }

    void EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table) {
        luabridge::LuaRef new_metatable = luabridge::LuaRef::newTable(lua_state);
        new_metatable["__index"] = parent_table;

        instance_table.push(lua_state);
        new_metatable.push(lua_state);
        lua_setmetatable(lua_state, -2);
        lua_pop(lua_state, 1);
    }

    ComponentType GetComponentType(const std::string& type) {
        auto it = component_tables.find(type);
        if (it != component_tables.end()) {
            return it->second;
        }
        return ComponentType(); // Return an empty ComponentType if not found
    }

    std::shared_ptr<luabridge::LuaRef> CreateComponentInstance(const std::string& type, const std::string& key) {
        // C++ component types
        if (cpp_component_types.count(type)) {
            if (type == "Rigidbody") {
                Rigidbody* rb = new Rigidbody();
                rb->key = key;
                rb->type = type;
                return std::make_shared<luabridge::LuaRef>(lua_state, rb);
            }
            if (type == "ParticleSystem") {
                ParticleSystem* ps = new ParticleSystem();
                ps->key = key;
                ps->type = type;
                return std::make_shared<luabridge::LuaRef>(lua_state, ps);
            }
        }

        // Lua component types
        auto type_it = component_tables.find(type);
        if (type_it == component_tables.end() || !(type_it->second.lua_table_ref) || type_it->second.lua_table_ref->isNil()) {
            std::cout << "error: failed to locate component " << type;
            exit(0);
        }

        luabridge::LuaRef instance = luabridge::LuaRef::newTable(lua_state);
        instance["key"] = key;
        EstablishInheritance(instance, *(type_it->second.lua_table_ref));
        return std::make_shared<luabridge::LuaRef>(instance);
    }

    void CloneComponentInstance(const std::shared_ptr<luabridge::LuaRef>& source_ref, std::shared_ptr<luabridge::LuaRef>& dest_ref, const std::string& type) {
        if (cpp_component_types.count(type)) {
            if (type == "Rigidbody") {
                Rigidbody* source_rb = (*source_ref).cast<Rigidbody*>();
                Rigidbody* new_rb = new Rigidbody(*source_rb);
                dest_ref = std::make_shared<luabridge::LuaRef>(lua_state, new_rb);
                return;
            }
            if (type == "ParticleSystem") {
                ParticleSystem* source_ps = (*source_ref).cast<ParticleSystem*>();
                ParticleSystem* new_ps = new ParticleSystem(*source_ps);
                dest_ref = std::make_shared<luabridge::LuaRef>(lua_state, new_ps);
                return;
            }
        }
        luabridge::LuaRef new_instance = luabridge::LuaRef::newTable(lua_state);
        EstablishInheritance(new_instance, *source_ref);
        dest_ref = std::make_shared<luabridge::LuaRef>(new_instance);
    }

    bool IsCppComponent(const std::string& type) const {
        return cpp_component_types.count(type) > 0;
    }

private:
    lua_State* lua_state;
    std::unordered_map<std::string, ComponentType> component_tables;
    std::unordered_set<std::string> cpp_component_types;

    ComponentManager() {
        InitializeState();
        RegisterCppComponents();
        InitializeComponentTypes();
    }

    void InitializeState() {
        lua_state = luaL_newstate();
        luaL_openlibs(lua_state);
    }

    void InitializeComponentTypes() {
        // Register component types and their properties to Lua here
        std::filesystem::path components_path = "resources/component_types/";
        if (std::filesystem::exists(components_path) && std::filesystem::is_directory(components_path)) {
            for (const auto& entry : std::filesystem::directory_iterator(components_path)) {
                if (entry.path().extension() == ".lua") {
                    if (luaL_dofile(lua_state, entry.path().string().c_str()) != LUA_OK) {
                        std::cout << "problem with lua file " << entry.path().stem().string();
                        exit(0); // Exit if there's an error loading any Lua file
                    }
                    InitializeComponentType(entry.path().stem().string().c_str());
                }
            }
        }
    }

    void InitializeComponentType(const std::string& component_name) {
        luabridge::LuaRef component_table = luabridge::getGlobal(lua_state, component_name.c_str());
        if (!component_table.isTable()) {
            std::cout << "error: component " << component_name << " did not return a table";
            exit(0);
        }

        ComponentType component_type;
        component_type.type_name = component_name;
        component_type.lua_table_ref = std::make_shared<luabridge::LuaRef>(component_table);
        for (int i = 0; i < LIFECYCLE_COUNT; ++i) {
            if (component_table[LIFECYCLE_NAMES[i]].isFunction()) {
                component_type.lifecycle_functions[i] = true;
            }
        }

        component_tables[component_name] = component_type;
    }

    void RegisterCppComponents() {
        cpp_component_types.insert("Rigidbody");

        ComponentType rb_type;
        rb_type.type_name = "Rigidbody";
        rb_type.lifecycle_functions[Lifecycle::OnStart] = true;
        rb_type.lifecycle_functions[Lifecycle::OnDestroy] = true;
        component_tables["Rigidbody"] = rb_type;

        cpp_component_types.insert("ParticleSystem");

        ComponentType ps_type;
        ps_type.type_name = "ParticleSystem";
        ps_type.lifecycle_functions[Lifecycle::OnStart] = true;
        ps_type.lifecycle_functions[Lifecycle::OnUpdate] = true;
        component_tables["ParticleSystem"] = ps_type;
    }
};

class Component {
public:
    explicit Component(const std::string& key, const std::string type) : type(type), key(key) {
        componentRef = ComponentManager::getInstance().CreateComponentInstance(type, key);
        if (!componentRef) {
            std::cout << "error: failed to locate component " << type;
            exit(0);
        }
        const ComponentType& component_type = ComponentManager::getInstance().GetComponentType(type);
        std::memcpy(lifecycle_functions, component_type.lifecycle_functions, sizeof(lifecycle_functions));
        (*componentRef)["enabled"] = true;
    }


    explicit Component(const Component& other) : type(other.type), key(other.key) {
        ComponentManager::getInstance().CloneComponentInstance(other.componentRef, componentRef, type);
        std::memcpy(lifecycle_functions, other.lifecycle_functions, sizeof(lifecycle_functions));
        (*componentRef)["enabled"] = true;
    }

    void PopulateFromJSON(const rapidjson::Value& component_json) {
        for (auto it = component_json.MemberBegin(); it != component_json.MemberEnd(); ++it) {
            std::string property_name = it->name.GetString();
            const rapidjson::Value& property_value = it->value;

            if (property_name == "type") continue; // Skip the "type" field since it's already used to determine the component type
            if (property_value.IsInt()) {
                (*componentRef)[property_name.c_str()] = property_value.GetInt();
            }
            else if (property_value.IsFloat()) {
                (*componentRef)[property_name.c_str()] = property_value.GetFloat();
            }
            else if (property_value.IsBool()) {
                (*componentRef)[property_name.c_str()] = property_value.GetBool();
            }
            else if (property_value.IsString()) {
                (*componentRef)[property_name.c_str()] = property_value.GetString();
            }
        }
    }

    std::shared_ptr<luabridge::LuaRef> componentRef;
    std::string type;
    std::string key;

    bool isEnabled() const {
        return (*componentRef)["enabled"].cast<bool>();
    }

    void setEnabled(bool enabled) {
        (*componentRef)["enabled"] = enabled;
    }
    bool lifecycle_functions[LIFECYCLE_COUNT] = {};

    bool hasLifecycle(Lifecycle lc) const {
        return lifecycle_functions[lc];
    }
};

#endif // COMPONENTMANAGER_H