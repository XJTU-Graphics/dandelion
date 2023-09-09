#include "halfedge.h"

#include <Eigen/Dense>

using Eigen::Vector3f;
using std::size_t;

Face::Face(size_t face_id, bool is_boundary)
    : id(face_id), halfedge(nullptr), is_boundary(is_boundary)
{
}

Vector3f Face::area_weighted_normal()
{
    Halfedge* h = halfedge;
    Vertex* v1  = h->from;
    Vertex* v2  = h->next->from;
    Vertex* v3  = h->next->next->from;
    Vector3f a  = v2->pos - v1->pos;
    Vector3f b  = v3->pos - v1->pos;
    return a.cross(b);
}

Vector3f Face::normal()
{
    return area_weighted_normal().normalized();
}

Vector3f Face::center() const
{
    const Halfedge* h = halfedge;
    Vector3f result(0.0f, 0.0f, 0.0f);
    size_t n_vertices = 0;
    do {
        result += h->from->pos;
        ++n_vertices;
        h = h->next;
    } while (h != halfedge);
    return result / n_vertices;
}
