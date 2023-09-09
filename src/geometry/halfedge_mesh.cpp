#include "halfedge.h"

#include <map>
#include <unordered_map>
#include <array>
#include <vector>
#include <utility>
#include <string>

#include <Eigen/Core>
#include <Eigen/Dense>

#include "../utils/logger.h"
#include "../utils/math.hpp"

using Eigen::Matrix4f;
using Eigen::Vector3f;
using std::array;
using std::make_pair;
using std::map;
using std::monostate;
using std::optional;
using std::pair;
using std::set;
using std::size_t;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;
using std::visit;

size_t HalfedgeMesh::next_available_id = 0;

template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

HalfedgeMesh::HalfedgeMesh(Object& object)
    : inconsistent_element(monostate()), global_inconsistent(false), object(object),
      mesh(object.mesh), halfedge_arrows("Halfedge Mesh")
{
    logger                  = get_logger("Halfedge Mesh");
    const size_t n_vertices = mesh.vertices.count();
    const size_t n_faces    = mesh.faces.count();
    // Map vertex's index in GL::Mesh to pointer.
    unordered_map<size_t, Vertex*> index_to_vertex;
    // The degree of each vertex (i.e. the number of faces that contains it).
    unordered_map<size_t, size_t> v_degree;
    // Map face's index in GL::Mesh to pointer.
    unordered_map<size_t, Face*> index_to_face;
    // Map a pair (from , to) to a pointer pointing the corresponding halfedge, where `from` and
    // `to` are both indices in GL::Mesh.
    map<pair<size_t, size_t>, Halfedge*> endpoints_to_halfedge;

    // Create the vertices and record them in vid_to_vertex.
    v_pointers.reserve(n_vertices);
    v_pointers.resize(n_vertices);
    for (size_t index = 0; index < n_vertices; ++index) {
        Vertex* v         = new_vertex();
        v->pos            = mesh.vertex(index);
        v_indices[v]      = index;
        v_pointers[index] = v;
        index_to_vertex.emplace(index, v);
        v_degree[index] = 0;
    }
    logger->debug("vertices are recorded");

    // Create the faces and record them in fid_to_face. Also, count each vertex's
    // degree.
    for (size_t index = 0; index < n_faces; ++index) {
        Face* f = new_face();
        index_to_face.emplace(index, f);

        array<size_t, 3> v = mesh.face(index);
        for (size_t vid : v) {
            ++v_degree[vid];
        }
    }
    logger->debug("faces are recorded");

    // Iterate over faces to build halfedge connectivity.
    // By the end of this pass, the only halfedges that will not have a inversion
    // will be those that sit along the domain boundary (on boundary, but still
    // inside the mesh).
    for (size_t index = 0; index < n_faces; ++index) {
        array<size_t, 3> v = mesh.face(index);
        array<Halfedge*, 3> face_halfedges;
        // For each pair of vertices, try to create a halfedge.
        for (size_t i = 0; i < 3; ++i) {
            size_t a                = v[i];
            size_t b                = v[(i + 1) % 3];
            pair<size_t, size_t> ab = make_pair(a, b);
            Halfedge* h_ab;
            if (endpoints_to_halfedge.find(ab) != endpoints_to_halfedge.end()) {
                // If the halfedge has already been created, we have a problem.
                error_info = HalfedgeMeshFailure::MULTIPLE_ORIENTED_EDGES;
                logger->warn("found multiple oriented edges connecting vertices ({}, {})", ab.first,
                             ab.second);
                logger->warn("This means either");
                logger->warn("1) more than two faces contain this edge (hance the surface is "
                             "non-manifold), or");
                logger->warn(
                    "2) there are exactly two faces containing this edge, but they have the same "
                    "orientation (hence the surface is not consistently oriented");
                return;
            } else {
                h_ab                      = new_halfedge();
                endpoints_to_halfedge[ab] = h_ab;
                h_ab->face                = index_to_face[index];
                h_ab->face->halfedge      = h_ab;
                h_ab->from                = index_to_vertex[a];
                h_ab->from->halfedge      = h_ab;
                face_halfedges[i]         = h_ab;
            }
            // Check if the inversion of this halfedge has been created. If so,
            // link them together and create their shared edge.
            pair<size_t, size_t> ba = make_pair(b, a);
            if (endpoints_to_halfedge.find(ba) != endpoints_to_halfedge.end()) {
                Halfedge* h_ba = endpoints_to_halfedge[ba];
                h_ab->inv      = h_ba;
                h_ba->inv      = h_ab;
                Edge* edge     = new_edge();
                h_ab->edge     = edge;
                h_ba->edge     = edge;
                edge->halfedge = h_ab;
            } else {
                h_ab->inv = nullptr;
            }
        }
        // Link halfedges along the face loop via `next` and `prev`.
        for (size_t i = 0; i < 3; ++i) {
            const size_t next_id          = (i + 1) % 3;
            face_halfedges[i]->next       = face_halfedges[next_id];
            face_halfedges[next_id]->prev = face_halfedges[i];
        }
    }
    logger->debug("halfedges' basic connectivity are built");

    // Find vertices on the boundary of mesh, advance its halfedge pointer to a halfedge
    // which is also on the boundary.
    for (size_t vid = 0; vid < n_vertices; ++vid) {
        Vertex* v   = index_to_vertex[vid];
        Halfedge* h = v->halfedge;
        do {
            if (h->inv == nullptr) {
                v->halfedge = h;
                break;
            }
            h = h->inv->next;
        } while (h != v->halfedge);
    }

    // Connect all halfedges along each boundary loop and create virtual faces.
    for (Halfedge* h = halfedges.head; h != nullptr; h = h->next_node) {
        // A halfedge whose inversion does not exist is a halfedge along the boundary.
        // (But "inside" the domain boundary)
        if (h->inv == nullptr) {
            logger->debug("found a new boundary loop");
            // Found a new boundary loop, create a virtual face representing it.
            Face* virtual_face = new_face(true);
            // Keep all halfedges of the virtual face representing the boundary loop.
            vector<Halfedge*> boundary_halfedges;
            Halfedge* i = h;
            do {
                Halfedge* boundary_halfedge = new_halfedge();
                Edge* e                     = new_edge();
                e->halfedge                 = i;
                boundary_halfedges.push_back(boundary_halfedge);
                i->inv                  = boundary_halfedge;
                i->edge                 = e;
                boundary_halfedge->inv  = i;
                boundary_halfedge->from = i->next->from;
                boundary_halfedge->edge = e;
                boundary_halfedge->face = virtual_face;

                // Find the next halfedge along the boundary loop.
                i = i->next;
                while (i != h && i->inv != nullptr) {
                    i = i->inv->next;
                }
            } while (i != h);
            virtual_face->halfedge = boundary_halfedges.front();
            // Now all halfedges of the virtual face should have been created. Connect
            // them use the opposite order in the list, since the orientation of the
            // boundary loop is opposite the orientation of the halfedges "inside" the
            // domain boundary.
            const size_t degree = boundary_halfedges.size();
            for (size_t index = 0; index < degree; ++index) {
                const size_t next_index         = (index + degree - 1) % degree;
                const size_t prev_index         = (index + 1) % degree;
                boundary_halfedges[index]->next = boundary_halfedges[next_index];
                boundary_halfedges[index]->prev = boundary_halfedges[prev_index];
            }
        }
    }
    logger->debug("virtual faces representing boundary loops are created");

    // Check if all vertices are manifold.
    for (size_t vid = 0; vid < n_vertices; ++vid) {
        Vertex* v = index_to_vertex[vid];
        // There should not be any "floating" vertex in a 2-manifold mesh.
        if (v->halfedge == nullptr) {
            error_info = HalfedgeMeshFailure::NON_MANIFOLD_VERTEX;
            logger->warn("vertex {} is not referenced by any polygon", v->id);
            return;
        }
        // Each vertex should be a "fan" of faces, indicating the number of halfedges
        // emanating from the vertex is equal as the number of faces containing the
        // vertex.
        size_t count = 0;
        Halfedge* h  = v->halfedge;
        do {
            if (!(h->face->is_boundary)) {
                ++count;
            }
            h = h->inv->next;
        } while (h != v->halfedge);
        if (count != v_degree[vid]) {
            error_info = HalfedgeMeshFailure::NON_MANIFOLD_VERTEX;
            logger->warn("vertex {} is non-manifold (contained by {} non-boundary faces, but "
                         "only {} can be accessed via halfedges",
                         v->id, v_degree[vid], count);
            return;
        }
    }
    logger->debug("all vertices are manifold");

    regenerate_halfedge_arrows();
    logger->debug("the line set is initialized");
    optional<HalfedgeMeshFailure> validation_result = validate();
    if (!validation_result.has_value()) {
        logger->debug("validation passed");
    }
    error_info = std::nullopt;
    logger->debug("done");
}

