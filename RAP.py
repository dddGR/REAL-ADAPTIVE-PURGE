#!/usr/bin/env python
import re
import math
import argparse

#######################################################################
# PROCESS GCODE FILE ##################################################
#######################################################################

def read_file(read_path: str) -> list:
    try:
        with open(read_path, 'r') as file:
            return file.readlines()

    except FileNotFoundError:
        print(f"The file {read_path} was not found.")

    except Exception as e:
        print(f"An error occurred: {e}")


def write_file(write_path: str, content: list) -> None:
    try:
        with open(write_path, 'w') as file:
            file.write(''.join(content))

    except FileNotFoundError:
        print(f"The file {write_path} was not found.")

    except Exception as e:
        print(f"An error occurred: {e}")


def add_pruge_macro(content: list, macro: str) -> list:
    """ Find the line with G21 or G90 and insert the purge macro before it.
    :return: The modified gcode file with the purge macro inserted. """

    for idx, line in enumerate(content, 1):
        if line.startswith('G90') or line.startswith('G21'):
        # if mark_line in line:
            idx -= 1
            content.insert(idx, macro)
            return content


def generate_purge_gcode(firstPoint: tuple, point_1: tuple, point_2: tuple) -> str:

    # calculate move speed
    purge_move_speed: float = round(((args.flow_rate / 5.0) * 60), 1)

    # Check if printer use firmware retraction
    if use_firmware_retraction is True:
        retract: str = 'G10'
        unRetract: str = 'G11'
    else:
        retract: str = "G1 E-0.8 F2100"
        unRetract: str = 'G1 E0.8 F2100'

    # Generate Purge Gcode
    purge_gcode =   (   f'\n; Adaptive Purge START\n'
                        f'M118 [ADAPTIVE PURGE] start from {point_2} to {point_1}, with flow rate {args.flow_rate}mm3/s.\n'
                        f'G92 E0 ; Reset extruder\n'
                        f'G90 ; Absolute positioning\n'
                        f'G0 X{point_2[0]} Y{point_2[1]} F{travel_speed} ; Move to purge position\n'
                        f'G0 Z{args.purge_height} ; Move to purge Z height\n'
                        f'M83 ; Relative extrusion mode\n'
                        f'G1 E{args.tip_distance} F{purge_move_speed} ; Move filament tip\n'
                        f'G1 X{point_1[0]} Y{point_1[1]} E{args.purge_amount} F{purge_move_speed} ; Purge line\n'
                        f'{retract} ; Retract\n'
                        f'G0 X{firstPoint[0]} Y{firstPoint[1]} F{travel_speed} ; Rapid move to first print point to break stringing\n'
                        f'{unRetract} ; Un-retract\n'
                        f'G92 E0 ; Reset extruder distance\n'
                        f'; Adaptive Purge END\n\n'
                    )
    
    if args.verbose is True:
        print (purge_gcode)

    return purge_gcode


#######################################################################
# OBJECT POLYGON ######################################################
#######################################################################

