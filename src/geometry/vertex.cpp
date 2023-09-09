#include "halfedge.h"

using Eigen::Vector3f;
using std::size_t;

Vertex::Vertex(size_t vertex_id) : id(vertex_id), halfedge(nullptr), is_new(false)
{
}

size_t Vertex::degree() const
{
    const Halfedge* h = halfedge;
    size_t counter    = 0;
    do {
        const Face* f = h->face;
        if (!(f->is_boundary)) {
            ++counter;
        }
        h = h->inv->next;
    } while (h != halfedge);
    return counter;
}

Vector3f Vertex::neighborhood_center() const
{
    const Halfedge* h = halfedge;
    Vector3f center(0.0f, 0.0f, 0.0f);
    unsigned int n_neighbors = 0;
    do {
        center += h->inv->from->pos;
        ++n_neighbors;
        h = h->inv->next;
    } while (h != halfedge);
    return center / n_neighbors;
}

Vector3f Vertex::normal() const
{
    Halfedge* h = halfedge;
    Vector3f normal(0.0f, 0.0f, 0.0f);
    do {
        Vector3f area_weighted_normal = h->face->area_weighted_normal();
        normal += area_weighted_normal;
        h = h->inv->next;
    } while (h != halfedge);
    return normal.normalized();
}
