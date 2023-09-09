#include "aabb.h"

#include <array>

#include <Eigen/Geometry>

using Eigen::Vector3f;

AABB::AABB()
{
    float min_num = std::numeric_limits<float>::lowest();
    float max_num = std::numeric_limits<float>::max();
    p_max         = Vector3f(min_num, min_num, min_num);
    p_min         = Vector3f(max_num, max_num, max_num);
}

AABB::AABB(const Vector3f& p1, const Vector3f& p2)
{
    p_min = Vector3f(fmin(p1.x(), p2.x()), fmin(p1.y(), p2.y()), fmin(p1.z(), p2.z()));
    p_max = Vector3f(fmax(p1.x(), p2.x()), fmax(p1.y(), p2.y()), fmax(p1.z(), p2.z()));
}
// 返回AABB的對角线长度
Vector3f AABB::diagonal() const
{
    return p_max - p_min;
}
// 返回AABB最长的一维
int AABB::max_extent() const
{
    Vector3f d = diagonal();
    if (d.x() > d.y() && d.x() > d.z())
        return 0;
    else if (d.y() > d.z())
        return 1;
    else
        return 2;
}
// 返回AABB的中心点
Vector3f AABB::centroid()
{
    return 0.5 * p_min + 0.5 * p_max;
}
// 判断当前射线是否与当前AABB相交
bool AABB::intersect(const Ray& ray, const Vector3f& inv_dir, const std::array<int, 3>& dir_is_neg)
{
    // these lines below are just for compiling and can be deleted
    (void)ray;
    (void)inv_dir;
    (void)dir_is_neg;
    return true;
    // these lines above are just for compiling and can be deleted

}
// 获取当前图元对应AABB
AABB get_aabb(const GL::Mesh& mesh, size_t face_idx)
{
    std::array<size_t, 3> face = mesh.face(face_idx);
    std::array<Vector3f, 3> v;
    v[0] = mesh.vertex(face[0]).homogeneous().topRows(3); // a
    v[1] = mesh.vertex(face[1]).homogeneous().topRows(3); // b
    v[2] = mesh.vertex(face[2]).homogeneous().topRows(3); // c
    return union_AABB(AABB(v[0], v[1]), v[2]);
}
// 将两个AABB用一个AABB包住
AABB union_AABB(const AABB& b1, const AABB& b2)
{
    AABB ret;
    ret.p_min = Vector3f(fmin(b1.p_min.x(), b2.p_min.x()), fmin(b1.p_min.y(), b2.p_min.y()),
                         fmin(b1.p_min.z(), b2.p_min.z()));
    ret.p_max = Vector3f(fmax(b1.p_max.x(), b2.p_max.x()), fmax(b1.p_max.y(), b2.p_max.y()),
                         fmax(b1.p_max.z(), b2.p_max.z()));
    return ret;
}
// 将一个AABB和一个点用一个AABB包住
AABB union_AABB(const AABB& b, const Vector3f& p)
{
    AABB ret;
    ret.p_min =
        Vector3f(fmin(b.p_min.x(), p.x()), fmin(b.p_min.y(), p.y()), fmin(b.p_min.z(), p.z()));
    ret.p_max =
        Vector3f(fmax(b.p_max.x(), p.x()), fmax(b.p_max.y(), p.y()), fmax(b.p_max.z(), p.z()));
    return ret;
}
