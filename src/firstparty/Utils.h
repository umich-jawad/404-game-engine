// utils.h
// Utility functions and definitions for the project
#ifndef UTILS_H
#define UTILS_H
#include <utility>
#include <string>
#include <cstdio>
#include <iostream>
#include <cctype>
#include <glm/glm.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#pragma clang diagnostic pop
#include "AudioHelper.h"

struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const noexcept {
        return (static_cast<size_t>(v.x) << 32) ^ static_cast<size_t>(v.y);
    }
};

struct IVec2Equal {
    bool operator()(const glm::ivec2& a, const glm::ivec2& b) const noexcept {
        return a.x == b.x && a.y == b.y;
    }
};

class Utils {
public:
    // Pair hash function for unordered_set or unordered_map
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator() (const std::pair<T1, T2>& pair) const {
            auto hash1 = std::hash<T1>{}(pair.first);
            auto hash2 = std::hash<T2>{}(pair.second);
            return hash1 ^ hash2;
        }
    };

    static void ReadJsonFile(const std::string& path, rapidjson::Document & out_document)
    {
        FILE* file_pointer = nullptr;
    #ifdef _WIN32
        fopen_s(&file_pointer, path.c_str(), "rb");
    #else
        file_pointer = fopen(path.c_str(), "rb");
    #endif
        char buffer[65536];
        rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
        out_document.ParseStream(stream);
        std::fclose(file_pointer);

        if (out_document.HasParseError()) {
            rapidjson::ParseErrorCode errorCode = out_document.GetParseError();
            std::cout << "error parsing json at [" << path << "] Error code: " << errorCode << std::endl;
            exit(0);
        }
    }

    static std::string obtain_word_after_phrase(const std::string& input, const std::string& phrase) {
        // Find the position of the phrase in the string
        size_t pos = input.find(phrase);

        // If phrase is not found, return an empty string
        if (pos == std::string::npos) return "";

        // Find the starting position of the next word (skip spaces after the phrase)
        pos += phrase.length();
        while (pos < input.size() && std::isspace(input[pos])) {
            ++pos;
        }

        // If we're at the end of the string, return an empty string
        if (pos == input.size()) return "";

        // Find the end position of the word (until a space or the end of the string)
        size_t endPos = pos;
        while (endPos < input.size() && !std::isspace(input[endPos])) {
            ++endPos;
        }

        // Extract and return the word
        return input.substr(pos, endPos - pos);
    }

    static bool collision(const glm::vec2& center1, const glm::vec2& size1, const glm::vec2& center2, const glm::vec2& size2) {
        return glm::abs(center1.x - center2.x) * 2 < (size1.x + size2.x) &&
               glm::abs(center1.y - center2.y) * 2 < (size1.y + size2.y);
    }
};

#endif // UTILS_H