// Game.h
// Main game class header
#ifndef GAME_H
#define GAME_H

#include <string>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <utility>
#include "ConfigDB.h"
#include "Renderer.h"
#include "Actor.h"
#include "SceneDB.h"
#include "TemplateDB.h"
#include "ImageDB.h"
#include "TextDB.h"
#include "AudioDB.h"
#include "Input.h"
#include "CInput.h"
#include "ComponentManager.h"
#include "RaycastManager.h"
#include "Eventbus.h"
#include "LuaInjections.h"

class Game {
public:
    Game() {

        lua_State* lua_state = ComponentManager::getInstance().GetState();
        LuaInjections::InjectAll(lua_state);

        TemplateDB::getInstance();
        Input::init();
        CInput::Init();
        ConfigDB::init();
        SceneDB::init(ConfigDB::initial_scene);
        EngineRenderer::init(ConfigDB::game_title);
        ImageDB::init(EngineRenderer::getRenderer());
        TextDB::init(EngineRenderer::getRenderer());
        AudioDB::init();
    }

    void run() {
        bool quit = false;
        while (true) {
            quit = SDLEvents(); // Handle SDL events such as keyboard and mouse input

            updateScene(); // Update the game logic for the current scene
            EngineRenderer::clear(); // Clear the screen for the new frame
            renderScene();
            EngineRenderer::present(); // Present the rendered frame to the screen
            Input::LateUpdate(); // Update input states for the next frame
            CInput::LateUpdate(); // Update controller input states
            if (quit) exit(0); // Exit the game loop if quit signal received
        }
    }

private:
    bool SDLEvents() {
        SDL_Event e;
        while (Helper::SDL_PollEvent(&e)) {
            Input::ProcessEvent(e);
            CInput::ProcessEvent(e);
            if (e.type == SDL_QUIT) {
                return true; // Signal to quit the game loop
            }
        }
        return false;
    }

    void updateScene() {
        SceneDB::StartActors();
        SceneDB::UpdateActors();
        SceneDB::LateUpdateActors();
        SceneDB::ProcessUpdatedActors();
        EventBus::ProcessPending();
        Rigidbody::PhysicsStep();
        SceneDB::ApplyQueuedSceneSwitch();
    }

    void renderScene() {
        ImageDB::RenderQueuedImages(); // Render all queued image drawing requests (scene and UI)
        TextDB::RenderQueuedText(); // Render all queued text drawing requests
        ImageDB::RenderQueuedPixels(); // Render all queued pixels (on top of everything)
    }
};

#endif // GAME_H
