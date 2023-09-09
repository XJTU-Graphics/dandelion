#include "halfedge.h"

using std::size_t;

Halfedge::Halfedge(size_t halfedge_id)
    : id(halfedge_id), next(nullptr), prev(nullptr), inv(nullptr), from(nullptr), edge(nullptr),
      face(nullptr)
{
}

void Halfedge::set_neighbors(Halfedge* next, Halfedge* prev, Halfedge* inv, Vertex* from,
                             Edge* edge, Face* face)
{
    this->next = next;
    this->prev = prev;
    this->inv  = inv;
    this->from = from;
    this->edge = edge;
    this->face = face;
}

bool Halfedge::is_boundary() const
{
    return face->is_boundary;
}
