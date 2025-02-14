#ifndef H_STATUSLED
#define H_STATUSLED

#include "config.h"
#include <EventManager.h>
#include <Ticker.h>
#include "gps.h"

/**
 * StatusLED class controls the LED behavior based on the GPS status.
 * It uses a singleton pattern to ensure only one instance manages the status LED.
 * The LED behavior can be customized based on GPS reception status (no reception, acquired, etc.).
 */
class StatusLED
{
public:
    // Get the singleton instance of StatusLED.
    static StatusLED *getInstance() { return StatusLED::instance; };

    /**
     * Initialize the StatusLED, register event handlers, and start the timer.
     * This is the entry point for setting up the status LED functionality.
     *
     * @param eventManager Pointer to the EventManager instance that handles events.
     */
    static void init(EventManager *eventManager)
    {
        instance = new StatusLED();                                                       // Create the StatusLED singleton instance.
        eventManager->addListener(GPS_STATUS_CHANGED, &StatusLED::statusChangedCallback); // Register event listener.
    };

    /**
     * Event handler callback for when the GPS status changes.
     * This updates the status of the LED based on the new GPS status.
     *
     * @param eventCode Event identifier.
     * @param eventParam The new GPS status to be applied.
     */
    static void statusChangedCallback(int eventCode, int eventParam) { getInstance()->setStatus(eventParam); };

    /**
     * Change the GPS status and trigger the corresponding LED behavior.
     *
     * @param state The new GPS status (e.g., 0 for no reception, 1 for reception).
     */
    void setStatus(byte state);

    /**
     * Callback for timer ticks. This is invoked periodically by the ticker.
     * It handles LED animations based on the current GPS status.
     */
    static void tickCallback() { getInstance()->tick(); };

protected:
    // Constructor (protected to prevent direct instantiation)
    StatusLED();

    // Singleton instance of StatusLED
    static StatusLED *instance;

    // Current GPS status (initialized to -1 indicating an unknown state)
    byte gpsStatus = -1;

    // Ticker instance for triggering periodic timer callbacks
    Ticker ticker;

    /**
     * Handle periodic timer ticks. This function controls the LED behavior
     * based on the GPS status (animations or pings).
     */
    void tick();

    // Position index in the LED animation sequence
    word pos = 0;

    /**
     * Map for the pulse animation. This defines the LED brightness at each step
     * of the animation.
     */
    const uint16_t animPulse[25] = {1023, 980, 900, 800, 600, 400, 200, 0, 200, 400, 600, 800, 900, 1000,
                                    1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023};
};

#endif
