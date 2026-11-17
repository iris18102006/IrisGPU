#include "ObjLoader.hpp"

#include <fstream>
#include <sstream>

namespace {

struct Corner {
    int v = 0;
    int vt = -1;
    int vn = -1;
};

// Resolves a 1-based (or negative, relative) OBJ index to a 0-based one. Returns -1 if absent/invalid.
int resolveIndex(int raw, int count) {
    if (raw > 0) return raw - 1 < count ? raw - 1 : -1;
    if (raw < 0) return count + raw >= 0 ? count + raw : -1;
    return -1;
}

bool parseCorner(const std::string& token, Corner& out, int vCount, int vtCount, int vnCount) {
    int parts[3] = {0, 0, 0};
    bool present[3] = {false, false, false};

    std::size_t start = 0;
    for (int i = 0; i < 3 && start <= token.size(); ++i) {
        const std::size_t slash = token.find('/', start);
        const std::string piece = token.substr(
            start, slash == std::string::npos ? std::string::npos : slash - start);

        if (!piece.empty()) {
            try {
                parts[i] = std::stoi(piece);
                present[i] = true;
            } catch (...) {
                return false;
            }
        }

        if (slash == std::string::npos) break;
        start = slash + 1;
    }

    if (!present[0]) return false;

    out.v = resolveIndex(parts[0], vCount);
    if (out.v < 0) return false;

    if (present[1]) out.vt = resolveIndex(parts[1], vtCount);
    if (present[2]) out.vn = resolveIndex(parts[2], vnCount);
    return true;
}