HalfedgeMesh::~HalfedgeMesh()
{
    clear_erasure_records();
}

void HalfedgeMesh::sync()
{
    if (!global_inconsistent) {
        // Synchronize the inconsistent element
        const auto sync_vertex = [this](Vertex* vertex) {
            mesh.VAO.bind();
            mesh.vertices.update(v_indices[vertex], vertex->pos);
            mesh.VAO.release();
            const Halfedge* h = vertex->halfedge;
            halfedge_arrows.VAO.bind();
            do {
                if (!(h->is_boundary())) {
                    auto [from, to] = halfedge_arrow_endpoints(h);
                    halfedge_arrows.update_arrow(h_indices[h], from, to);
                }
                if (!(h->inv->is_boundary())) {
                    auto [from, to] = halfedge_arrow_endpoints(h->inv);
                    halfedge_arrows.update_arrow(h_indices[h->inv], from, to);
                }
                h = h->inv->next;
            } while (h != vertex->halfedge);
            halfedge_arrows.VAO.release();
        };
        const auto sync_edge = [&sync_vertex](Edge* edge) {
            Vertex* v1 = edge->halfedge->from;
            Vertex* v2 = edge->halfedge->inv->from;
            sync_vertex(v1);
            sync_vertex(v2);
        };
        const auto sync_face = [&sync_vertex](Face* face) {
            Halfedge* h = face->halfedge;
            do {
                sync_vertex(h->from);
                h = h->next;
            } while (h != face->halfedge);
        };
        visit(
            overloaded{[]([[maybe_unused]] monostate empty) {}, sync_vertex, sync_edge, sync_face},
            inconsistent_element);
        return;
    }

    logger->info("synchronize halfedge mesh to object {} (ID: {})", object.name, object.id);
    vector<unsigned int>& mesh_faces = mesh.faces.data;

    unordered_map<Vertex*, unsigned int> vertex_to_index;
    unsigned int counter = 0;
    mesh.clear();
    // Copy the vertices in HalfedgeMesh to GL::Mesh, use area weighted normal
    // as estimation of vertex normal.
    v_pointers.reserve(vertices.size);
    v_pointers.resize(vertices.size);
    for (Vertex* v = vertices.head; v != nullptr; v = v->next_node) {
        mesh.vertices.append(v->pos.x(), v->pos.y(), v->pos.z());
        vertex_to_index[v]  = counter;
        v_indices[v]        = static_cast<size_t>(counter);
        v_pointers[counter] = v;
        ++counter;

        // Traverse all adjacent faces to calculate a weighted average normal.
        Halfedge* h = v->halfedge;
        Vector3f normal(0.0f, 0.0f, 0.0f);
        do {
            Vector3f area_weighted_normal = h->face->area_weighted_normal();
            normal += area_weighted_normal;
            h = h->inv->next;
        } while (h != v->halfedge);
        normal.normalize();
        mesh.normals.append(normal.x(), normal.y(), normal.z());
    }
    logger->debug("vertex data is synchronized");
    for (Edge* e = edges.head; e != nullptr; e = e->next_node) {
        unsigned int v1 = vertex_to_index[e->halfedge->from];
        unsigned int v2 = vertex_to_index[e->halfedge->inv->from];
        mesh.edges.append(v1, v2);
    }
    logger->debug("edge data is synchronized");
    for (Face* f = faces.head; f != nullptr; f = f->next_node) {
        if (f->is_boundary) {
            // This is a virtual face representing a boundary loop, which should
            // not be synced back to the original mesh.
            continue;
        }
        Halfedge* h = f->halfedge;
        vector<unsigned int> vertices;
        do {
            mesh_faces.push_back(vertex_to_index[h->from]);
            vertices.push_back(vertex_to_index[h->from]);
            h = h->next;
        } while (h != f->halfedge);
    }
    logger->debug("face data is synchronized");
    object.modified = true;
    logger->debug("all data is synchronized, the object's dirty flag is set");
    regenerate_halfedge_arrows();
    logger->debug("halfedge arrows are regenerated");
    global_inconsistent = false;
    logger->info("synchronization done");
    logger->info("");
}

