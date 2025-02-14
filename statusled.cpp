#include "statusled.h"

/**
 * Singleton instance of the StatusLED class.
 * This ensures there is only one instance managing the status LED.
 */
StatusLED *StatusLED::instance;

/**
 * Constructor for the StatusLED class.
 * Initializes the ticker to call the tickCallback function every 100 milliseconds.
 */
StatusLED::StatusLED()
{
    ticker.attach_ms(100, &StatusLED::tickCallback); // Set up a ticker to call the tick method periodically.
}

/**
 * Set the GPS status and trigger the appropriate LED behavior.
 *
 * @param state The new GPS status (e.g., 0 for no reception, 1 for reception).
 * If the new status is the same as the current one, no action is taken.
 */
void StatusLED::setStatus(byte state)
{
    // If the new state is the same as the current state, do nothing.
    if (state == gpsStatus)
        return;

#ifdef DEBUG
    // Print the status change for debugging purposes.
    Serial.print("Setting status to ");
    Serial.print(state);
    Serial.print("\n");
#endif

    gpsStatus = state; // Update the current GPS status.
    pos = 0;           // Reset the animation position.
}

/**
 * Handle the periodic LED ticks, controlling the LED behavior based on GPS status.
 * This function is called periodically via the ticker.
 */
void StatusLED::tick()
{
    if (!gpsStatus) // No GPS reception
    {
        // Display a sweeping animation when there's no reception.
        analogWrite(CFG_LED_STATUS, animPulse[pos]);
        pos++;
        // Loop the animation when it reaches the end.
        if (pos >= sizeof(animPulse) / 2)
            pos = 0;
    }
    else // GPS reception acquired
    {
        // Display short pings when GPS reception is acquired.
        if (pos == 0)
        {
            // Use different LED brightness for night or day.
            if (GPS::isNight())
            {
                analogWrite(CFG_LED_STATUS, 900); // Nighttime brightness.
            }
            else
            {
                analogWrite(CFG_LED_STATUS, 1); // Daytime dim brightness.
            }
        }
        else
        {
            analogWrite(CFG_LED_STATUS, 1023); // Full brightness for active status.
        }

        // Reset the animation position after a certain number of ticks.
        if (++pos >= 150)
            pos = 0;
    }
}