class objectPolygon:
    def __init__(self, vertices):
        self.vertices = vertices

    def contains_point(self, point: tuple) -> bool:
        """ Determine if a point is inside a polygon using the ray-casting algorithm
        :return: True if the point is inside the polygon or on the edge, otherwise False. """
    
        x, y = point
        n = len(self.vertices)
        inside: bool = False

        p1x, p1y = self.vertices[0]

        for i in range(n + 1):
            p2x, p2y = self.vertices[i % n]

            # Check if the point is on the edge
            if (p1x - p2x) * (y - p1y) == (p1y - p2y) * (x - p1x) and \
            min(p1x, p2x) <= x <= max(p1x, p2x) and \
            min(p1y, p2y) <= y <= max(p1y, p2y):
                return True  # Point is on the edge
            
            # Ray-Casting algorithm
            if min(p1y, p2y) < y <= max(p1y, p2y) and x <= max(p1x, p2x):
                    if p1y != p2y:
                        xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                    if p1x == p2x or x <= xinters:
                        inside = not inside
            p1x, p1y = p2x, p2y

        return inside
    
    
    def closest_point_to_polygon_edge(self, pointX: tuple) -> tuple[tuple, tuple[tuple, tuple], float]:
        """ Find the closest point on the edges of the polygon to point X. 
        :return: The closest point, closest edge, and the distance. """

        closest_point: tuple = None
        min_distance = float('inf')

        n = len(self.vertices)
        for i in range(n):
            point1 = self.vertices[i]
            point2 = self.vertices[(i + 1) % n]  # Wrap around to the first point
            closest_on_segment = objectPolygon.get_closest_point(pointX, point1, point2)
            dist = math.sqrt((pointX[0] - closest_on_segment[0]) ** 2 + (pointX[1] - closest_on_segment[1]) ** 2)
            
            # if distance of point to edge is too small,
            # point A must be on the edge of polygon
            if dist < 1e-9:
                closest_point = pointX
                closest_edge = (point1, point2)
                break

            if dist < min_distance:
                min_distance = dist
                closest_point = closest_on_segment
                closest_edge = (point1, point2)
        
        return closest_point, closest_edge, min_distance
    

    def get_closest_point(pointX: tuple, point1: tuple, point2: tuple) -> tuple:
        """ Find the closest point from point point X
        to the line segment defined by point1 and point2. """
        
        # Vector AB
        AB = (point2[0] - point1[0], point2[1] - point1[1])
        # Vector AP
        AP = (pointX[0] - point1[0], pointX[1] - point1[1])
        
        # Project point A onto the line defined by point1 and point2
        AB_length = AB[0] ** 2 + AB[1] ** 2
        if AB_length == 0:  # point1 and point2 are the same point
            return point1
        
        # Projection factor
        t = (AP[0] * AB[0] + AP[1] * AB[1]) / AB_length
        # Clamp t to the range [0, 1] to stay within the segment
        t = max(0, min(1, t))
        
        # Calculate and return the closest point on the segment
        return (point1[0] + t * AB[0], point1[1] + t * AB[1])
    

    def point_along_edge(self, pointX: tuple, distance: int) -> tuple:
        """ Find a point certain distance away from A along closest edge of the polygon """

        # Find closest edge of the bed to the point A
        edge = self.closest_point_to_polygon_edge(pointX)[1]

        # Calculate the direction vector of the edge line
        direction_vector = (edge[1][0] - edge[0][0], edge[1][1] - edge[0][1])

        # Normalize the direction vector
        unit_vector = normalize(direction_vector)

        # Calculate point D along the edge line
        return  (pointX[0] + distance * unit_vector[0], pointX[1] + distance * unit_vector[1])


#######################################################################
# GET PRINT INFO FROM GCODE ###########################################
#######################################################################

def check_slicer(content: list) -> str:
    """ Check the slicer used to generate the G-code file
    :return: The name of the slicer used to generate the G-code file. """

    slicer = None
    for line in content:
        if 'generated by PrusaSlicer' in line:
            slicer = 'PrusaSlicer'
            break
        
        elif 'generated by OrcaSlicer' in line:
            slicer = 'OrcaSlicer'
            break
        
        # Stop reading the file if reaching the end of first block to avoid reading the whole file
        elif ';TYPE:Custom' in line:
            break
        
    return slicer


