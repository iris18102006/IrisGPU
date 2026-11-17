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

}  // namespace

bool parseObj(std::istream& input, Mesh& out, std::string* error) {
    out = Mesh{};

    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;
    bool anyMissingNormal = false;

    std::string line;
    int lineNumber = 0;

    auto fail = [&](const std::string& message) {
        if (error) *error = "line " + std::to_string(lineNumber) + ": " + message;
        return false;
    };

    while (std::getline(input, line)) {
        ++lineNumber;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "v") {
            Vec3 p;
            if (!(ss >> p.x >> p.y >> p.z)) return fail("bad vertex");
            out.vertices.push_back(p);
        } else if (tag == "vt") {
            Vec2 t;
            if (!(ss >> t.u >> t.v)) return fail("bad texcoord");
            uvs.push_back(t);
        } else if (tag == "vn") {
            Vec3 n;
            if (!(ss >> n.x >> n.y >> n.z)) return fail("bad normal");
            normals.push_back(n);
        } else if (tag == "f") {
            std::vector<Corner> face;
            std::string token;

            while (ss >> token) {
                Corner c;
                if (!parseCorner(token, c,
                                 static_cast<int>(out.vertices.size()),
                                 static_cast<int>(uvs.size()),
                                 static_cast<int>(normals.size()))) {
                    return fail("bad face index '" + token + "'");
                }
                face.push_back(c);
            }

            if (face.size() < 3) return fail("face needs at least 3 vertices");

            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const Corner tri[3] = {face[0], face[i], face[i + 1]};

                MeshTriangle t;
                for (int k = 0; k < 3; ++k) {
                    t.indices[k] = tri[k].v;
                    if (tri[k].vt >= 0) t.uvs[k] = uvs[tri[k].vt];

                    if (tri[k].vn >= 0) {
                        t.normals[k] = normalize(normals[tri[k].vn]);
                    } else {
                        anyMissingNormal = true;
                    }
                }
                out.triangles.push_back(t);
            }
        }
    }

    if (out.triangles.empty()) {
        if (error) *error = "no faces found";
        return false;
    }

    if (anyMissingNormal) out.computeSmoothNormals();
    return true;
}

bool loadObj(const std::string& path, Mesh& out, std::string* error) {
    std::ifstream file(path);
    if (!file) {
        if (error) *error = "cannot open '" + path + "'";
        return false;
    }

    if (!parseObj(file, out, error)) return false;

    out.name = path;
    return true;
}
