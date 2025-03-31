#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "global.hpp"
#include "classes.hpp"
#include "ProcessPolygon.hpp"
#include "ProcessGcode.hpp"

using std::cout, std::endl, std::string, std::vector, std::pair;

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Extracts and processes polygons from the first layer of G-code content.
 *
 * This function retrieves all the print zone points from the first layer of the 
 * provided G-code content, converts these points into polygon objects, and removes 
 * any nested polygons from the resulting collection.
 *
 * @param content A reference to a vector of strings representing the lines of G-code content.
 * @return A vector of ObjectPolygon objects representing non-nested polygons from the first layer.
 */
vector<ObjectPolygon> getObjectPolygons(const vector<string>& content);

// Find position of Purge-Line relative to print area.
// @return Purge-Line first -> second point
pair<Point, Point> calPurgeLine(const Point& first_point, const vector<ObjectPolygon>& polygons, ObjectPolygon& bed_poly);

// Calculate Purge-Line second point, with first print point and print object area.
pair<Point, Point> calPurgeSecondPoint(const ObjectPolygon& polygon, const Point& point_x, const float& distance);

// Chose Purge-Line on Bed corner or edge.
pair<Point, Point> purgeOnEdge(const ObjectPolygon& bed_poly, const Point& point_1);

// Generate Purge-Line and add to Gcode content
void generateGcode(const Point& first_point, const pair<Point, Point>& purge_line, vector<string>& content);

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) 
{
    // Check if the user provided a file path as an argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << endl;
        return 1; // Return with an error code
    }
    // Get the file path from the command-line arguments
    const string filePath = argv[1];

    vector<string> content;
    
    try 
    {
        content = readFile(filePath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // cout << "Slicer: " << g_Slicer << endl;

    switch (checkSlicer(content))
    {
        case ORCA_SLICER:
            get_info_orca(content);
            break;
        
        case PRUSA_SLICER:
            get_info_prusa(content);
            break;

        default:
            std::cerr << "Slicer not supported" << endl;
            return 1;
    }

    Point firstPoint = getFirstPoint(content);

    // firstPoint.displayPoint();

    vector<ObjectPolygon> polygons = getObjectPolygons(content);

    // cout << "Find " << polygons.size() << " polygons" << "\n";
    // for (auto& polygon : polygons) {
    //     polygon.displayPolygon();
    // }

    
    ObjectPolygon bed_polygon = getBedPolygon(content);

    pair<Point, Point> purge_line = pair<Point, Point>(Point(0, 0), Point(0, 0));
    try
    {
        purge_line = calPurgeLine(firstPoint, polygons, bed_polygon);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // purge_line.first.displayPoint();
    // purge_line.second.displayPoint();

    /* Generate Purge-Line and add to Gcode content.
    Save it to gcode file */
    generateGcode(firstPoint, purge_line, content);
    writeFile(filePath, content);
    
    return 0; // Return success
}


vector<ObjectPolygon> getObjectPolygons(const vector<string>& content)
{
    vector<ObjectPolygon> polygons;

    for (vector<Point>& object_points : getPrintZonePoints(content))
    {
        polygons.emplace_back(makePolygon(object_points));
    }

    polygons = removeNestedPolygon(std::move(polygons));

    return polygons;
}



pair<Point, Point> purgeOnEdge(const ObjectPolygon& bed_poly, const Point& point_1)
{
    // get all corners and mid-point of bed edge
    vector<Point> points_can_purge;

    for (const auto& segment : bed_poly.getLineSegments()) {
        points_can_purge.push_back(segment.first);

        float mid_x = (segment.first.x + segment.second.y) / 2;
        float mid_y = (segment.first.y + segment.second.y) / 2;

        points_can_purge.push_back(Point(mid_x, mid_y));
    }

    Point bed_point = std::get<0>(bed_poly.closestOnPolygon(point_1));


    Point closet_point(0, 0);
    float min_distance = std::numeric_limits<float>::max();

    for (const auto& point : points_can_purge) {
        if (point.distanceTo(point_1) < min_distance) {
            min_distance = point.distanceTo(point_1);
            closet_point = point;
        }
    }

    Point purge_begin_point = closet_point.oppositeSide(bed_point, g_purge_length);

    /* if pointY inside bed (closest_point must be mid-point of the edge)
     Purge-Line is from pointY -> closest_point
     otherwise, revert Purge-Line direction */
    if (bed_poly.containPoint(purge_begin_point)) {
        return {purge_begin_point, bed_point};
    } 
    else {
        purge_begin_point = closet_point.oppositeSide(purge_begin_point, g_purge_length);
        return {bed_point, purge_begin_point};
    }

}


// Calculate Purge-Line second point, with first print point and print object area.
pair<Point, Point> calPurgeSecondPoint(const ObjectPolygon& polygon, const Point& point_x, const float& distance) 
{
    Point point_1(0, 0);
    
    auto [cls_point, cls_edge, cls_distance] = polygon.closestOnPolygon(point_x);

    if (cls_distance == 0)
    {
        point_1 = point_x.pointPerpendicular(cls_edge.first, distance);
    }
    else if (polygon.containPoint(point_x))
    {
        point_1 = cls_point.oppositeSide(point_x, distance);
    }
    else
    {
        point_1 = point_x.oppositeSide(cls_point, distance);
    }

    return pair<Point, Point>(cls_point, point_1);
}


pair<Point, Point> calPurgeLine(const Point& first_point, const vector<ObjectPolygon>& polygons, ObjectPolygon& bed_poly) 
{
    auto [point_0, point_1] = calPurgeSecondPoint(polygons.front(), first_point, g_purge_margin);

    /* Check point_1 is inside other objects (if print multiple objects)
    If is true, likely objects too close to each other
    -> merge all polygons and recalculate. */
    for (const auto& poly : polygons) 
    {
        if ((poly.containPoint(point_1)) || (poly.closerThanMargin(point_1, g_purge_margin))) 
        {
            ObjectPolygon new_polygon = mergeAllPolygons(polygons);

            std::tie(point_0, point_1) = calPurgeSecondPoint(new_polygon, first_point, g_purge_margin);
        }
    }

    /* Check if point_1 (aka. Purge-Line end point) is inside bed.
     if not, likely object is too close to bed.
     -> recalculate Purge-Line to be on bed edge or coner. */
    Point point_2(0, 0);
    if (bed_poly.containPoint(point_1)) {
        point_2 = point_1.oppositeSide(point_0, g_purge_length);
    }
    else {
        return pair<Point, Point> (purgeOnEdge(bed_poly, point_1));
    }

    /* Check if point_2(aka. Purge-Line start point) is inside the bed or other objects print area
     If is true -> recalculate point_2 to be parrallel to polygon edge. */
    if (!bed_poly.containPoint(point_2))
    {
        point_2 = bed_poly.pointAlongEdge(point_1, g_purge_length);
    }

    bool flag = true;
    int count = 0;
    while (flag)
    {
        flag = false;
        for (const auto& polygon : polygons)
        {
            if ((polygon.containPoint(point_2)) || (polygon.closerThanMargin(point_2, g_purge_margin)))
            {
                point_2 = polygon.pointAlongEdge(point_1, g_purge_length);
                flag = true;
                count++;
                break;
            }
        }

        if (count > 5 && flag)
        {
            throw std::runtime_error("Error: Purge-Line is too close to other objects");
            break;
        }
    }

    return pair<Point, Point> (std::move(point_2), std::move(point_1));
}


void generateGcode(const Point& first_point, const pair<Point, Point>& purge_line, vector<string>& content)
{

    float purge_move_speed = std::pow(((g_flow_rate / 5) * 60), 1);

    string retract = "G1 E-0.8 F2100";
    string unRetract = "G1 E0.8 F2100";

    if (g_UseFirmwareRetraction) {
        retract = "G10";
        unRetract = "G11";
    }

    std::ostringstream purge_gcode;
    purge_gcode << "\n; Adaptive Purge START\n";
    purge_gcode << "M118 [ADAPTIVE PURGE] start from (" << purge_line.first.x << ", " << purge_line.first.y << ") to (" << purge_line.second.x << ", " << purge_line.second.y << ") with flow rate " << g_flow_rate << "mm3/s.\n";
    purge_gcode << "G92 E0 ; Reset extruder" << "\n";
    purge_gcode << "G90 ; Absolute positioning" << "\n";
    purge_gcode << "G0 X" << purge_line.first.x << " Y" << purge_line.first.y << " F" << g_TravelSpeed << " ; Move to purge position" << "\n";
    purge_gcode << "G0 Z" << g_purge_height << " ; Move to purge Z height" << "\n";
    purge_gcode << "M83 ; Relative extrusion mode" << "\n";
    purge_gcode << "G1 E" << g_tip_distance << " F" << purge_move_speed << " ; Move filament tip" << "\n";
    purge_gcode << "G1 X" << purge_line.second.x << " Y" << purge_line.second.y << " E" << g_purge_amount << " F" << purge_move_speed << " ; Purge line" << "\n";
    purge_gcode << retract << "\n";
    purge_gcode << "G0 X" << first_point.x << " Y" << first_point.y << " F" << g_TravelSpeed << " ; Rapid move to break string" << "\n";
    purge_gcode << unRetract << "\n";
    purge_gcode << "G92 E0 ; Reset extruder distance" << "\n";
    purge_gcode << "; Adaptive Purge END" << "\n\n";

    // cout << purge_gcode.str() << "\n";

    std::string_view insert_points[] = {"G20" , "G21", "G90", "G91"};

    for (size_t i = 0; i < content.size(); ++i)
    {
        const std::string &line = content[i];

        if ( std::any_of(std::cbegin(insert_points), std::cend(insert_points),
        [&line](const auto& str) { return lineStartWith(str, line); }) )
        {
            content.insert(content.begin() + i, purge_gcode.str());
            break;
        }        
    }
}