def get_print_info(content: list, slicer: str) -> object:
    """ Read gcode file to find all the print related info
    :return: Bed Polygon only, other parameters will be save as global variable """
    
    global combined_skirt
    # found_params = 0
    skirt_params = set()

    # Read file in reverse to find config block first
    # so script don't have to go through all the line
    n = len(content)
    while True:
        n -= 1
        line = content[n]

        # Different route for each slicer
        match slicer:
            case 'PrusaSlicer':
                # exit line
                if 'prusaslicer_config = begin' in line or n <= 0:
                    break

                # check skirt for PrusaSlicer
                if line.startswith('; skirts ') and int(line.split(' = ')[1]) > 0:
                    combined_skirt = True                

            case 'OrcaSlicer':
                # exit line
                if 'CONFIG_BLOCK_START' in line or n <= 0:
                    break

                if len(skirt_params) < 2:
                    # get skirt type used in gcode
                    if line.startswith('; skirt_type '):
                        skirt_params.add(line.split(' = ')[1])

                    # get how many skirt loops used in gcode
                    elif line.startswith('; skirt_loops '):
                        skirt_params.add(int(line.split(' = ')[1]))
                
                elif combined_skirt is False:
                    if 'combined' in skirt_params and 0 not in skirt_params:
                        combined_skirt = True
        
        if line.startswith("; travel_speed "):
            global travel_speed
            travel_speed = float(line.split(' = ')[1])
            continue

        if line.startswith('; use_firmware_retraction '):
            if int(line.split(' = ')[1]) == 1:
                global use_firmware_retraction
                use_firmware_retraction = True
            continue
        
        # get Bed Polygon that defined in slicer
        if line.startswith("; bed_shape "):
            bed_data = line.split(' = ')[1]
            # Split the string by commas to get individual points
            bedPoints = bed_data.split(',')

            bed_data: list = []

            for point in bedPoints:
                # Split by 'x' and convert to integers
                x, y = map(int, point.split('x'))
                bed_data.append((x, y))

            bed_polygon = objectPolygon(bed_data)
            continue

    return bed_polygon

def get_first_point(content: list) -> tuple:
    """ Read gcode file to find the point that the print begin
    :return: The first point of the print object. """

    # flag
    recording: bool = False

    for line in content:
        # start after "LAYER_CHANGE" line to avoid any G1 command at "print start G-code"
        if line.startswith(';LAYER_CHANGE'):
            recording = True
            continue

        if line.startswith('G1') and recording is True:
            # Use regex to find X and Y values
            x_match = re.search(r'X([-+]?\d*\.\d+|\d+)', line)
            y_match = re.search(r'Y([-+]?\d*\.\d+|\d+)', line)

            # Extract the values if they exist
            x_value = float(x_match.group(1)) if x_match else None
            y_value = float(y_match.group(1)) if y_match else None

            if x_value is not None and y_value is not None:
                # get values of firt print point and ignore all the G1 command after this
                return (x_value, y_value)


def get_print_zone_points(content: list) -> list:
    """ Find all the points on first layer that when combine make up the print object."""

    # flag
    first_layer: bool = False
    recording: bool = False

    # Parameters
    collect_threshold: int = 4
    collectable_types = (   'TYPE:External perimeter', 
                            'TYPE:Support', 
                            'TYPE:Brim', 
                            'TYPE:Skirt', 
                            'TYPE:Outer wall' )

    first_layer_points: list = []
    object_points: list = []


    for line in content:
        # mark when first layer begin and end
        if line.startswith(';LAYER_CHANGE'):
            if first_layer is False:
                first_layer = True
            
            # return all the points when reach the end of first layer
            else:
                # if collecting point greater than minimum threshold
                # append all the point to object_points
                if len(first_layer_points) > collect_threshold:
                    object_points.append(tuple(first_layer_points))

                return object_points
        
        elif first_layer is True:
            if recording is True:
                if line.startswith('G1'):
                    # Use regex to find X and Y values
                    x_match = re.search(r'X([-+]?\d*\.\d+|\d+)', line)
                    y_match = re.search(r'Y([-+]?\d*\.\d+|\d+)', line)

                    # Extract the values if they exist
                    x_value = float(x_match.group(1)) if x_match else None
                    y_value = float(y_match.group(1)) if y_match else None

                    if x_value is not None and y_value is not None:
                        first_layer_points.append((x_value, y_value))

                # when change to another type
                # 'M486' is PrusaSlicer object change command
                elif line.startswith(";") or line.startswith('M486'):
                    # if collecting point greater than minimum threshold
                    # append all the point to object_points
                    if len(first_layer_points) > collect_threshold:
                        object_points.append(tuple(first_layer_points))
                    
                    # and clear the list
                    first_layer_points.clear()

                    # if new type is not the collect able type -> stop collecting
                    if "TYPE" in line and not any(type in line for type in collectable_types):
                        recording = False
            
            # start collecting point when reach the collectable type
            if any(type in line for type in collectable_types):
                recording = True



