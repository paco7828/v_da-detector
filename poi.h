#ifndef POI_H
#define POI_H

#include <Arduino.h>
#include "config.h"

/**
 * Structure to represent an edge of the polygon.
 * An edge is defined by two points: the start and end longitude,
 * as well as the slope (m) and intercept (c) of the line connecting them.
 */
struct EDGE
{
    float lng1; // Longitude of the first endpoint of the edge.
    float lng2; // Longitude of the second endpoint of the edge.
    float m;    // Slope of the edge (rate of change of latitude with respect to longitude).
    float c;    // Y-intercept of the edge's line equation.
};

/**
 * Class representing a Point of Interest (POI), which is a polygon defined by multiple edges.
 * The polygon can be used to check if a given point is inside or outside.
 */
class POI
{
public:
    uint16_t limit;     // Speed limit associated with this POI (optional).
    uint16_t edgeCount; // Number of edges that define the polygon for this POI.
    float heading;      // Heading limit for the POI (if less than 0, this value is not used).
    EDGE *edges;        // Pointer to an array of edges defining the polygon.

    /**
     * Check if a point (latitude, longitude) is inside the polygon defined by this POI.
     * Uses the ray-casting algorithm to determine if the point is inside.
     *
     * @param lat Latitude of the point to check.
     * @param lng Longitude of the point to check.
     * @return true if the point is inside the polygon, false otherwise.
     */
    boolean checkPointInside(float lat, float lng);
};

#endif
