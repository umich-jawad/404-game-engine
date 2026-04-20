#ifndef LUA_INJECTIONS_H
#define LUA_INJECTIONS_H

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "box2d/box2d.h"
#include "glm/glm.hpp"
#include "ComponentManager.h"
#include "Rigidbody.h"
#include "ParticleSystem.h"
#include "Actor.h"
#include "SceneDB.h"
#include "Renderer.h"
#include "ImageDB.h"
#include "TextDB.h"
#include "AudioDB.h"
#include "Input.h"
#include "CInput.h"
#include "RaycastManager.h"
#include "Eventbus.h"

class LuaInjections {
public:
    static void InjectAll(lua_State* lua_state) {
        // Core types: vec2, Vector2, Debug, Application
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<glm::vec2>("vec2")
            .addProperty("x", &glm::vec2::x)
            .addProperty("y", &glm::vec2::y)
            .endClass()
            .beginNamespace("Debug")
            .addFunction("Log", ComponentManager::Print)
            .addFunction("LogError", ComponentManager::PrintError)
            .endNamespace()
            .beginNamespace("Application")
            .addFunction("Quit", ComponentManager::Quit)
            .addFunction("Sleep", ComponentManager::Sleep)
            .addFunction("GetFrame", ComponentManager::GetFrame)
            .addFunction("OpenURL", ComponentManager::OpenURL)
            .endNamespace()
            .beginClass<b2Vec2>("Vector2")
            .addConstructor<void(*)(float, float)>()
            .addProperty("x", &b2Vec2::x)
            .addProperty("y", &b2Vec2::y)
            .addFunction("Normalize", &b2Vec2::Normalize)
            .addFunction("Length", &b2Vec2::Length)
            .addFunction("__add", &b2Vec2::operator_add)
            .addFunction("__sub", &b2Vec2::operator_sub)
            .addFunction("__mul", &b2Vec2::operator_mul)
            .endClass()
            .beginNamespace("Vector2")
            .addFunction("Distance", b2Distance)
            .addFunction("Dot", static_cast<float(*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
            .endNamespace();

        // Rigidbody + Collision
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<Collision>("Collision")
            .addProperty("other", &Collision::other)
            .addProperty("point", &Collision::point)
            .addProperty("relative_velocity", &Collision::relative_velocity)
            .addProperty("normal", &Collision::normal)
            .endClass()
            .beginClass<Rigidbody>("Rigidbody")
            .addConstructor<void(*)(void)>()
            .addProperty("x", &Rigidbody::x)
            .addProperty("y", &Rigidbody::y)
            .addProperty("body_type", &Rigidbody::body_type)
            .addProperty("precise", &Rigidbody::precise)
            .addProperty("gravity_scale", &Rigidbody::gravity_scale)
            .addProperty("density", &Rigidbody::density)
            .addProperty("angular_friction", &Rigidbody::angular_friction)
            .addProperty("rotation", &Rigidbody::rotation)
            .addProperty("has_collider", &Rigidbody::has_collider)
            .addProperty("collider_type", &Rigidbody::collider_type)
            .addProperty("width", &Rigidbody::width)
            .addProperty("height", &Rigidbody::height)
            .addProperty("radius", &Rigidbody::radius)
            .addProperty("friction", &Rigidbody::friction)
            .addProperty("bounciness", &Rigidbody::bounciness)
            .addProperty("has_trigger", &Rigidbody::has_trigger)
            .addProperty("trigger_type", &Rigidbody::trigger_type)
            .addProperty("trigger_width", &Rigidbody::trigger_width)
            .addProperty("trigger_height", &Rigidbody::trigger_height)
            .addProperty("trigger_radius", &Rigidbody::trigger_radius)
            .addProperty("enabled", &Rigidbody::enabled)
            .addProperty("key", &Rigidbody::key)
            .addProperty("type", &Rigidbody::type)
            .addProperty("actor", &Rigidbody::actor)
            .addFunction("GetPosition", &Rigidbody::GetPosition)
            .addFunction("GetRotation", &Rigidbody::GetRotation)
            .addFunction("GetVelocity", &Rigidbody::GetVelocity)
            .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
            .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
            .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
            .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
            .addFunction("AddForce", &Rigidbody::AddForce)
            .addFunction("SetVelocity", &Rigidbody::SetVelocity)
            .addFunction("SetPosition", &Rigidbody::SetPosition)
            .addFunction("SetRotation", &Rigidbody::SetRotation)
            .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
            .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
            .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
            .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
            .addFunction("OnStart", &Rigidbody::OnStart)
            .addFunction("OnDestroy", &Rigidbody::OnDestroy)
            .endClass();

        // ParticleSystem
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<ParticleSystem>("ParticleSystem")
            .addConstructor<void(*)(void)>()
            .addFunction("OnStart", &ParticleSystem::OnStart)
            .addFunction("OnUpdate", &ParticleSystem::OnUpdate)
            .addFunction("Burst", &ParticleSystem::Burst)
            .addFunction("Stop", &ParticleSystem::Stop)
            .addFunction("Play", &ParticleSystem::Play)
            .addProperty("x", &ParticleSystem::x_offset)
            .addProperty("y", &ParticleSystem::y_offset)
            .addProperty("frames_between_bursts", &ParticleSystem::frames_between_bursts)
            .addProperty("burst_quantity", &ParticleSystem::burst_quantity)
            .addProperty("start_color_r", &ParticleSystem::start_color_r)
            .addProperty("start_color_g", &ParticleSystem::start_color_g)
            .addProperty("start_color_b", &ParticleSystem::start_color_b)
            .addProperty("start_color_a", &ParticleSystem::start_color_a)
            .addProperty("image", &ParticleSystem::image)
            .addProperty("sorting_order", &ParticleSystem::sorting_order)
            .addProperty("duration_frames", &ParticleSystem::duration_frames)
            .addProperty("gravity_scale_x", &ParticleSystem::gravity_scale_x)
            .addProperty("gravity_scale_y", &ParticleSystem::gravity_scale_y)
            .addProperty("drag_factor", &ParticleSystem::drag_factor)
            .addProperty("angular_drag_factor", &ParticleSystem::angular_drag_factor)
            .addProperty("end_scale", &ParticleSystem::end_scale)
            .addProperty("end_color_r", &ParticleSystem::end_color_r)
            .addProperty("end_color_g", &ParticleSystem::end_color_g)
            .addProperty("end_color_b", &ParticleSystem::end_color_b)
            .addProperty("end_color_a", &ParticleSystem::end_color_a)
            .addProperty("emit_angle_min", &ParticleSystem::emit_angle_min)
            .addProperty("emit_angle_max", &ParticleSystem::emit_angle_max)
            .addProperty("emit_radius_min", &ParticleSystem::emit_radius_min)
            .addProperty("emit_radius_max", &ParticleSystem::emit_radius_max)
            .addProperty("start_speed_min", &ParticleSystem::start_speed_min)
            .addProperty("start_speed_max", &ParticleSystem::start_speed_max)
            .addProperty("rotation_min", &ParticleSystem::rotation_min)
            .addProperty("rotation_max", &ParticleSystem::rotation_max)
            .addProperty("rotation_speed_min", &ParticleSystem::rotation_speed_min)
            .addProperty("rotation_speed_max", &ParticleSystem::rotation_speed_max)
            .addProperty("start_scale_min", &ParticleSystem::start_scale_min)
            .addProperty("start_scale_max", &ParticleSystem::start_scale_max)
            .addProperty("enabled", &ParticleSystem::enabled)
            .addProperty("key", &ParticleSystem::key)
            .addProperty("type", &ParticleSystem::type)
            .addProperty("actor", &ParticleSystem::actor)
            .endClass();

        // Actor class + Actor namespace + Scene namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<Actor>("Actor")
            .addFunction("GetName", &Actor::GetName)
            .addFunction("GetID", &Actor::GetID)
            .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
            .addFunction("GetComponent", &Actor::GetComponent)
            .addFunction("GetComponents", &Actor::GetComponents)
            .addFunction("AddComponent", &Actor::AddComponent)
            .addFunction("RemoveComponent", &Actor::RemoveComponent)
            .addFunction("OnCollisionEnter", &Actor::OnCollisionEnter)
            .addFunction("OnCollisionExit", &Actor::OnCollisionExit)
            .addFunction("OnTriggerEnter", &Actor::OnTriggerEnter)
            .addFunction("OnTriggerExit", &Actor::OnTriggerExit)
            .addFunction("OnDestroy", &Actor::OnDestroy)
            .endClass()
            .beginNamespace("Actor")
            .addFunction("Find", &SceneDB::Find)
            .addFunction("FindAll", &SceneDB::FindAll)
            .addFunction("Instantiate", &SceneDB::Instantiate)
            .addFunction("Destroy", &SceneDB::Destroy)
            .endNamespace()
            .beginNamespace("Scene")
            .addFunction("Load", &SceneDB::Load)
            .addFunction("GetCurrent", &SceneDB::GetCurrent)
            .addFunction("DontDestroy", &SceneDB::DontDestroy)
            .endNamespace();

        // Camera namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Camera")
            .addFunction("SetPosition", &EngineRenderer::CameraSetPosition)
            .addFunction("GetPositionX", &EngineRenderer::CameraGetPositionX)
            .addFunction("GetPositionY", &EngineRenderer::CameraGetPositionY)
            .addFunction("SetZoom", &EngineRenderer::CameraSetZoom)
            .addFunction("GetZoom", &EngineRenderer::CameraGetZoom)
            .endNamespace();

        // Image namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Image")
            .addFunction("Draw", static_cast<void(*)(const std::string&, float, float)>(&ImageDB::Draw))
            .addFunction("DrawEx", static_cast<void(*)(const std::string&, float, float, float, float, float, float, float, float, float, float, float, float)>(&ImageDB::DrawEx))
            .addFunction("DrawUI", static_cast<void(*)(const std::string&, float, float)>(&ImageDB::DrawUI))
            .addFunction("DrawUIEx", static_cast<void(*)(const std::string&, float, float, float, float, float, float, float)>(&ImageDB::DrawUIEx))
            .addFunction("DrawPixel", static_cast<void(*)(float, float, float, float, float, float)>(&ImageDB::DrawPixel))
            .endNamespace();

        // Text namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Text")
            .addFunction("Draw", &TextDB::Draw)
            .endNamespace();

        // Audio namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Audio")
            .addFunction("Play", &AudioDB::Play)
            .addFunction("Halt", &AudioDB::Halt)
            .addFunction("SetVolume", &AudioDB::SetVolume)
            .endNamespace();

        // Input namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Input")
            .addFunction("GetKey", &Input::GetKey)
            .addFunction("GetKeyDown", &Input::GetKeyDown)
            .addFunction("GetKeyUp", &Input::GetKeyUp)
            .addFunction("GetMousePosition", &Input::GetMousePosition)
            .addFunction("GetMouseButton", &Input::GetMouseButton)
            .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
            .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
            .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
            .addFunction("HideCursor", &Input::HideCursor)
            .addFunction("ShowCursor", &Input::ShowCursor)
            .addFunction("GetButton", &CInput::GetButton)
            .addFunction("GetButtonDown", &CInput::GetButtonDown)
            .addFunction("GetButtonUp", &CInput::GetButtonUp)
            .addFunction("GetAxis", &CInput::GetAxis)
            .addFunction("IsControllerConnected", &CInput::IsControllerConnected)
            .addFunction("SetDeadzone", &CInput::SetDeadzone)
            .addFunction("GetDeadzone", &CInput::GetDeadzone)
            .addFunction("GetControllerCount", &CInput::GetControllerCount)
            .endNamespace();

        // HitResult class + Physics namespace
        RaycastManager::lua_state = lua_state;
        luabridge::getGlobalNamespace(lua_state)
            .beginClass<HitResult>("HitResult")
            .addProperty("actor", &HitResult::actor)
            .addProperty("point", &HitResult::point)
            .addProperty("normal", &HitResult::normal)
            .addProperty("is_trigger", &HitResult::is_trigger)
            .endClass()
            .beginNamespace("Physics")
            .addFunction("Raycast", &RaycastManager::Raycast)
            .addFunction("RaycastAll", &RaycastManager::RaycastAll)
            .endNamespace();

        // Event namespace
        luabridge::getGlobalNamespace(lua_state)
            .beginNamespace("Event")
            .addFunction("Publish", &EventBus::Publish)
            .addFunction("Subscribe", &EventBus::Subscribe)
            .addFunction("Unsubscribe", &EventBus::Unsubscribe)
            .endNamespace();
    }
};

#endif // LUA_INJECTIONS_H
