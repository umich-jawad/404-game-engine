#ifndef RENDERER_H
#define RENDERER_H

#include <filesystem>
#include "glm/glm.hpp"
#include "Utils.h"

class EngineRenderer {
public:
    inline static constexpr int PIXELS_PER_UNIT = 100;

    inline static int x_resolution = 640;
    inline static int y_resolution = 360;
    inline static int clear_color_r = 255;
    inline static int clear_color_g = 255;
    inline static int clear_color_b = 255;
    inline static float cam_offset_x = 0.0f;
    inline static float zoom_factor = 1.0f;
    inline static float cam_ease_factor = 1.0f;
    inline static SDL_Window* window = nullptr;
    inline static SDL_Renderer* renderer = nullptr;
    inline static glm::vec2 camera_position = glm::vec2(0.0f, 0.0f);
    inline static glm::vec2 camera_offset_base = glm::vec2(0.0f, 0.0f);

    static void UpdateCameraOffsetBase() {
        camera_offset_base = glm::vec2((x_resolution / zoom_factor) / 2.0f, (y_resolution / zoom_factor) / 2.0f);
        camera_offset_base -= glm::vec2(cam_offset_x * PIXELS_PER_UNIT, 0.0f);
    }

    static void init(std::string title) {
        if (std::filesystem::exists("resources/rendering.config")) {
            rapidjson::Document rconfig;
            Utils::ReadJsonFile("resources/rendering.config", rconfig);
            if (rconfig.HasMember("x_resolution") && rconfig["x_resolution"].IsInt()) {
                x_resolution = rconfig["x_resolution"].GetInt();
            }
            if (rconfig.HasMember("y_resolution") && rconfig["y_resolution"].IsInt()) {
                y_resolution = rconfig["y_resolution"].GetInt();
            }
            if (rconfig.HasMember("clear_color_r") && rconfig["clear_color_r"].IsInt()) {
                clear_color_r = rconfig["clear_color_r"].GetInt();
            }
            if (rconfig.HasMember("clear_color_g") && rconfig["clear_color_g"].IsInt()) {
                clear_color_g = rconfig["clear_color_g"].GetInt();
            }
            if (rconfig.HasMember("clear_color_b") && rconfig["clear_color_b"].IsInt()) {
                clear_color_b = rconfig["clear_color_b"].GetInt();
            }
        }
        UpdateCameraOffsetBase();
        window = Helper::SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, x_resolution, y_resolution, SDL_WINDOW_SHOWN);
        renderer = Helper::SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        applyWorldScale();
    }

    static void CameraSetPosition(float x, float y) {
        camera_position = glm::vec2(x, y);
    }

    static float CameraGetPositionX() {
        return camera_position.x;
    }

    static float CameraGetPositionY() {
        return camera_position.y;
    }

    static void CameraSetZoom(float zoom) {
        if (zoom <= 0.0f) return;
        zoom_factor = zoom;
        UpdateCameraOffsetBase();
        SDL_RenderSetScale(renderer, zoom, zoom);
    }

    static float CameraGetZoom() {
        return zoom_factor;
    }

    static void applyWorldScale() {
        SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);
    }

    static void applyUIScale() {
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    }

    static void clear() {
        SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255);
        SDL_RenderClear(renderer);
    }

    static void present() {
        Helper::SDL_RenderPresent(renderer);
    }

    // Renders a texture in world-space: converts world coords → pixels, applies camera offset and pivot.
    static void RenderSceneImage(SDL_Texture* texture, float x, float y, int rotation_degrees,
                                  float scale_x, float scale_y, float pivot_x, float pivot_y,
                                  int r, int g, int b, int a) {
        float w, h;
        Helper::SDL_QueryTexture(texture, &w, &h);
        int flip = SDL_FLIP_NONE;
        if (scale_x < 0) flip |= SDL_FLIP_HORIZONTAL;
        if (scale_y < 0) flip |= SDL_FLIP_VERTICAL;
        float abs_sx = glm::abs(scale_x);
        float abs_sy = glm::abs(scale_y);
        float piv_px = pivot_x * w * abs_sx;
        float piv_py = pivot_y * h * abs_sy;
        SDL_FRect dst = {
            x * PIXELS_PER_UNIT + (camera_offset_base.x - camera_position.x * PIXELS_PER_UNIT) - piv_px,
            y * PIXELS_PER_UNIT + (camera_offset_base.y - camera_position.y * PIXELS_PER_UNIT) - piv_py,
            w * abs_sx,
            h * abs_sy
        };
        SDL_FPoint pivot_point = {piv_px, piv_py};
        SDL_SetTextureColorMod(texture, r, g, b);
        SDL_SetTextureAlphaMod(texture, a);
        Helper::SDL_RenderCopyEx(-1, "", renderer, texture, nullptr, &dst,
                                 static_cast<double>(rotation_degrees), &pivot_point, static_cast<SDL_RendererFlip>(flip));
        SDL_SetTextureColorMod(texture, 255, 255, 255);
        SDL_SetTextureAlphaMod(texture, 255);
    }

    // Renders a texture in screen-space: raw pixel position, no camera transform.
    static void RenderUIImage(SDL_Texture* texture, float x, float y, int rotation_degrees,
                               int r, int g, int b, int a) {
        float w, h;
        Helper::SDL_QueryTexture(texture, &w, &h);
        SDL_FRect dst = {x, y, w, h};
        SDL_SetTextureColorMod(texture, r, g, b);
        SDL_SetTextureAlphaMod(texture, a);
        Helper::SDL_RenderCopyEx(-1, "", renderer, texture, nullptr, &dst,
                                 static_cast<double>(rotation_degrees), nullptr, SDL_FLIP_NONE);
        SDL_SetTextureColorMod(texture, 255, 255, 255);
        SDL_SetTextureAlphaMod(texture, 255);
    }

    static SDL_Renderer* getRenderer() {
        return renderer;
    }
};

#endif // SDLRENDERER_H