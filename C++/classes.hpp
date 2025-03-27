#ifndef CLASSES_HPP
#define CLASSES_HPP

#include <vector>

class Point {
public:
    float x, y;

    // class constructor
    Point(float xCoord, float yCoord);
    
    // Print the coordinates of the point
    void displayPoint() const;

    // Calculate the distance to another point
    float distanceTo(const Point& other) const;

    // Find point X that is a certain distance from X, perpendicular to B
    Point pointPerpendicular(const Point& B, float distance) const;

    // Find point X that is a certain distance from X, opposite side of B
    Point oppositeSide(const Point& B, float distance) const;

    // Find a point closet to X that on line AB
    // Point closestPointOnLine(const Point& point_a, const Point& point_b) const;
    Point closestPointOnLine(const std::pair<Point, Point>& line) const;

    // Find point parrallel to line AB, certain distance from X
    Point pointParrallel(const Point& P1, const Point& P2, float distance) const;


    // Overload the < operator for sorting
    bool operator<(const Point& other) const;

};


class ObjectPolygon {
private:

    std::vector<Point> vertices;
    
    float isLeft(const Point& v1, const Point& v2, const Point& p) const;

    Point closestPointOnSegment(const Point& point_p, const Point& point_a, const Point& point_b);


public:
    // class constructor
    ObjectPolygon(const std::vector<Point>& pts);

    // Method to display the vertices of the polygon
    void displayPolygon() const;

    // Method to add a point to the polygon
    void addPoint(const Point& point);

    // Getter for vertices
    const std::vector<Point>& getVertices() const;

    // Method to access a point at a certain index
    Point getPointAt(size_t index) const;

    // Method to get line segments as a vector of pairs of points
    std::vector<std::pair<Point, Point>> getLineSegments() const;

    // Check if a point is inside the polygon using the winding number algorithm
    // @return true if the point is inside or on the edges of the polygon
    bool containPoint(const Point& p) const;

    // Function to check if this polygon is nested inside another polygon
    bool isNested(const ObjectPolygon& outer) const;

    // Compair point with polygon
    // @return Point closest on polygon edge, the edge closet and distance
    std::tuple<Point, std::pair<Point, Point>, float> closestOnPolygon(const Point& point_x) const;

    // Check distane point to polygon, compair with Purge margin
    // @return true if distance < margin
    bool closerThanMargin (const Point& point_x, float margin) const;

    // Get a point parallel to the closest edge of polygon
    // start at point_x, with certain distance
    Point pointAlongEdge(const Point& point_x, float distance) const;

};

#endif