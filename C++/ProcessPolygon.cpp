#include <vector>
#include <deque>
#include <algorithm>

#include "classes.hpp"
#include "ProcessPolygon.hpp"



ObjectPolygon makePolygon(std::vector<Point>& points)
{
    std::sort(points.begin(), points.end());

    std::vector<Point> lower, upper;

    // Build the lower hull
    for (const auto& p : points)
    {
        while (lower.size() >= 2 && cross(lower[lower.size() - 2], lower.back(), p) <= 0)
        {
            lower.pop_back();
        }
        lower.push_back(p);
    }

    // Build the upper hull
    for (int i = points.size() - 1; i >= 0; --i)
    {
        const auto& p = points[i];
        while (upper.size() >= 2 && cross(upper[upper.size() - 2], upper.back(), p) <= 0)
        {
            upper.pop_back();
        }
        upper.push_back(p);
    }

    // Remove the last point of each half because it is repeated at the beginning of the other half
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());

    return ObjectPolygon(lower);
}

float cross(const Point& O, const Point& A, const Point& B)
{
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}


std::vector<ObjectPolygon> removeNestedPolygon(const std::vector<ObjectPolygon> polygons)
{
    std::vector<ObjectPolygon> non_nested_polygons;

    for (const auto& polygon1 : polygons)
    {
        bool is_nested = false;

        for (const auto& polygon2 : polygons)
        {
            // Check if polygon1 is nested inside polygon2
            if ((&polygon2 != &polygon1) && polygon1.isNested(polygon2))
            {
                is_nested = true;
                break;
            }
        }

        if (!is_nested)
        {
            non_nested_polygons.push_back(std::move(polygon1));
        }
    }

    return non_nested_polygons;
}


ObjectPolygon mergeAllPolygons(const std::vector<ObjectPolygon>& polygons) 
{
    std::vector<Point> all_points;

    for (const ObjectPolygon& polygon : polygons) 
    {
        all_points.insert(all_points.end(), polygon.getVertices().begin(), polygon.getVertices().end());
    }

    return ObjectPolygon(makePolygon(all_points));

}

