#pragma once

struct Point
{
    int x;
    int y;
    bool operator==(const Point other) const
    {
        return this->x == other.x && this->y == other.y;
    }
};