def get_polygon_from_first_layer(content: list) -> list[object]:
    """ Find all the print object from first layer
    :return: A list of Polygon objects. """

    polygons = []

    # get all object points from first layer
    object_points = get_print_zone_points(content)

    # if used combined skirt, only keep the first object (the skirt that cover all)
    if combined_skirt:
        del object_points[1:]

    # convert all the points to polygon objects
    for i in range(len(object_points)):
            polygon = make_polygon(object_points[i])
            polygons.append(objectPolygon(polygon))

    # Remove any nested polygons
    polygons = remove_nested_polygons(polygons)

    return polygons


#######################################################################
# CACULATE FUNCTIONS ##################################################
#######################################################################

def make_polygon(points: tuple) -> list:
    """ Take a list of points, make a polygon cover all them
    :return: A list vertices that make a polygon. """

    # Sort the points lexicographically (by x, then by y)
    points = sorted(points)

    # Build the lower hull
    lower = []
    for p in points:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)

    # Build the upper hull
    upper = []
    for p in reversed(points):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)

    # Remove the last point of each half because it's repeated at the beginning of the other half
    return lower[:-1] + upper[:-1]


def cross(o, a, b):
    """ Cross product of vector OA and OB.
    A positive cross product indicates a counter-clockwise turn,
    a negative cross product indicates a clockwise turn,
    and a zero cross product indicates a collinear point. """

    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])


def remove_nested_polygons(polygons: list[object]) -> list[object]:
    """ Remove nested polygons from the list.
    :return: A list of non-nested polygons. """

    non_nested_polygons = []

    for i in range(len(polygons)):
        nested: bool = False
        for j in range(len(polygons)):
            # take one point from polygon i to check if it is inside polygon j
            # if it is, polygon i is nested inside polygon j
            if i != j and all(polygons[j].contains_point(vertex) for vertex in polygons[i].vertices):
                nested = True
                break
        
        if not nested:
            non_nested_polygons.append(polygons[i])
    
    return non_nested_polygons


def cal_distance(point1, point2) -> float:
    """ Calculate the Euclidean distance between two points. """

    return math.sqrt((point1[0] - point2[0]) ** 2 + (point1[1] - point2[1]) ** 2)


def normalize(vector: tuple) -> tuple:
    """ Normalize a vector to unit length. """

    length: float = cal_distance((0, 0), vector)

    # Return a zero vector if the length is zero
    if length == 0:
        return (0, 0)
    
    return (vector[0] / length, vector[1] / length)


def find_point_opposite_side(pointA: tuple, pointB: tuple, distance: int) -> tuple[float, float]:
    """ Find point X that is a certain distance from point A
    in the opposite direction of point B. """
    
    # Calculate the direction vector from A to B
    direction_vector = (pointB[0] - pointA[0], pointB[1] - pointA[1])
    
    # Normalize the direction vector
    normalized_vector = normalize(direction_vector)
    
    # Scale the normalized vector to the desired distance
    scaled_vector = (normalized_vector[0] * distance, normalized_vector[1] * distance)
    
    # Calculate point X by moving in the opposite direction
    return round(pointA[0] - scaled_vector[0], 3), round(pointA[1] - scaled_vector[1], 3)


def find_point_perpendicular(point_A: tuple, pointX: tuple, pointY: tuple, distance: int) -> tuple:
    """ Find point X that is a certain distance from point A (that on the line XY)
    in the perpendicular direction to the line XY. """

    # Calculate the direction vector of line XY
    direction_vector = (pointX[0] - pointY[0], pointX[1] - pointY[1])
    
    # Normalize the direction vector
    length = normalize(direction_vector)

    # The perpendicular direction
    perpendicular_dx = -length[1]
    perpendicular_dy = length[0]
    
    # Scale the normalized vector to the desired distance
    scaled_vector = (perpendicular_dx * distance, perpendicular_dy * distance)
    
    # Calculate point X by moving in the opposite direction
    return round(point_A[0] + scaled_vector[0], 3), round(point_A[1] + scaled_vector[1], 3)


