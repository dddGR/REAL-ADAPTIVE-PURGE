#ifndef CALFUNCTION_HPP
#define CALFUNCTION_HPP

#include "classes.hpp"



/* Function to calculate the cross product of vectors OA and OB
A positive cross product indicates a counter-clockwise turn, 
a negative cross product indicates a clockwise turn, 
and zero indicates a collinear point. */
float cross(const Point& O, const Point& A, const Point& B);

// Function to compute the Convex Hull using Andrew's monotone chain algorithm
ObjectPolygon makePolygon(std::vector<Point>& points);

// Function to remove nested polygon in the list
std::vector<ObjectPolygon> removeNestedPolygon(const std::vector<ObjectPolygon> polygons);

// Get all the points from polygons and convert them into one polygon
ObjectPolygon mergeAllPolygons(const std::vector<ObjectPolygon>& polygons);

#endif
