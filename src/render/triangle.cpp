#include <algorithm>
#include <array>
#include <iterator>

#include "triangle.h"

// Triangle的构造函数
Triangle::Triangle()
{
    viewport_pos[0] << 0, 0, 0, 1;
    viewport_pos[1] << 0, 0, 0, 1;
    viewport_pos[2] << 0, 0, 0, 1;

    world_pos[0] << 0, 0, 0, 1;
    world_pos[1] << 0, 0, 0, 1;
    world_pos[2] << 0, 0, 0, 1;
}
