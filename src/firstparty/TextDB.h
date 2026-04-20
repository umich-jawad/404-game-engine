#ifndef TEXTDB_H
#define TEXTDB_H

#include <filesystem>
#include <queue>
#include "Utils.h"
#include <SDL/SDL_ttf.h>

struct TextDrawRequest {
    std::string str_content;
    float x;
    float y;
    std::string font_name;
    int font_size;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

class TextDB {
public:
    static void init(SDL_Renderer* r) {
        TextDB::renderer = r;
        TTF_Init();
        if (!std::filesystem::exists("resources/fonts")) return;
        // Load font paths from resources/fonts directory
        for (const auto& entry : std::filesystem::directory_iterator("resources/fonts/")) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string filename = entry.path().filename().string();
                std::string font_name = filename.substr(0, filename.find_last_of('.'));
                font_paths[font_name] = path;
            }
        }
    }

    static void Draw(const std::string& str_content, float x, float y, const std::string& font_name, float font_size, float r, float g, float b, float a) {
        TextDrawRequest request{
            str_content,
            x,
            y,
            font_name,
            static_cast<int>(font_size),
            static_cast<Uint8>(r),
            static_cast<Uint8>(g),
            static_cast<Uint8>(b),
            static_cast<Uint8>(a)
        };
        draw_queue.push(request);
    }

    static void RenderQueuedText() {
        while (!draw_queue.empty()) {
            const TextDrawRequest& req = draw_queue.front();
            
            // Get or load font at requested size
            auto path_it = font_paths.find(req.font_name);
            if (path_it == font_paths.end()) {
                std::cout << "error: font " << req.font_name << " not found" << std::endl;
                draw_queue.pop();
                continue;
            }

            TTF_Font* font = GetOrLoadFont(path_it->second, req.font_name, req.font_size);
            if (!font) {
                std::cout << "error: failed to load font " << req.font_name << " at size " << req.font_size << std::endl;
                draw_queue.pop();
                continue;
            }

            SDL_Color color = { req.r, req.g, req.b, req.a };
            if (req.str_content.empty()) {
                draw_queue.pop();
                continue;
            }
            SDL_Surface* surf = TTF_RenderText_Solid(font, req.str_content.c_str(), color);
            if (!surf) {
                std::cout << "error: failed to render text surface" << std::endl;
                //verbose error
                std::cout << "  content: " << req.str_content << std::endl;
                std::cout << "  font: " << req.font_name << std::endl;
                std::cout << "  size: " << req.font_size << std::endl;
                draw_queue.pop();
                continue;
            }

            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
            if (!texture) {
                std::cout << "error: failed to create texture from surface" << std::endl;
                SDL_FreeSurface(surf);
                draw_queue.pop();
                continue;
            }

            SDL_FRect dst { req.x, req.y, static_cast<float>(surf->w), static_cast<float>(surf->h) };
            SDL_FreeSurface(surf);
            Helper::SDL_RenderCopyEx(-1, "", renderer, texture, nullptr, &dst, 0.0f, nullptr, SDL_FLIP_NONE);
            SDL_DestroyTexture(texture);
            
            draw_queue.pop();
        }
    }

private:
    static TTF_Font* GetOrLoadFont(const std::string& path, const std::string& font_name, int font_size) {
        auto size_map_it = fonts.find(font_name);
        if (size_map_it == fonts.end()) {
            // Font name doesn't exist yet, create new map
            fonts[font_name] = std::unordered_map<int, TTF_Font*>();
            size_map_it = fonts.find(font_name);
        }

        auto& size_map = size_map_it->second;
        auto font_it = size_map.find(font_size);
        if (font_it != size_map.end()) {
            return font_it->second; // Already loaded
        }

        // Load font at requested size
        TTF_Font* font = TTF_OpenFont(path.c_str(), font_size);
        if (font) {
            size_map[font_size] = font;
        }
        return font;
    }

    inline static std::unordered_map<std::string, std::string> font_paths;  // font_name -> file_path
    inline static std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fonts;  // font_name -> size -> TTF_Font*
    inline static std::queue<TextDrawRequest> draw_queue;
    inline static SDL_Renderer* renderer = nullptr;

};

#endif // TEXTDB_H