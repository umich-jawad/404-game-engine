#ifndef RAYCASTMANAGER_H
#define RAYCASTMANAGER_H

#include <algorithm>
#include <vector>
#include "box2d/box2d.h"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "Rigidbody.h"

class HitResult {
public:
    Actor* actor = nullptr;
    b2Vec2 point;
    b2Vec2 normal;
    bool is_trigger = false;
    float fraction = 0.0f;
};

class ClosestRaycastCallback : public b2RayCastCallback {
public:
    HitResult closestHit;
    bool hasHit = false;
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
        if (!actor) return -1;
        closestHit.actor = actor;
        closestHit.point = point;
        closestHit.normal = normal;
        closestHit.is_trigger = fixture->IsSensor();
        hasHit = true;
        return fraction;
    }
};

class AllRaycastCallback : public b2RayCastCallback {
public:
    std::vector<HitResult> hits;
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
        if (!actor) return -1;
        HitResult hit;
        hit.actor = actor;
        hit.point = point;
        hit.normal = normal;
        hit.is_trigger = fixture->IsSensor();
        hit.fraction = fraction;
        hits.push_back(hit);
        return 1;
    }
};

class RaycastManager {
public:
    static inline lua_State* lua_state = nullptr;

    static luabridge::LuaRef Raycast(b2Vec2 pos, b2Vec2 dir, float dist) {
        if (dist <= 0 || !Rigidbody::physics_world) return luabridge::LuaRef(lua_state);

        dir.Normalize();
        b2Vec2 endPoint = pos + dist * dir;

        ClosestRaycastCallback callback;
        Rigidbody::physics_world->RayCast(&callback, pos, endPoint);

        if (!callback.hasHit) return luabridge::LuaRef(lua_state);

        return luabridge::LuaRef(lua_state, callback.closestHit);
    }

    static luabridge::LuaRef RaycastAll(b2Vec2 pos, b2Vec2 dir, float dist) {
        luabridge::LuaRef result = luabridge::LuaRef::newTable(lua_state);
        if (dist <= 0 || !Rigidbody::physics_world) return result;

        dir.Normalize();
        b2Vec2 endPoint = pos + dist * dir;

        AllRaycastCallback callback;
        Rigidbody::physics_world->RayCast(&callback, pos, endPoint);

        std::sort(callback.hits.begin(), callback.hits.end(),
            [](const HitResult& a, const HitResult& b) { return a.fraction < b.fraction; });

        for (size_t i = 0; i < callback.hits.size(); ++i) {
            result[static_cast<int>(i + 1)] = callback.hits[i];
        }
        return result;
    }

};

#endif // RAYCASTMANAGER_H