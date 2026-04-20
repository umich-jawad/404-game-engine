#ifndef IMAGEDB_H
#define IMAGEDB_H

#include <filesystem>
#include <queue>
#include <algorithm>
#include "Utils.h"

struct ImageDrawRequest {
    std::string image_name;
    float x;
    float y;
    int rotation_degrees = 0;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float pivot_x = 0.5f;
    float pivot_y = 0.5f;
    int r = 255;
    int g = 255;
    int b = 255;
    int a = 255;
    int sorting_order = 0;
    int request_order = 0;

    // Basic constructor - uses all defaults
    ImageDrawRequest(const std::string& name, float px, float py, int req_order)
        : image_name(name), x(px), y(py), request_order(req_order) {}

    // Full constructor - all parameters
    ImageDrawRequest(const std::string& name, float px, float py, int rot, float sx, float sy, float ppx, float ppy, int cr, int cg, int cb, int ca, int sort, int req_order)
        : image_name(name), x(px), y(py), rotation_degrees(rot), scale_x(sx), scale_y(sy), pivot_x(ppx), pivot_y(ppy), r(cr), g(cg), b(cb), a(ca), sorting_order(sort), request_order(req_order) {}
};

struct PixelDrawRequest {
    float x;
    float y;
    int r;
    int g;
    int b;
    int a;
};

class ImageDB {
public:
    static void init(SDL_Renderer* r) {
        ImageDB::renderer = r;
        
        // Preload all images from resources/images
        if (std::filesystem::exists("resources/images")) {
            for (const auto& entry : std::filesystem::directory_iterator("resources/images")) {
                if (entry.is_regular_file()) {
                    std::string path = entry.path().string();
                    std::string filename = entry.path().filename().string();
                    std::string image_name = filename.substr(0, filename.find_last_of('.'));
                    
                    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
                    if (texture) {
                        textures[image_name] = texture;
                    } else {
                        std::cout << "error: failed to load image " << path << std::endl;
                    }
                }
            }
        }
    }

    // Scene-space drawing functions
    static void Draw(const std::string& image_name, float x, float y) {
        scene_draw_queue.push(ImageDrawRequest(image_name, x, y, static_cast<int>(scene_draw_queue.size())));
    }

    static void DrawEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order) {
        scene_draw_queue.push(ImageDrawRequest(image_name, x, y, static_cast<int>(rotation_degrees), scale_x, scale_y, pivot_x, pivot_y, static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), static_cast<int>(a), static_cast<int>(sorting_order), static_cast<int>(scene_draw_queue.size())));
    }

    // UI drawing functions
    static void DrawUI(const std::string& image_name, float x, float y) {
        ui_draw_queue.push(ImageDrawRequest(image_name, x, y, static_cast<int>(ui_draw_queue.size())));
    }

    static void DrawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a, float sorting_order) {
        ui_draw_queue.push(ImageDrawRequest(image_name, x, y, 0, 1.0f, 1.0f, 0.5f, 0.5f, static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), static_cast<int>(a), static_cast<int>(sorting_order), static_cast<int>(ui_draw_queue.size())));
    }

    // Pixel drawing
    static void DrawPixel(float x, float y, float r, float g, float b, float a) {
        PixelDrawRequest request{
            x,
            y,
            static_cast<int>(r),
            static_cast<int>(g),
            static_cast<int>(b),
            static_cast<int>(a)
        };
        pixel_draw_queue.push(request);
    }

    static void RenderQueuedImages() {
        EngineRenderer::applyWorldScale();
        RenderQueue(scene_draw_queue, false);
        EngineRenderer::applyUIScale();
        RenderQueue(ui_draw_queue, true);
    }

    static void RenderQueuedPixels() {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        while (!pixel_draw_queue.empty()) {
            const PixelDrawRequest& req = pixel_draw_queue.front();
            SDL_SetRenderDrawColor(renderer, req.r, req.g, req.b, req.a);
            SDL_RenderDrawPoint(renderer, static_cast<int>(req.x), static_cast<int>(req.y));
            pixel_draw_queue.pop();
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    static void CreateDefaultParticleTextureWithName(const std::string & name) {
        // Have we already cached this default texture?
        if (textures.find(name) != textures.end())
            return;

        // Create an SDL_Surface (a cpu-side texture) with no special flags, 8 width, 8 height, 32 bits of color depth (RGBA) and no masking.
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);

        // Ensure color set to white (255, 255, 255, 255)
        Uint32 white_color = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
        SDL_FillRect(surface, NULL, white_color);

        // Create a gpu-side texture from the cpu-side surface now that we're done editing it.
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        // Clean up the surface and cache this default texture for future use (we'll probably spawn many particles with it).
        SDL_FreeSurface(surface);
        textures[name] = texture;
    }

    static SDL_Texture* GetTexture(const std::string& name) {
        if (textures.find(name) != textures.end()) {
            return textures[name];
        }
        return nullptr;
    }

private:
    static void RenderQueue(std::queue<ImageDrawRequest>& queue, bool is_ui) {
        std::vector<ImageDrawRequest> requests;
        while (!queue.empty()) {
            requests.push_back(queue.front());
            queue.pop();
        }

        std::stable_sort(requests.begin(), requests.end(),
            [](const ImageDrawRequest& a, const ImageDrawRequest& b) {
                if (a.sorting_order != b.sorting_order) return a.sorting_order < b.sorting_order;
                return a.request_order < b.request_order;
            });

        for (const auto& req : requests) {
            if (textures.find(req.image_name) == textures.end()) continue;
            SDL_Texture* texture = textures[req.image_name];
            if (is_ui)
                EngineRenderer::RenderUIImage(texture, req.x, req.y, req.rotation_degrees, req.r, req.g, req.b, req.a);
            else
                EngineRenderer::RenderSceneImage(texture, req.x, req.y, req.rotation_degrees, req.scale_x, req.scale_y, req.pivot_x, req.pivot_y, req.r, req.g, req.b, req.a);
        }
    }

    inline static std::unordered_map<std::string, SDL_Texture*> textures;
    inline static std::queue<ImageDrawRequest> scene_draw_queue;
    inline static std::queue<ImageDrawRequest> ui_draw_queue;
    inline static std::queue<PixelDrawRequest> pixel_draw_queue;
    inline static SDL_Renderer* renderer = nullptr;

};

#endif // IMAGEDB_H