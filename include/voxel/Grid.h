#ifndef QUESTFARERGAMEENGINE_GRID_H
#define QUESTFARERGAMEENGINE_GRID_H


#include <memory>
#include "Chunk.h"
#include "glm/glm.hpp"

namespace std {
    template <>
    struct hash<glm::ivec2> {
        size_t operator()(const glm::ivec2& v) const noexcept {
            return hash<int>()(v.x) ^ (hash<int>()(v.y) << 1);
        }
    };
}

namespace glm {
    inline bool operator<(const ivec2& a, const ivec2& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    }
}
class Grid {

public:

    Grid();


    std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> chunks;
};


#endif //QUESTFARERGAMEENGINE_GRID_H
