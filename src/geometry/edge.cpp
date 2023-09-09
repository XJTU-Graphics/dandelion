#include "halfedge.h"

using std::size_t;
using Eigen::Vector3f;

Edge::Edge(size_t edge_id) : id(edge_id), halfedge(nullptr), is_new(false)
{
}

bool Edge::on_boundary() const
{
    return halfedge->is_boundary() || halfedge->inv->is_boundary();
}

Vector3f Edge::center() const
{
    const Vertex* v1 = this->halfedge->from;
    const Vertex* v2 = this->halfedge->inv->from;
    return (v1->pos + v2->pos) / 2.0f;
}

float Edge::length() const
{
    const Vertex* v1 = halfedge->from;
    const Vertex* v2 = halfedge->inv->from;
    return (v1->pos - v2->pos).norm();
}
