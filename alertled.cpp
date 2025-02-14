#include "alertled.h"

// Current alert status (e.g., NONE, INFO, WARNING, DANGER)
int8_t AlertLED::alertStatus;

// Position in the LED animation sequence
uint8_t AlertLED::animPtr;

// Ticker object for managing LED blinking timing
Ticker AlertLED::ticker;

// Ticker for handling GPS signal reception animation
Ticker AlertLED::receptionTicker;

// Counter tracking the position in the GPS reception animation
uint16_t AlertLED::receptionCounter = 0;

// Current GPS reception status (0 = no signal, 1 = signal acquired)
byte AlertLED::receptionStatus = 0;

/**
 * Initializes the alert LED system.
 * Sets up pins for LEDs and beeper, then registers event listeners.
 * Starts the ticker for periodic updates.
 */
void AlertLED::init(EventManager *eventManager)
{
    // Set LED and beeper pins as outputs
    pinMode(CFG_LED_ALERT1, OUTPUT);
    pinMode(CFG_LED_ALERT2, OUTPUT);
    pinMode(CFG_BEEPER, OUTPUT);

    // Turn off LEDs and beeper initially
    digitalWrite(CFG_LED_ALERT1, HIGH);
    digitalWrite(CFG_LED_ALERT2, HIGH);
    digitalWrite(CFG_BEEPER, HIGH);

    // Set initial alert status to none
    alertStatus = ALERT_NONE;

    // Register event listeners for alerts and GPS status changes
    eventManager->addListener(ALERT_TRIGGERED, &AlertLED::alertTriggeredCallback);
    eventManager->addListener(ALERT_RESET, &AlertLED::resetCallback);
    eventManager->addListener(GPS_STATUS_CHANGED, &AlertLED::resetCallback);

    // Start ticker with 50ms interval
    ticker.attach_ms(50, &AlertLED::tickCallback);
}

/**
 * Callback function triggered when an alert event occurs.
 * Determines the alert level based on event parameter and adjusts blink speed.
 */
void AlertLED::alertTriggeredCallback(int eventCode, int eventParam)
{
    Serial.print("Alert: ");
    Serial.println(eventParam);

    // Determine alert level based on event parameter
    if (eventParam <= -10)
    {
        alertStatus = ALERT_INFO1;
    }
    else if (eventParam < -5)
    {
        alertStatus = ALERT_INFO2;
    }
    else if (eventParam <= 0)
    {
        alertStatus = ALERT_INFO3;
    }
    else if (eventParam <= 15)
    {
        alertStatus = ALERT_WARNING;
    }
    else
    {
        alertStatus = ALERT_DANGER;
    }

    // Adjust ticker speed for warning alerts to create a speeding-up effect
    if (alertStatus == ALERT_WARNING)
    {
        ticker.attach_ms(50 - 3 * eventParam, &AlertLED::tickCallback);
    }
    else
    {
        ticker.attach_ms(50, &AlertLED::tickCallback);
    }
}

/**
 * Callback function triggered when an alert is reset or GPS status changes.
 * Resets alert state and handles GPS reception animations.
 */
void AlertLED::resetCallback(int eventCode, int eventParam)
{
    // Reset alert status if the event is not a GPS status change or GPS signal is lost
    if (eventCode != GPS_STATUS_CHANGED || eventParam == 0)
    {
        alertStatus = ALERT_NONE;
    }

    // Handle GPS reception status change
    if (eventCode == GPS_STATUS_CHANGED)
    {
        // If status hasn't changed, do nothing
        if (eventParam == receptionStatus)
            return;

        // Update reception status
        receptionStatus = eventParam;

        // Start GPS reception animation
        receptionCounter = receptionStatus ? 1 : 10;
        analogWriteFreq(receptionCounter * 88);
        analogWrite(CFG_BEEPER, 512);
        receptionTicker.attach_ms(100, &AlertLED::receptionAnimCallback);
    }
}

/**
 * Callback function for handling GPS reception change animation.
 * Adjusts beeper frequency to create an audible effect when GPS status changes.
 */
void AlertLED::receptionAnimCallback()
{
    // Adjust reception counter based on signal status
    if (receptionStatus)
    {
        receptionCounter++;
    }
    else
    {
        receptionCounter--;
    }

    // Update beeper frequency to create an audible effect
    analogWriteFreq(receptionCounter * 88);

    // Stop animation when reaching the target frequency
    if (receptionCounter == 10 || receptionCounter == 1)
    {
        receptionTicker.detach();
        analogWrite(CFG_BEEPER, 1023); // Turn off beeper
    }
}
