#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "classes.hpp"



Point::Point(float xCoord, float yCoord) : x(xCoord), y(yCoord) {}


void Point::displayPoint() const {
    std::cout << "(" << x << "," << y << ") ";
}


float Point::distanceTo(const Point& other) const {
    return std::sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
}


Point Point::pointPerpendicular(const Point& B, float distance) const 
{
    // Calculate the perpendicular vector
    float dirX = y - B.y;
    float dirY = B.x - x;

    // Scale the perpendicular vector to the specified distance
    float scale = distance / std::sqrt(dirX * dirX + dirY * dirY);
    dirX *= scale;
    dirY *= scale;

    return Point(x + dirX, y + dirY);
}   


Point Point::oppositeSide(const Point& B, float distance) const 
{
    // Calculate the direction vector from A to B
    float dirX = B.x - x;
    float dirY = B.y - y;

    // Normalize the direction vector
    float unitDirX = dirX / std::sqrt(dirX * dirX + dirY * dirY);
    float unitDirY = dirY / std::sqrt(dirX * dirX + dirY * dirY);

    return Point (x - unitDirX * distance, y - unitDirY * distance);
}


Point Point::closestPointOnLine(const std::pair<Point, Point>& line) const 
{
    float abx = line.second.x - line.first.x;
    float aby = line.second.y - line.first.y;

    float apx = x - line.first.x;
    float apy = y - line.first.y;

    float ab2 = abx * abx + aby * aby;
    if (ab2 == 0) return line.first; // Degenerate segment

    float t = (apx * abx + apy * aby) / ab2;
    t = std::clamp(t, 0.0f, 1.0f);

    return Point(line.first.x + t * abx, line.first.y + t * aby);
}


Point Point::pointParrallel(const Point& A, const Point& B, float distance) const {
    // Calculate the direction vector of line L
    float dx = B.x - A.x;
    float dy = B.y - A.y;

    // Calculate the length of the direction vector
    float len = std::hypot(dx, dy);

    // Scale the direction vector to the desired distance
    dx = (dx / len) * distance;
    dy = (dy / len) * distance;

    return Point(x + dx, y + dy);
}


bool Point::operator<(const Point& other) const {
    return (x < other.x) || (x == other.x && y < other.y);
}


// Constructor to initialize the polygon with a vector of points
ObjectPolygon::ObjectPolygon(const std::vector<Point>& pts) : vertices(pts) {}

    // // Method to add a vertex to the polygon
    // void addVertex(float x, float y) {
    //     vertices.emplace_back(x, y);
    // }


void ObjectPolygon::addPoint(const Point& point) 
{
    vertices.push_back(point);
}


const std::vector<Point>& ObjectPolygon::getVertices() const 
{
    return vertices;
}


Point ObjectPolygon::getPointAt(size_t index) const 
{
    if (index >= vertices.size()) {
        throw std::out_of_range("Index out of range");
    }
    return vertices[index];
}


std::vector<std::pair<Point, Point>> ObjectPolygon::getLineSegments() const 
{
    std::vector<std::pair<Point, Point>> segments;

    for (size_t i = 0; i < vertices.size(); ++i) 
    {
        Point start = vertices[i];
        Point end = vertices[(i + 1) % vertices.size()];
        segments.emplace_back(start, end);
    }

    return segments;
}


void ObjectPolygon::displayPolygon() const 
{
    std::cout << "Vertices of the polygon: [ ";
    for (const auto &vertex : vertices) {
        std::cout << "(" << vertex.x << "," << vertex.y << ") ";
    }
    std::cout << "]\n";
}


bool ObjectPolygon::containPoint(const Point& p) const 
{
    int windingNumber = 0;
    int n = vertices.size();

    for (int i = 0; i < n; i++) 
    {
        Point v1 = vertices[i];
        Point v2 = vertices[(i + 1) % n]; // Wrap around to the first vertex

        // Check if the edge crosses the horizontal line from the point
        if (v1.y <= p.y) {
            if ((v2.y > p.y) && (isLeft(v1, v2, p) > 0))
            {
                windingNumber++; // Point is to the left of the edge
            }
        } 
        else if ((v2.y <= p.y) && (isLeft(v1, v2, p) < 0))
        {
            windingNumber--; // Point is to the right of the edge
        }
    }

    return windingNumber != 0; // Non-zero winding number means inside
}


bool ObjectPolygon::isNested(const ObjectPolygon& outer) const 
{
    for (const Point& vertex : vertices) 
    {
        if (!outer.containPoint(vertex)) 
        {
            return false; // If any vertex is outside, it's not nested
        }
    }
    return true; // All vertices are inside the outer polygon
}


std::tuple<Point, std::pair<Point, Point>, float> ObjectPolygon::closestOnPolygon(const Point& point_x) const 
{
    Point closest_point(0, 0);

    std::pair<Point, Point> closest_edge = std::pair(Point(0, 0), Point(0, 0));
    float minDistance = std::numeric_limits<float>::max();

    std::vector<std::pair<Point, Point>> lines = this->getLineSegments();

    for (const auto& line : lines) 
    {

        // Find the closest point on the current edge
        Point closestOnEdge = point_x.closestPointOnLine(line);

        float distance = point_x.distanceTo(closestOnEdge);

        // Distance is zero means the point is on the edge
        if (distance < 1e-9)
        {
            return std::tuple(closestOnEdge, line, 0.0f);
        }

        // Update the closest point if this one is closer
        if (distance < minDistance)
        {
            minDistance = distance;
            closest_point = closestOnEdge;
            closest_edge = line;
        }
    }

    return std::tuple(closest_point, closest_edge, minDistance);
}


bool ObjectPolygon::closerThanMargin (const Point& point_x, float margin) const
{
    if (std::get<2>(closestOnPolygon(point_x)) < margin) {
        return true;
    }

    return false;
}


Point ObjectPolygon::pointAlongEdge(const Point& point_x, float distance) const
{
    auto edge = std::get<1>(closestOnPolygon(point_x));

    return Point(point_x.pointParrallel(edge.first, edge.second, distance));
}


// Function to find the closest point on a line segment (edge) to a given point
Point ObjectPolygon::closestPointOnSegment(const Point& point_p, const Point& point_a, const Point& point_b) {
    float abx = point_b.x - point_a.x;
    float aby = point_b.y - point_a.y;

    float apx = point_p.x - point_a.x;
    float apy = point_p.y - point_a.y;

    float ab2 = abx * abx + aby * aby;
    if (ab2 == 0) return point_a; // Degenerate segment

    float t = (apx * abx + apy * aby) / ab2;
    t = std::clamp(t, 0.0f, 1.0f);

    return Point(point_a.x + t * abx, point_a.y + t * aby);
}

// Function to determine if the point p is to the left of the line from v1 to v2
float ObjectPolygon::isLeft(const Point& v1, const Point& v2, const Point& p) const {
    return (v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x);
}