def purge_line_on_edge(pointA: tuple, bed_polygon: objectPolygon) -> tuple[tuple, tuple]:
    """ Purge line is choose from bed corner or the mid point of the edge depending on which point is closest 
    
    :return: Purge-Line start, end points. """

    bed: list = bed_polygon.vertices

    # get all the corners and mid-point of bed edge
    points_can_purge = []
    
    n = len(bed)
    for i in range(n):
        if bed_polygon[i] not in points_can_purge:
            # Add corners
            points_can_purge.append(bed[i])

            # Calculate the midpoint of the edge between current and next vertex
            next_index = (i + 1) % n
            midpoint = ((bed[i][0] + bed[next_index][0]) / 2, (bed[i][1] + bed[next_index][1]) / 2)
            
            # Add midpoint of the edge
            points_can_purge.append(midpoint)

    # point X is on bed edge closet to pointA
    pointX = bed_polygon.closest_point_to_polygon_edge(pointA)[0]

    # Find the closest point to X in all the point can be purge.
    closest_point: tuple = None
    min_distance = float('inf')

    for point in points_can_purge:
        distance = cal_distance(pointX, point)
        if distance < min_distance:
            min_distance = distance
            closest_point = point
    
    # Find the other point of Purge-Line
    pointY = find_point_opposite_side(closest_point, pointX, args.purge_length)

    # if pointY inside bed (closest_point must be mid-point of the edge)
    # Purge-Line is from pointY -> closest_point
    if bed_polygon.contains_point(pointY):
        return closest_point, pointY
    # otherwise, revert Purge-Line direction
    else:
        pointY = find_point_opposite_side(closest_point, pointY, args.purge_length)
        return pointY, closest_point


def find_end_point(firstPoint: tuple, polygon: objectPolygon) -> tuple[tuple, tuple]:
    """ Find the end point of the Purge-Line by projecting a point from the first point to the polygon edge.
    :return: The end-point of the Purge-Line, and the point closest to the first point on the polygon edge """

    # find a point on polygon edge that closest to firstPoint
    point_0, closest_edge = polygon.closest_point_to_polygon_edge(firstPoint)[0:2]

    # if firstPoint is on the edge of object polygon
    if point_0 == firstPoint:
        if args.verbose is True:
            print ('First point on edge polygon')
        
        point_1 = find_point_perpendicular(point_0, closest_edge[0], closest_edge[1], args.purge_margin)

    # if firstPoint is inside object
    elif polygon.contains_point(firstPoint):
        if args.verbose is True:
            print ('First point inside polygon')
        
        point_1 = find_point_opposite_side(point_0, firstPoint, args.purge_margin)
    
    # if firstPoint is outside object
    else:
        if args.verbose is True:
            print('First point is outside polygon')
        
        point_1 = find_point_opposite_side(firstPoint, point_0, args.purge_margin)

    return point_0, point_1


def convert_all_polygons_to_one(polygons: list[object]) -> object:
    """ get all the points from polygons and convert them to one polygon """

    all_points = []

    for polygon in polygons:
        all_points.extend(polygon.vertices)

    return objectPolygon(make_polygon(all_points))


#######################################################################
# MAIN FUNCTION #######################################################
#######################################################################

