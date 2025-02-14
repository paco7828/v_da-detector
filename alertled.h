#ifndef H_ALERTLED
#define H_ALERTLED

#include "config.h"
#include <EventManager.h>
#include <Ticker.h>
#include "gps.h"

// Alert levels indicating different severity of alerts
#define ALERT_NONE -1   // No alert
#define ALERT_INFO1 0   // Low-priority info alert
#define ALERT_INFO2 1   // Medium-priority info alert
#define ALERT_INFO3 2   // High-priority info alert
#define ALERT_WARNING 3 // Warning level alert
#define ALERT_DANGER 4  // Critical danger alert

/**
 * AlertLED class manages LED and beeper signals to indicate proximity alerts.
 * It uses two LEDs (left and right) to display different alert patterns
 * based on proximity to a point of interest (POI).
 */
class AlertLED
{
public:
    /**
     * Initializes the alert LED system.
     * Registers event handlers and starts the animation timer.
     * @param eventManager Pointer to the EventManager instance for handling events.
     */
    static void init(EventManager *eventManager);

    /**
     * Advances the animation sequence for alert indicators.
     * Adjusts LED brightness and beeper intensity based on alert level.
     */
    static void tickCallback()
    {
        ALED_CFG anim;
        float mult = GPS::isNight() ? 0.05 : 1; // Dim LEDs at night

        if (alertStatus == ALERT_NONE)
        {
            // If no alert is active, turn LEDs and beeper off
            if (!animPtr)
                return;
            animPtr = 0;
            analogWrite(CFG_LED_ALERT1, 1023);
            analogWrite(CFG_LED_ALERT2, 1023);
            analogWrite(CFG_BEEPER, 1023);
            return;
        }

        // Reset animation position if it reaches the end of the sequence
        if (animPtr >= sizeof(anim_warning[alertStatus]) / sizeof(ALED_CFG))
            animPtr = 0;

        // Retrieve the next animation frame
        memcpy_P(&anim, &anim_warning[alertStatus][animPtr], sizeof(ALED_CFG));

        // Adjust LED brightness
        analogWrite(CFG_LED_ALERT1, (anim.lft - 1023) * mult + 1023);
        analogWrite(CFG_LED_ALERT2, (anim.rgt - 1023) * mult + 1023);

        // Set beeper intensity
        analogWrite(CFG_BEEPER, anim.beeper);

        // Move to the next animation frame
        animPtr++;
    }

    /**
     * Handles animation when GPS reception status changes.
     */
    static void receptionAnimCallback();

    /**
     * Handles an incoming alert event.
     * Determines the severity level and adjusts the animation accordingly.
     */
    static void alertTriggeredCallback(int eventCode, int eventParam);

    /**
     * Resets the alert system.
     * Can be triggered by an event or when GPS status changes.
     */
    static void resetCallback(int eventCode, int eventParam);

protected:
    /**
     * Stores the current alert status level.
     */
    static int8_t alertStatus;

    /**
     * Tracks the current position in the animation sequence.
     */
    static uint8_t animPtr;

    /**
     * Timer managing LED and beeper update intervals.
     */
    static Ticker ticker;

    /**
     * Timer for handling GPS reception change animations.
     */
    static Ticker receptionTicker;

    /**
     * Counter tracking the position in the reception animation sequence.
     */
    static uint16_t receptionCounter;

    /**
     * Stores the current GPS reception status (0 = no signal, 1 = signal acquired).
     */
    static byte receptionStatus;
};

#endif
