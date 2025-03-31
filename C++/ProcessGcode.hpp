#ifndef PROCESSGCODE_HPP
#define PROCESSGCODE_HPP


#include <string>
#include <vector>

#include "classes.hpp"



// Read all lines from a file and return them as vector of strings
std::vector<std::string> readFile(const std::string &file_path);

// Write Gcode to file
void writeFile(const std::string& file_path, const std::vector<std::string>& modifed_gcode);

// Function to check if a line contains a specific keyword
bool lineContain(const std::string_view keyword, const std::string_view line) noexcept;


// Function to check if a line starts with a specific prefix
bool lineStartWith(const std::string_view prefix, const std::string_view line) noexcept;


// Check slicer is used to generate G-code file
SlicerType checkSlicer(const std::vector<std::string> &content);


// Extract value (int) from a line
int get_value(const std::string &line);

// Extract string data from a line
std::string get_data(const std::string &line);


// Extract paremeters form OrcaSlicer sliced g-code
// all return variables are save at global
void get_info_orca(const std::vector<std::string> &content);


// Extract paremeters form PrusaSlicer sliced g-code
// all return variables are save at global
void get_info_prusa(const std::vector<std::string> &content);


/**
 * Extracts the bed polygon from the G-code content.
 *
 * @param content A vector of strings representing the lines of G-code content.
 * @return A ObjectPolygon representing the bed polygon.
 */
ObjectPolygon getBedPolygon(const std::vector<std::string> &content);


/**
 * Finds the first point of the print object in the provided G-code content.
 * 
 * This function iterates through the G-code content until it finds the first G1 command
 * after the ";LAYER_CHANGE" mark. It then extracts the X and Y coordinates from this
 * command and returns them as a Point.
 * 
 * If no G1 command is found, it returns an invalid point (999, 999).
 * 
 * @param content A reference to a vector of strings representing the lines of G-code content.
 * @return A Point representing the first point of the print object.
 */
Point getFirstPoint(const std::vector<std::string> &content);


// Function to extract and process polygons from the first layer
std::vector<std::vector<Point>> getPrintZonePoints(const std::vector<std::string>& content);

#endif