void HalfedgeMesh::render(const Shader& shader)
{
    shader.set_uniform("model", I4f);
    halfedge_arrows.render(shader);
}

tuple<Vector3f, Vector3f> HalfedgeMesh::halfedge_arrow_endpoints(const Halfedge* h)
{
    Vertex* v1 = h->from;
    Vertex* v2 = h->next->from;
    Vertex* v3 = h->next->next->from;
    // Slightly rise the arrow from the face to ensure it will not be occluded.
    Vector3f delta = h->face->normal() * (v1->pos - v2->pos).norm() * 0.01f;
    Vector3f from  = 0.86f * v1->pos + 0.09f * v2->pos + 0.05f * v3->pos + delta;
    Vector3f to    = 0.09f * v1->pos + 0.86f * v2->pos + 0.05f * v3->pos + delta;
    return {from, to};
}

Halfedge* HalfedgeMesh::new_halfedge()
{
    Halfedge* h = halfedges.append(next_available_id);
    ++next_available_id;
    return h;
}

Vertex* HalfedgeMesh::new_vertex()
{
    Vertex* v = vertices.append(next_available_id);
    ++next_available_id;
    return v;
}

Edge* HalfedgeMesh::new_edge()
{
    Edge* e = edges.append(next_available_id);
    ++next_available_id;
    return e;
}

