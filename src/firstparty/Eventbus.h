#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

using Subscriber = std::pair<luabridge::LuaRef, luabridge::LuaRef>; // (component, function)

class EventBus {
public:
    static inline std::unordered_map<std::string, std::vector<Subscriber>> subscribers;
    static inline std::vector<std::pair<std::string, Subscriber>> pending_subscribes;
    static inline std::vector<std::pair<std::string, Subscriber>> pending_unsubscribes;

    static void Publish(const std::string& event_type, luabridge::LuaRef event_object) {
        auto it = subscribers.find(event_type);
        if (it == subscribers.end()) return;
        for (auto& [component, function] : it->second) {
            function(component, event_object);
        }
    }

    static void Subscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
        pending_subscribes.emplace_back(event_type, Subscriber(component, function));
    }

    static void Unsubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
        pending_unsubscribes.emplace_back(event_type, Subscriber(component, function));
    }

    static void ProcessPending() {
        for (auto& [event_type, sub] : pending_subscribes) {
            subscribers[event_type].push_back(sub);
        }
        pending_subscribes.clear();

        for (auto& [event_type, unsub] : pending_unsubscribes) {
            auto it = subscribers.find(event_type);
            if (it == subscribers.end()) continue;
            auto& vec = it->second;
            for (auto vit = vec.begin(); vit != vec.end(); ++vit) {
                if (vit->first == unsub.first && vit->second == unsub.second) {
                    vec.erase(vit);
                    break;
                }
            }
        }
        pending_unsubscribes.clear();
    }

};

#endif // EVENTBUS_H