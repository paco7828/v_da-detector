// Enable debug features (set to 1 to activate debugging output)
#define DEBUG 1

// Define board version
#define BOARD_VERSION "b1"

// Define firmware version
#define FW_VERSION "1.5"

// Enable or disable specific modules (set to 1 to enable, 0 to disable)
#define ENABLE_STATUSLED 1 // Enables the status LED functionality
#define ENABLE_ALERTLED 1  // Enables the alert LED functionality

// GPIO pin configuration

// Status LED pin (connected to an OLED display)
#define CFG_LED_STATUS 5 // GPIO5 (D1)

// Alert LED pins
#define CFG_LED_ALERT1 12 // GPIO12 (D6) - Left LED
#define CFG_LED_ALERT2 14 // GPIO14 (D5) - Right LED

// Beeper pin (used for sound alerts, also connected to an OLED display)
#define CFG_BEEPER 13 // GPIO13

// Debug button pin (used for triggering debug functions)
#define CFG_BTN 0 // GPIO0 (Boot button)

// Defines the allowable heading tolerance in degrees when tracking a point of interest (POI)
#define CFG_HDG_TOLERANCE 45

// Custom event definitions using EventManager

// Event triggered when GPS reception status changes
#define GPS_STATUS_CHANGED EventManager::kEventUser1

// Event triggered when a new GPS message is received
#define GPS_MESSAGE_RECEIVED EventManager::kEventUser2

// Event triggered when GPS coordinates are updated
#define GPS_UPDATED EventManager::kEventUser3

// Event triggered when an alert is raised
// The event parameter represents the difference between current speed and the speed limit
#define ALERT_TRIGGERED EventManager::kEventUser4

// Event triggered when an alert condition is cleared
#define ALERT_RESET EventManager::kEventUser5