Face* HalfedgeMesh::new_face(bool is_boundary)
{
    Face* f = faces.append(next_available_id, is_boundary);
    ++next_available_id;
    return f;
}

void HalfedgeMesh::regenerate_halfedge_arrows()
{
    halfedge_arrows.clear();
    h_indices.clear();
    size_t counter = 0;
    for (Halfedge* h = halfedges.head; h != nullptr; h = h->next_node) {
        // Do not draw the boundary halfedges on the virtual faces.
        if (h->face->is_boundary) {
            continue;
        }
        auto [from, to] = halfedge_arrow_endpoints(h);
        halfedge_arrows.add_arrow(from, to);
        h_indices[h] = counter;
        ++counter;
    }
    halfedge_arrows.to_gpu();
}

void HalfedgeMesh::erase(Halfedge* h)
{
    erased_halfedges[h->id] = halfedges.release(h);
}

void HalfedgeMesh::erase(Vertex* v)
{
    erased_vertices[v->id] = vertices.release(v);
}

void HalfedgeMesh::erase(Edge* e)
{
    erased_edges[e->id] = edges.release(e);
}

void HalfedgeMesh::erase(Face* f)
{
    erased_faces[f->id] = faces.release(f);
}

void HalfedgeMesh::clear_erasure_records()
{
    for (auto& p : erased_halfedges) {
        delete p.second;
    }
    for (auto& p : erased_vertices) {
        delete p.second;
    }
    for (auto& p : erased_edges) {
        delete p.second;
    }
    for (auto& p : erased_faces) {
        delete p.second;
    }
    erased_halfedges.clear();
    erased_vertices.clear();
    erased_edges.clear();
    erased_faces.clear();
}

