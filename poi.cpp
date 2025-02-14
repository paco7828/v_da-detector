#include "poi.h"

/**
 * Check if a given geographic point (latitude, longitude) is inside the polygon.
 * This function uses the ray-casting algorithm to determine whether the point
 * is inside or outside the polygon.
 *
 * @param lat Latitude of the point to check.
 * @param lng Longitude of the point to check.
 * @return true if the point is inside the polygon, false otherwise.
 */
boolean POI::checkPointInside(float lat, float lng)
{
    EDGE *edge;        // Pointer to the current polygon edge being checked.
    byte oddNodes = 0; // Toggle flag for detecting if point is inside (odd crossings).
    byte i;            // Loop counter.

    // Iterate through all edges of the polygon.
    for (i = 0; i < edgeCount; i++)
    {
        edge = &edges[i]; // Get the current edge.

        /**
         * Check if the horizontal ray from (lat, lng) crosses this edge.
         * A crossing occurs if the point's longitude (lng) is between the two
         * longitudes of the edge's endpoints (lng1 and lng2).
         */
        if ((edge->lng1 < lng && lng <= edge->lng2) || (edge->lng2 < lng && lng <= edge->lng1))
        {
            /**
             * Compute the intersection point of the edge with the horizontal ray.
             * Equation of the edge: lat = m * lng + c
             * If the computed latitude of the intersection is below the given point,
             * toggle the oddNodes flag.
             */
            oddNodes ^= (lng * edge->m + edge->c) < lat;
        }
    }

    // If the number of crossings is odd, the point is inside; otherwise, it's outside.
    return oddNodes;
}