def process_file(input_f: str) -> None:
    """ Main function to process the G-code file """

    # read input file
    content = read_file(input_f)

    # check slicer used to generate the file (PrusaSlicer or OrcaSlicer)
    slicer = check_slicer(content)

    # Find all print related infos
    bed_polygon: objectPolygon = get_print_info(content, slicer)

    firstPoint = get_first_point(content)

    polygons: list[objectPolygon] = get_polygon_from_first_layer(content)


    # output print-infos
    if args.verbose is True:
        print(f'First print Point is: {firstPoint}')
        for i in range(len(polygons)):
            print(f'Print Object-{i+1} is: {polygons[i].vertices}')
        print(f'Max travel speed is: {travel_speed}')
        print(f'Bed polygon is: {bed_polygon.vertices}')


    # Find point_1 (aka. Purge-Line end point)
    point_0, point_1 = find_end_point(firstPoint, polygons[0])

    # Check point_1 is inside or too close to another print objects
    if len(polygons) > 1:
        for i in range(1, len(polygons)):
            # if point_in_polygon(point_1, polygons[i]):
            if polygons[i].contains_point(point_1) or polygons[i].closest_point_to_polygon_edge(point_1)[2] < args.purge_margin:
                # If point_1 is inside another object, that mean object is too close to each other
                # Convert all polygons to one polygon, and use that polygon to find new point_1
                polygons = [convert_all_polygons_to_one(polygons)]
                # add new polygon to another list to make it compatible with function check point_2 bellow
                point_0, point_1 = find_end_point(firstPoint, polygons[0])
                break

    # Find point_2 (aka. Purge-Line start point)
    # Check if first point inside print bed
    if bed_polygon.contains_point(point_1):
        # other point of Pure-Line is calculate by offset by purge_length from point_1
        point_2 = find_point_opposite_side(point_1, point_0, args.purge_length)

        # if point_2 is not inside print bed
        if not bed_polygon.contains_point(point_2):
            # re-caculate point_2
            # point_2 = point_along_edge(point_1, bed_polygon, args.purge_length)
            point_2 = bed_polygon.point_along_edge(point_1, args.purge_length)

        # if print multiple objects, check if point_2 is inside or too close to another print objects
        if len(polygons) > 1:
            for i in range(1, len(polygons)):
                if polygons[i].contains_point(point_2) or polygons[i].closest_point_to_polygon_edge(point_2)[2] < args.purge_margin:
                    # re-caculate point_2
                    point_2 = polygons[i].point_along_edge(point_1, args.purge_length)
    
    # if point_1 outside of the bed, Purge-Line will be put on bed edge
    else:
        point_1, point_2 = purge_line_on_edge(point_1, bed_polygon)


    if args.verbose is True:
        print(f'Purge-Line is from {point_2} to {point_1} and first point is {firstPoint}')

    # Generate Purge gcode
    purge_gcode = generate_purge_gcode(firstPoint, point_1, point_2)

    # Insert Purge g-code into .gcode file and save it
    modified_content = add_pruge_macro(content, purge_gcode)
    write_file(input_f, modified_content)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Post-process G-code generating Purge-line near your actual printed objects.")
    parser.add_argument("input_file", help="Path to the input G-code file")

    # Add argument as Config Variables
    parser.add_argument("-verbose", type=bool, default=False, help="Output info, (default: False)")
    parser.add_argument("-purge_height", type=float, default=0.5, help="Z position (in mm) of nozzle during purge, (default: 0.5mm)")
    parser.add_argument("-tip_distance", type=float, default=1.0, help="Distance between tip of filament and nozzle before purge. Should be similar to PRINT_END final retract amount.")
    parser.add_argument("-purge_margin", type=int, default=5, help="Distance the purge will be in front of the print area, (default: 5mm)")
    parser.add_argument("-purge_length", type=int, default=10, help="The length of purge line, (default: 10mm)")
    parser.add_argument("-purge_amount", type=int, default=10, help="Amount of filament to be purged prior to printing, (default: 10mm)")
    parser.add_argument("-flow_rate", type=int, default=12, help="Flow rate of purge in mm3/s, (default: 12)")

    args = parser.parse_args()

    # global
    use_firmware_retraction: bool = False
    travel_speed: float = 300
    combined_skirt: bool = False
    
    # Call the main processing function
    process_file(args.input_file)