optional<HalfedgeMeshFailure> HalfedgeMesh::validate()
{
    for (Vertex* v = vertices.head; v != nullptr; v = v->next_node) {
        bool is_finite =
            std::isfinite(v->pos.x()) && std::isfinite(v->pos.y()) && std::isfinite(v->pos.z());
        if (!is_finite) {
            logger->error("vertex {}'s position was set to a non-finite value", v->id);
            return HalfedgeMeshFailure::INIFINITE_POSITION_VALUE;
        }
    }

    unordered_map<Vertex*, set<Halfedge*>> v_accessible;
    unordered_map<Edge*, set<Halfedge*>> e_accessible;
    unordered_map<Face*, set<Halfedge*>> f_accessible;
    set<Halfedge*> permutation_next, permutation_prev;

    // Check valid halfedge permutation
    for (Halfedge* h = halfedges.head; h != nullptr; h = h->next_node) {
        if (erased_halfedges.find(h->id) != erased_halfedges.end()) {
            logger->error("an erased halfedge is still in the linked list");
            continue;
        }
        if (erased_halfedges.find(h->next->id) != erased_halfedges.end()) {
            logger->error("a live halfedge ({})'s next ({}) was erased", h->id, h->next->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (erased_halfedges.find(h->prev->id) != erased_halfedges.end()) {
            logger->error("a live halfedge ({})'s prev ({}) was erased", h->id, h->prev->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (erased_halfedges.find(h->inv->id) != erased_halfedges.end()) {
            logger->error("a live halfedge ({})'s inv ({}) was erased", h->id, h->inv->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (erased_vertices.find(h->from->id) != erased_vertices.end()) {
            logger->error("a live halfedge ({})'s from ({}) was erased", h->id, h->from->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (erased_edges.find(h->edge->id) != erased_edges.end()) {
            logger->error("a live halfedge ({})'s edge ({}) was erased", h->id, h->edge->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (erased_faces.find(h->face->id) != erased_faces.end()) {
            logger->error("a live halfface ({})'s face ({}) was erased", h->id, h->face->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }

        // Check whether each halfedge's next / prev points to a unique halfedge
        if (permutation_next.find(h->next) == permutation_next.end()) {
            permutation_next.insert(h->next);
        } else {
            logger->error("a halfedge ({}) is the next of multiple halfedges", h->next->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (permutation_prev.find(h->prev) == permutation_prev.end()) {
            permutation_prev.insert(h->prev);
        } else {
            logger->error("a halfedge ({}) is prev of multiple halfedges", h->prev->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
    }

    // Check whether each halfedge incident on a vertex points to that vertex
    for (Vertex* v = vertices.head; v != nullptr; v = v->next_node) {
        if (erased_vertices.find(v->id) != erased_vertices.end()) {
            logger->error("an erased vertex is still in the linked list");
            continue;
        }
        Halfedge* h = v->halfedge;
        if (erased_halfedges.find(h->id) != erased_halfedges.end()) {
            logger->error("a vertex ({})'s halfedge ({}) is erased", v->id, h->id);
            return HalfedgeMeshFailure::INVALID_VERTEX_CONNECTIVITY;
        }
        set<Halfedge*> accessible;
        do {
            accessible.insert(h);
            if (h->from != v) {
                logger->error("a vertex ({})'s halfedge ({}) does not pointing to that vertex",
                              v->id, h->id);
                return HalfedgeMeshFailure::INVALID_VERTEX_CONNECTIVITY;
            }
            h = h->inv->next;
        } while (h != v->halfedge);
        v_accessible[v] = std::move(accessible);
    }

    // Check whether each halfedge incident on an edge points to that edge
    for (Edge* e = edges.head; e != nullptr; e = e->next_node) {
        if (erased_edges.find(e->id) != erased_edges.end()) {
            logger->error("an erased edge is still in the linked list");
            continue;
        }
        Halfedge* h = e->halfedge;
        if (erased_halfedges.find(h->id) != erased_halfedges.end()) {
            logger->error("an edge ({})'s halfedge ({}) is erased", e->id, h->id);
            return HalfedgeMeshFailure::INVALID_EDGE_CONNECTIVITY;
        }
        set<Halfedge*> accessible;
        do {
            accessible.insert(h);
            if (h->edge != e) {
                logger->error("an edge ({})'s halfedge ({}) does not pointing to that edge", e->id,
                              h->id);
                return HalfedgeMeshFailure::INVALID_EDGE_CONNECTIVITY;
            }
            h = h->inv;
        } while (h != e->halfedge);
        e_accessible[e] = std::move(accessible);
    }

    // Check whether each halfedge incident on an face points to that face
    for (Face* f = faces.head; f != nullptr; f = f->next_node) {
        if (erased_faces.find(f->id) != erased_faces.end()) {
            logger->error("an erased face is still in the linked list");
            continue;
        }
        Halfedge* h = f->halfedge;
        if (erased_halfedges.find(h->id) != erased_halfedges.end()) {
            logger->error("a face ({})'s halfedge ({}) is erased", f->id, h->id);
            return HalfedgeMeshFailure::INVALID_FACE_CONNECTIVITY;
        }
        set<Halfedge*> accessible;
        do {
            accessible.insert(h);
            if (h->face != f) {
                logger->error("a face ({})'s halfedge ({}) does not pointing to that face", f->id,
                              h->id);
                return HalfedgeMeshFailure::INVALID_FACE_CONNECTIVITY;
            }
            h = h->next;
        } while (h != f->halfedge);
        f_accessible[f] = std::move(accessible);
    }

    for (Halfedge* h = halfedges.head; h != nullptr; h = h->next_node) {
        if (erased_halfedges.find(h->id) != erased_halfedges.end()) {
            // This halfedge has been erased, do not check it
            continue;
        }
        // Check whether this halfedge was pointed by a halfedge
        if (permutation_next.find(h) == permutation_next.end()) {
            logger->error("a halfedge ({}) is next of zero halfedge", h->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        if (permutation_prev.find(h) == permutation_prev.end()) {
            logger->error("a halfedge ({}) is prev of zero halfedge", h->id);
            return HalfedgeMeshFailure::INVALID_HALFEDGE_PERMUTATION;
        }
        // Check inversion relationships
        if (h->inv == h) {
            logger->error("a halfedge ({})'s inv is itself", h->id);
            return HalfedgeMeshFailure::ILL_FORMED_HALFEDGE_INVERSION;
        }
        if (h->inv->inv != h) {
            logger->error("a halfedge ({})'s inv's inv ({}) is not itself", h->id, h->inv->inv->id);
            return HalfedgeMeshFailure::ILL_FORMED_HALFEDGE_INVERSION;
        }
        // Check that the halfedge can be accessed via its from, edge and face
        if (v_accessible[h->from].find(h) == v_accessible[h->from].end()) {
            logger->error("a halfedge ({}) is not accessible from its from ({})", h->id,
                          h->from->id);
            return HalfedgeMeshFailure::POOR_HALFEDGE_ACCESSIBILITY;
        }
        if (e_accessible[h->edge].find(h) == e_accessible[h->edge].end()) {
            logger->error("a halfedge ({}) is not accessible from its edge ({})", h->id,
                          h->edge->id);
            return HalfedgeMeshFailure::POOR_HALFEDGE_ACCESSIBILITY;
        }
        if (f_accessible[h->face].find(h) == f_accessible[h->face].end()) {
            logger->error("a halfedge ({}) is not accessible from its face ({})", h->id,
                          h->face->id);
            return HalfedgeMeshFailure::POOR_HALFEDGE_ACCESSIBILITY;
        }
    }

    clear_erasure_records();
    return std::nullopt;
}
