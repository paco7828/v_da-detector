#ifndef H_GPS
#define H_GPS

#include <Arduino.h>
#include "config.h"
#include <EventManager.h>

struct GPSSTATUS {
    float lat;       // Latitude
    float lng;       // Longitude
    float hdg;       // Heading
    float spd;       // Speed
    unsigned long timestamp; // Timestamp of last valid GPS data
    unsigned int time; // Current time of day in minutes
    unsigned int day;  // Current day of the year
};

struct SUNINFO {
    uint16_t sunrise;
    uint16_t sunset;
};

class GPS {
public:
    /**
     * Register event handlers
     * @param em Event manager used for events
     * @param buff Pointer to message buffer
     */
    static void init(EventManager *em, String *buff) {
        eventManager = em;
        eventManager->addListener(GPS_MESSAGE_RECEIVED, &GPS::handleMessage);
        message = buff;
    }

    /**
     * Handle incoming GPS message
     */
    static void handleMessage(int eventCode, int eventParam);

    /**
     * Return the current GPS data
     */
    static GPSSTATUS *getCurrent() { return &last; }

    /**
     * Returns true if it's currently nighttime
     */
    static boolean isNight();

protected:
    static String *message;       // Reference to the message buffer
    static boolean reception;     // True if GPS reception is available
    static GPSSTATUS last;        // Latest GPS status data
    static EventManager *eventManager; // Reference to the event manager

    /**
     * Calculate the checksum of the NMEA sentence
     */
    static byte calculateChecksum();

    /**
     * Parse GPRMC messages
     */
    static void parseRMC(String *msg);

    /**
     * Convert NMEA coordinates to decimal representation
     * @param coord Coordinate in ddmm.mmmm format
     * @param inverse N/S/E/W indicator
     * @return Converted decimal coordinate
     */
    static float toCoordinates(float coord, char inverse);
};

// Sunrise and sunset times for different days of the year
const SUNINFO sunrisemap[360] PROGMEM = {
    {390, 904}, {390, 905}, {390, 906}, {390, 907}, {390, 908},
    {390, 909}, {390, 910}, {389, 911}, {389, 912}, {389, 913},
    // ... (data truncated for brevity)
    {390, 901}, {390, 902}
};

#endif
