#include <Eigen/Core>
#include <Eigen/Geometry>
#include <catch2/catch_amalgamated.hpp>
#include <fstream>
#include <random>
#include <spdlog/spdlog.h>

#include "../src/geometry/halfedge.h"
#include "../src/scene/group.h"
#include "../src/scene/object.h"
#include "../src/utils/formatter.hpp"
#include "../src/utils/math.hpp"

using Eigen::AngleAxisf;
using Eigen::Matrix4f;
using Eigen::Scaling;
using Eigen::Translation3f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::default_random_engine;
using std::random_device;
using std::uniform_real_distribution;

using std::pair;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

constexpr float threshold     = 1e-3f;
constexpr float threshold_squ = 1e-6f;

TEST_CASE("Loop Subdivision", "[geometry]")
{

    vector<pair<string, string>> test_cases = {
        {"../input/geometry/cube.obj", "../ans/geometry/loop_subdivision/cube.txt"},
        {"../input/geometry/sphere.obj", "../ans/geometry/loop_subdivision/sphere.txt"},
        {"../input/geometry/cow.dae", "../ans/geometry/loop_subdivision/cow.txt"},
        {"../input/geometry/teapot.dae", "../ans/geometry/loop_subdivision/teapot.txt"},
        {"../input/geometry/bunny.obj", "../ans/geometry/loop_subdivision/bunny.txt"}};

    size_t case_id = 0;
    for (const auto& test_case : test_cases) {
        auto model_path      = test_case.first;
        auto std_result_path = test_case.second;

        spdlog::info("Case #{}: Testing loop subdivision of: {}", ++case_id, model_path);

        // Step 1. Load Test and STD Data
        Group test_group("Test group");
        bool model_load_ok = test_group.load(model_path);
        REQUIRE(model_load_ok);
        REQUIRE(!test_group.objects.empty());

        HalfedgeMesh test_mesh(**test_group.objects.begin());
        test_mesh.loop_subdivide();

        std::ifstream std_result_file(std_result_path);
        REQUIRE(std_result_file.is_open());

        size_t std_vertex_count;
        std_result_file >> std_vertex_count;
        vector<Vector3f> std_vertices(std_vertex_count);

        for (size_t i = 0; i < std_vertex_count; ++i) {
            float x, y, z;
            std_result_file >> x >> y >> z;
            std_vertices[i] = Vector3f(x, y, z);
        }

        size_t std_edge_count;
        std_result_file >> std_edge_count;
        set<pair<size_t, size_t>> std_edges;

        for (size_t i = 0; i < std_edge_count; ++i) {
            size_t v1, v2;
            std_result_file >> v1 >> v2;
            std_edges.insert({v1, v2});
        }

        std_result_file.close();

        // Step 2. Point Check and Mapping
        size_t test_vertex_count = test_mesh.vertices.size;
        INFO("Vertex count: Test: " << test_vertex_count << ", Expected: " << std_vertex_count);
        REQUIRE(test_vertex_count == std_vertex_count);

        unordered_map<Vertex*, size_t> test_vertex_id;

        for (Vertex* v = test_mesh.vertices.head; v != nullptr; v = v->next_node) {
            std::optional<size_t> id = std::nullopt;
            for (size_t i = 0; i < test_vertex_count; i++) {
                if ((v->pos - std_vertices[i]).squaredNorm() < threshold_squ) {
                    id = i;
                    break;
                }
            }
            INFO("At least one vertex has wrong position.");
            REQUIRE(id.has_value());
            test_vertex_id[v] = id.value();
        }

        // Step 3. Edge Check
        size_t test_edge_count = test_mesh.edges.size;
        INFO("Edge count: Test: " << test_edge_count << ", Expected: " << std_edge_count);
        REQUIRE(test_edge_count == std_edge_count);

        for (Edge* e = test_mesh.edges.head; e != nullptr; e = e->next_node) {
            Vertex* v1 = e->halfedge->from;
            Vertex* v2 = e->halfedge->inv->from;
            size_t id1 = test_vertex_id[v1];
            size_t id2 = test_vertex_id[v2];

            auto edge_pair          = std::make_pair(id1, id2);
            auto edge_pair_reversed = std::make_pair(id2, id1);

            auto it = std_edges.find(edge_pair);
            if (it == std_edges.end()) {
                it = std_edges.find(edge_pair_reversed);
            }
            INFO("At least one edge connects wrong vertexs.");
            REQUIRE(it != std_edges.end());
        }
        spdlog::info("Test Pass: loop subdivision of: {}", model_path);
    }
}
