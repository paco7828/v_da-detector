#include "gps.h"

// Global variables to store parsed GPS time and date
int ora;          // Stores the hour
String perc;      // Stores the minute
String masodperc; // Stores the second
String datum;     // Stores the formatted date

// Pointer to the incoming serial message buffer
String *GPS::message;

// Current GPS reception status: true if a valid signal is available
boolean GPS::reception;

// Stores the last known valid GPS state
GPSSTATUS GPS::last;

// Reference to the event manager for event handling
EventManager *GPS::eventManager;

/**
 * Parses incoming GPS messages.
 * This function is called when a complete message is available.
 * It verifies the checksum and determines the message type.
 *
 * @param eventCode Event identifier
 * @param eventParam Additional event parameter
 */
void GPS::handleMessage(int eventCode, int eventParam)
{
    if (!calculateChecksum()) // Validate the checksum before processing
    {
#ifdef DEBUG
        Serial.print("X ");
        Serial.print(*message);
        Serial.println("Checksum error");
#endif
        return;
    }

    if (message->startsWith("$GPRMC,")) // Check if the message is a Recommended Minimum Sentence (RMC)
    {
#ifdef DEBUG
        Serial.print("> ");
        Serial.println(*message);
#endif
        parseRMC(message); // Process the RMC message
    }
}

/**
 * Parses an RMC (Recommended Minimum Sentence) message.
 * Extracts time, status, latitude, longitude, speed, heading, and date.
 *
 * @param msg Pointer to the GPS message string
 */
void GPS::parseRMC(String *msg)
{
    byte offset = 7;  // Initial offset in the message string
    byte fieldNr = 0; // Tracks the current field being processed
    String current;
    boolean old;

    float rawCoord, rawSpd, rawHdg;
    char rawIndicator;

    while (fieldNr < 9) // Process only the first 9 fields of the RMC message
    {
        current = msg->substring(offset);
        switch (fieldNr)
        {
        case 0: // UTC Time (hhmmss)
            last.time = current.substring(0, 2).toInt() * 60 + current.substring(2, 4).toInt();

            //******************* OLED display update *************************
            ora = current.substring(0, 2).toInt() + 1; // Adjust hour (timezone offset)
            if (ora == 24)
                ora = 0; // Reset to 0 if it exceeds 23 hours
            perc = current.substring(2, 4);
            masodperc = current.substring(4, 6);
            //*****************************************************************
            break;

        case 1: // GPS Status (A = Active, V = Void)
            old = reception;
            if (current.charAt(0) == 'A')
            {
                reception = true;
                last.timestamp = millis(); // Update timestamp of last valid reception
            }
            else if (current.charAt(0) == 'V')
            {
                reception = false;
            }
            if (old != reception)
            {
                eventManager->queueEvent(GPS_STATUS_CHANGED, reception); // Notify if status changes
            }
            break;

        case 2: // Latitude (raw coordinate value)
        case 4: // Longitude (raw coordinate value)
            rawCoord = current.toFloat();
            break;

        case 3:            // N/S Indicator
        case 5:            // E/W Indicator
            if (!rawCoord) // Skip processing if no valid coordinate exists
                break;
            rawIndicator = current.charAt(0);

            if (fieldNr == 3)
            {
                last.lat = toCoordinates(rawCoord, rawIndicator); // Convert latitude
            }
            else
            {
                last.lng = toCoordinates(rawCoord, rawIndicator); // Convert longitude
            }
            break;

        case 6: // Speed Over Ground (in knots, converted to km/h)
            if (current.indexOf(',') == 0)
            {
                last.spd = -1; // Invalid speed
                break;
            }
            last.spd = current.toFloat() * 1.852; // Convert knots to km/h
            break;

        case 7: // Course Over Ground (Heading in degrees)
            if (current.indexOf(',') == 0)
            {
                last.hdg = -1; // Invalid heading
                break;
            }
            last.hdg = current.toFloat();
            break;

        case 8: // Date (DDMMYY format)
            last.day = current.substring(0, 2).toInt() + (current.substring(2, 4).toInt() - 1) * 30;
            datum = current.substring(4, 6) + current.substring(2, 4) + current.substring(0, 2); // Convert to YYMMDD
            break;
        }

        offset = msg->indexOf(',', offset) + 1; // Move to the next field
        fieldNr++;
    }

    if (reception)
    {
        eventManager->queueEvent(GPS_UPDATED, 0); // Notify that GPS data has been updated
    }

#ifdef DEBUG
    Serial.print("Lat: ");
    Serial.println(last.lat, 6);
    Serial.print("Lng: ");
    Serial.println(last.lng, 6);
    Serial.print("SPD: ");
    Serial.println(last.spd);
    Serial.print("HDG: ");
    Serial.println(last.hdg);
    Serial.print("Is night: ");
    Serial.println(isNight());
    Serial.print("Time: ");
    Serial.print(ora);
    Serial.print(":");
    Serial.print(perc);
    Serial.print(":");
    Serial.println(masodperc);
    Serial.print("Last day: ");
    Serial.println(last.day);
    Serial.print("Date: ");
    Serial.println(datum);
#endif
}

/**
 * Determines if it is currently nighttime based on GPS data.
 * Uses a predefined sunrise/sunset map.
 *
 * @return boolean True if it's nighttime, false otherwise.
 */
boolean GPS::isNight()
{
    SUNINFO today;
    if (last.day < 0 || last.day > 359)
        return false;                                             // Invalid day
    memcpy_P(&today, &sunrisemap[last.day], sizeof(SUNINFO));     // Retrieve sunrise/sunset times
    return last.time < today.sunrise || today.sunset < last.time; // Check if current time is outside daylight range
}

/**
 * Converts NMEA coordinates from "ddmm.mmmm" format to decimal degrees.
 *
 * @param coord Raw coordinate value from NMEA sentence.
 * @param indicator Directional indicator ('N', 'S', 'E', 'W').
 * @return Converted decimal coordinate.
 */
float GPS::toCoordinates(float coord, char indicator)
{
    byte deg = coord / 100;          // Extract degrees
    float min = coord - (deg * 100); // Extract minutes

    float dec = deg + min / 60; // Convert to decimal degrees

    if (indicator == 'W' || indicator == 'S') // Apply negative sign for western/southern hemisphere
    {
        dec = -dec;
    }

    return dec;
}

/**
 * Computes the checksum for the current NMEA message.
 * Ensures data integrity by comparing calculated and provided checksums.
 *
 * @return 1 if checksum is valid, 0 otherwise.
 */
byte GPS::calculateChecksum()
{
    byte i, last;
    uint16_t checksum = 0;
    String strChecksum;

    last = message->lastIndexOf('*'); // Locate checksum delimiter
    if (last < 0)
        return 0;

    // Compute XOR checksum of all characters between '$' and '*'
    for (i = 1; i < last; i++)
    {
        checksum = checksum ^ message->charAt(i);
    }

    strChecksum = String(checksum, HEX);
    strChecksum.toUpperCase();

    if (checksum < 16) // Ensure two-digit formatting
    {
        strChecksum = "0" + strChecksum;
    }

    return message->substring(last + 1, last + 3) == strChecksum; // Compare computed and received checksum
}
