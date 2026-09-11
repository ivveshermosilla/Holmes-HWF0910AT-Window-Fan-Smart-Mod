#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* FW_NAME = "Holmes HWF0910AT Window Fan Smart Mod by Ivves";
static const char* FW_VERSION = "0.3.16-zc-stability";

static const uint8_t PIN_ZERO_CROSS = 1;
static const uint8_t PIN_H11_1 = 2;
static const uint8_t PIN_H11_2 = 42;
static const uint8_t PIN_H11_3 = 41;
static const uint8_t PIN_H11_4 = 40;
static const uint8_t PIN_LED_DATA = 21;
static const uint8_t PIN_TRIAC_TRIGGER = 47;
static const uint8_t PIN_TEMP = 39;
static const uint8_t PIN_BUTTON = 8;
static const uint8_t PIN_BOARD_LED = 48;

static const uint8_t LED_COUNT = 7;
static const uint32_t BUTTON_LONG_MS = 2500;
static const uint32_t BUTTON_DEBOUNCE_MS = 35;
static const uint32_t PULSE_SAMPLE_MS = 250;
static const uint32_t ZC_SAMPLE_MS = 1000;
static const uint32_t ZC_FRESH_US = 30000;
static const uint32_t AC_RECONNECT_GAP_US = 250000;
static const uint16_t FAN_START_BOOST_MS = 2000;
static const uint8_t FAN_CUSTOM_MIN_PERCENT = 85;
static const uint8_t FAN_CUSTOM_MAX_PERCENT = 100;
static const uint8_t FAN_CUSTOM_DEFAULT_PERCENT = 100;
static const uint8_t FAN_LOW_REFERENCE_PERCENT = 85;
static const uint8_t FAN_HIGH_REFERENCE_PERCENT = 100;
static const uint16_t DEFAULT_HIGH_DELAY_US = 300;
static const uint16_t DEFAULT_LOW_DELAY_US = 3200;
static const uint16_t DEFAULT_MOC_PULSE_US = 120;

static const char* AP_SSID_DEFAULT = "IvvesFan-Config";
static const char* AP_PASS_DEFAULT = "youarewelcome";
static const char* WEB_USER_DEFAULT = "thankyou";
static const char* WEB_PASS_DEFAULT = "youarewelcome";
static const char* ADMIN_USER_DEFAULT = "admin";
static const char* ADMIN_PASS_DEFAULT = "admin";
static const char* STA_SSID_DEFAULT = "";
static const char* STA_PASS_DEFAULT = "";
static const char* STA_IP_DEFAULT = "192.168.1.150";
static const char* STA_GATEWAY_DEFAULT = "192.168.1.1";
static const char* STA_SUBNET_DEFAULT = "255.255.255.0";
static const char* STA_DNS_DEFAULT = "8.8.8.8";
static const char* HOSTNAME_DEFAULT = "holmes-hwf0910at";
static const bool DEFAULT_BUTTON_ENABLED = true;
static const uint8_t CONFIG_SCHEMA_VERSION = 9;
static const uint8_t TEMP_HISTORY_DAYS = 7;
static const uint8_t TEMP_HISTORY_HOURS = 24;
static const int16_t TEMP_HISTORY_EMPTY = INT16_MIN;
static const uint16_t MINUTES_PER_DAY = 1440;
static const uint32_t POWER_LOG_RETENTION_SECONDS = 24UL * 60UL * 60UL;
static const uint8_t POWER_LOG_MAX_EVENTS = 64;
static const uint32_t POWER_LOG_MAGIC = 0x48574650UL;
static const uint8_t POWER_LOG_VERSION = 1;
static const uint32_t FIRE_GAP_WARNING_US = 12500;

enum ScheduleTempRule : uint8_t {
    SCHEDULE_TEMP_NONE = 0,
    SCHEDULE_TEMP_AND = 1,
    SCHEDULE_TEMP_OR = 2
};

enum ScheduleMode : uint8_t {
    SCHEDULE_OFF = 0,
    SCHEDULE_WEEKLY = 1,
    SCHEDULE_DAILY = 2
};

enum FanMode : uint8_t {
    MODE_OFF_LOOP = 0,
    MODE_HIGH_CONT,
    MODE_LOW_CONT,
    MODE_HIGH_60,
    MODE_HIGH_65,
    MODE_HIGH_70,
    MODE_HIGH_75,
    MODE_HIGH_80,
    MODE_LOW_60,
    MODE_LOW_65,
    MODE_LOW_70,
    MODE_LOW_75,
    MODE_LOW_80,
    MODE_CUSTOM,
    MODE_OFF_MEMORY
};

enum FanSpeed : uint8_t {
    SPEED_OFF = 0,
    SPEED_HIGH = 1,
    SPEED_LOW = 2,
    SPEED_CUSTOM = 3
};

enum PowerEventType : uint8_t {
    POWER_EVENT_BOOT = 0,
    POWER_EVENT_MOTOR_ON,
    POWER_EVENT_MOTOR_OFF,
    POWER_EVENT_AC_PRESENT,
    POWER_EVENT_AC_LOST,
    POWER_EVENT_APP_SESSION,
    POWER_EVENT_OUTPUT_GAP,
    POWER_EVENT_OUTPUT_BLOCKED,
    POWER_EVENT_OUTPUT_RESTORED,
    POWER_EVENT_SYSTEM
};

enum PowerEventCause : uint8_t {
    POWER_CAUSE_INTERNAL = 0,
    POWER_CAUSE_RESET_POWER_ON,
    POWER_CAUSE_RESET_SOFTWARE,
    POWER_CAUSE_RESET_PANIC,
    POWER_CAUSE_RESET_WATCHDOG,
    POWER_CAUSE_RESET_BROWNOUT,
    POWER_CAUSE_RESET_OTHER,
    POWER_CAUSE_BOOT_SAFETY,
    POWER_CAUSE_AC_CONNECTED_SAFETY,
    POWER_CAUSE_AC_SIGNAL_LOST,
    POWER_CAUSE_BUTTON_SHORT,
    POWER_CAUSE_BUTTON_LONG,
    POWER_CAUSE_WEB_FAN,
    POWER_CAUSE_WEB_POWER_OFF,
    POWER_CAUSE_WEB_CONTROL,
    POWER_CAUSE_SCHEDULE_START,
    POWER_CAUSE_SCHEDULE_END,
    POWER_CAUSE_SCHEDULE_EDIT,
    POWER_CAUSE_DURATION_TIMER,
    POWER_CAUSE_CLOCK_TIMER,
    POWER_CAUSE_TEMP_TARGET,
    POWER_CAUSE_TEMP_RESUME,
    POWER_CAUSE_TEMP_SENSOR_FAULT,
    POWER_CAUSE_OTA_FIRMWARE,
    POWER_CAUSE_OTA_LITTLEFS,
    POWER_CAUSE_ADMIN_REBOOT,
    POWER_CAUSE_APP_SESSION,
    POWER_CAUSE_FIRE_GAP
};

struct PowerEventRecord {
    uint32_t epoch;
    uint32_t uptimeMs;
    uint32_t bootId;
    uint32_t detail;
    uint8_t type;
    uint8_t cause;
    uint8_t mode;
    uint8_t speedPercent;
};

struct PowerLogStore {
    uint32_t magic;
    uint8_t version;
    uint8_t count;
    uint16_t reserved;
    PowerEventRecord events[POWER_LOG_MAX_EVENTS];
};

enum H11Role : uint8_t {
    H11_SA_A = 0,
    H11_SA_B = 1,
    H11_SB_A = 2,
    H11_SB_B = 3
};

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct ModeInfo {
    const char* id;
    const char* label;
    FanSpeed speed;
    int setpointF;
    bool thermostat;
    bool active;
};

static const ModeInfo MODE_INFO[] = {
    {"off_loop", "OFF loop", SPEED_OFF, 0, false, false},
    {"high", "HIGH continuous", SPEED_HIGH, 0, false, true},
    {"low", "LOW continuous", SPEED_LOW, 0, false, true},
    {"high_60", "HIGH 60F", SPEED_HIGH, 60, true, true},
    {"high_65", "HIGH 65F", SPEED_HIGH, 65, true, true},
    {"high_70", "HIGH 70F", SPEED_HIGH, 70, true, true},
    {"high_75", "HIGH 75F", SPEED_HIGH, 75, true, true},
    {"high_80", "HIGH 80F", SPEED_HIGH, 80, true, true},
    {"low_60", "LOW 60F", SPEED_LOW, 60, true, true},
    {"low_65", "LOW 65F", SPEED_LOW, 65, true, true},
    {"low_70", "LOW 70F", SPEED_LOW, 70, true, true},
    {"low_75", "LOW 75F", SPEED_LOW, 75, true, true},
    {"low_80", "LOW 80F", SPEED_LOW, 80, true, true},
    {"custom", "WEB custom", SPEED_CUSTOM, 0, false, true},
    {"off_memory", "OFF memory", SPEED_OFF, 0, false, false}
};

static const char* LED_LABELS[LED_COUNT] = {
    "LOW",
    "60F",
    "65F",
    "70F",
    "HIGH",
    "75F",
    "80F"
};

static const char* H11_ROLE_LABELS[4] = {
    "SA-A",
    "SA-B",
    "SB-A",
    "SB-B"
};

WebServer webServer(80);
Preferences prefs;
Adafruit_NeoPixel pixels(LED_COUNT, PIN_LED_DATA, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel boardLedPixel(1, PIN_BOARD_LED, NEO_GRB + NEO_KHZ800);

static Rgb cfgLed[LED_COUNT];
static uint8_t cfgLedEnabled[LED_COUNT];
static bool cfgLightsOn = true;
static uint8_t cfgLedDimmerPercent = 100;
static bool cfgButtonEnabled = DEFAULT_BUTTON_ENABLED;
static bool cfgCustomSpeedEnabled = false;
static uint8_t cfgCustomSpeedPercent = FAN_CUSTOM_DEFAULT_PERCENT;
static bool cfgScheduleEnabled = false;
static uint8_t cfgScheduleMode = SCHEDULE_OFF;
static uint8_t cfgScheduleDaysMask = 0;
static uint16_t cfgScheduleStartMin = 8 * 60;
static uint16_t cfgScheduleEndMin = 18 * 60;
static uint8_t cfgDailyDaysMask = 0;
static uint16_t cfgDailyStartMin[7] = {480, 480, 480, 480, 480, 480, 480};
static uint16_t cfgDailyEndMin[7] = {1080, 1080, 1080, 1080, 1080, 1080, 1080};
static uint8_t cfgScheduleSpeed = SPEED_HIGH;
static uint8_t cfgScheduleSpeedPercent = FAN_HIGH_REFERENCE_PERCENT;
static uint8_t cfgScheduleLedDimmerPercent = 100;
static uint8_t cfgScheduleTempRule = SCHEDULE_TEMP_NONE;
static int16_t cfgScheduleTempF = 75;
static uint16_t cfgHighDelayUs = DEFAULT_HIGH_DELAY_US;
static uint16_t cfgLowDelayUs = DEFAULT_LOW_DELAY_US;
static uint16_t cfgMocPulseUs = DEFAULT_MOC_PULSE_US;
static uint8_t cfgHystTenthF = 10;
static uint8_t cfgLedDataPin = PIN_LED_DATA;
static uint8_t cfgH11Role[4] = {H11_SA_A, H11_SA_B, H11_SB_A, H11_SB_B};
static String cfgApSsid = AP_SSID_DEFAULT;
static String cfgApPass = AP_PASS_DEFAULT;
static String cfgWebUser = WEB_USER_DEFAULT;
static String cfgWebPass = WEB_PASS_DEFAULT;
static String cfgAdminUser = ADMIN_USER_DEFAULT;
static String cfgAdminPass = ADMIN_PASS_DEFAULT;
static String cfgStaSsid = STA_SSID_DEFAULT;
static String cfgStaPass = STA_PASS_DEFAULT;
static bool cfgStaStaticEnabled = true;
static String cfgStaIp = STA_IP_DEFAULT;
static String cfgStaGateway = STA_GATEWAY_DEFAULT;
static String cfgStaSubnet = STA_SUBNET_DEFAULT;
static String cfgStaDns = STA_DNS_DEFAULT;
static String cfgHostname = HOSTNAME_DEFAULT;
static String gSessionToken;
static bool gConfigNeedsSave = false;
static bool gStaStarted = false;
static bool gMdnsStarted = false;
static uint32_t gLastWifiServiceMs = 0;
static uint32_t gWifiRestartAtMs = 0;
static bool gBoardLedOn = false;

static FanMode gMode = MODE_OFF_LOOP;
static FanMode gMemoryMode = MODE_HIGH_CONT;
static bool gMemoryValid = false;
static FanSpeed gCurrentSpeed = SPEED_OFF;
static bool gMotorShouldRun = false;
static bool gMotorWasRunning = false;
static bool gStartBoostActive = false;
static uint32_t gStartBoostUntilMs = 0;
static bool gThermostatCalling = false;
static bool gTempSafetyFault = false;
static bool gMocArmed = false;
static bool gAcPresent = false;
static uint32_t gLastAcStableMs = 0;
static uint32_t gAcConnectOffCount = 0;

static float gTempC = NAN;
static float gTempF = NAN;
static bool gTempPresent = false;
static bool gTempPending = false;
static uint32_t gTempRequestMs = 0;
static uint32_t gTempLastReadMs = 0;
static uint32_t gTempReadErrors = 0;
static uint32_t gTempResetOkCount = 0;
static uint32_t gTempResetFailCount = 0;
static uint32_t gTempCrcErrors = 0;
static uint8_t gTempRawLevel = 1;
static bool gTempLastPresence = false;
static uint16_t gTempPresenceDelayUs = 0;
static uint32_t gTempHistoryDay[TEMP_HISTORY_DAYS] = {0, 0, 0, 0, 0, 0, 0};
static int16_t gTempHistoryF[TEMP_HISTORY_DAYS * TEMP_HISTORY_HOURS];

static bool gTimeSynced = false;
static uint32_t gEpochAtSync = 0;
static uint32_t gMillisAtSync = 0;
static int16_t gTzOffsetMinutes = 0;
static bool gDurationTimerActive = false;
static uint32_t gDurationOffAtMs = 0;
static bool gClockTimerActive = false;
static uint32_t gClockOffAtEpoch = 0;
static bool gScheduleWindowWasActive = false;
static bool gScheduleDriving = false;
static bool gScheduleOverridesApplied = false;
static uint8_t gSchedulePreviousSpeedPercent = FAN_CUSTOM_DEFAULT_PERCENT;
static uint8_t gSchedulePreviousLedDimmerPercent = 100;

static bool gButtonStablePressed = false;
static bool gButtonLastRawPressed = false;
static bool gButtonWasPressed = false;
static uint32_t gButtonLastChangeMs = 0;
static uint32_t gButtonPressStartMs = 0;
static uint32_t gButtonCount = 0;
static uint32_t gButtonLongCount = 0;
static uint32_t gButtonLastEventMs = 0;

static volatile uint32_t vZcRiseCount = 0;
static volatile uint32_t vZcEdgeCount = 0;
static volatile uint32_t vZcLastRiseUs = 0;
static volatile uint32_t vZcLastEdgeUs = 0;
static volatile uint32_t vZcLastIntervalUs = 0;
static volatile uint32_t vZcPulseStartUs = 0;
static volatile uint32_t vZcPulseWidthUs = 0;
static volatile uint32_t vZcLongGapPendingUs = 0;
static volatile bool vZcLevel = false;
static TaskHandle_t gTriacFireTaskHandle = nullptr;
static portMUX_TYPE gTriacFireMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool vTriacFireEnabled = false;
static volatile uint16_t vTriacFireDelayUs = DEFAULT_HIGH_DELAY_US;
static volatile uint16_t vTriacPulseUs = DEFAULT_MOC_PULSE_US;

static volatile uint32_t vH11Edges[4] = {0, 0, 0, 0};
static volatile uint32_t vH11LastEdgeUs[4] = {0, 0, 0, 0};
static volatile uint8_t vH11Level[4] = {1, 1, 1, 1};

static uint32_t gPrevZcRiseCount = 0;
static uint32_t gPrevZcSampleMs = 0;
static float gZcHz = 0.0f;
static bool gZcStable = false;
static uint32_t gZcPulseWidthUs = 0;
static uint32_t gZcIntervalUs = 0;
static uint32_t gZcLastRiseSnapshotUs = 0;
static uint32_t gZcLastRiseAgeUs = UINT32_MAX;
static uint32_t gZcSampleElapsedMs = 0;

static uint32_t gPrevH11Edges[4] = {0, 0, 0, 0};
static uint32_t gPrevH11SampleMs = 0;
static float gH11EdgeHz[4] = {0, 0, 0, 0};
static bool gH11Active[4] = {false, false, false, false};

static uint32_t localEpochNow();
static void loadTempHistory();
static void recordTempHistory();
static void appendTempHistoryJson(String& s);
static uint32_t gH11LastAgeMs[4] = {0, 0, 0, 0};

static volatile bool gTriacFireTaskReady = false;
static volatile uint32_t gTriacNotifyCount = 0;
static volatile uint32_t gTriacWakeCount = 0;
static volatile uint32_t gTriacLastWakeMs = 0;
static volatile uint8_t gTriacTaskStage = 0;
static volatile bool gMocPulseActive = false;
static volatile uint32_t gFirePulseCount = 0;
static volatile uint32_t gLastFireMs = 0;
static volatile uint32_t gLastFireUs = 0;
static volatile uint32_t gFireLastGapUs = 0;
static volatile uint32_t gFireMaxGapUs = 0;

enum LedPreviewMode : uint8_t {
    LED_PREVIEW_NONE = 0,
    LED_PREVIEW_SINGLE = 1,
    LED_PREVIEW_ALL = 2,
    LED_PREVIEW_CHASE = 3
};

static LedPreviewMode gLedPreviewMode = LED_PREVIEW_NONE;
static int8_t gLedPreviewIndex = -1;
static uint32_t gLedPreviewUntilMs = 0;
static uint32_t gLedShowCount = 0;
static uint32_t gLedLastShowMs = 0;
static int8_t gLedDataOverride = -1;

static bool gUpdateActive = false;
static bool gUpdateOk = false;
static String gUpdateKind = "";
static String gUpdateError = "";
static size_t gUpdateBytes = 0;
static size_t gUpdateTotal = 0;
static uint32_t gUpdateStartedMs = 0;
static uint32_t gRebootAtMs = 0;
static PowerLogStore gPowerLog;
static uint32_t gBootId = 0;
static PowerEventCause gPendingMotorCause = POWER_CAUSE_INTERNAL;
static bool gFireGapLogged = false;
static bool gPowerLogDirty = false;
static bool gTempHistoryDirty = false;
static bool gZcOutputBlockedLogged = false;

static inline bool timeDueUs(uint32_t now, uint32_t due) {
    return (int32_t)(now - due) >= 0;
}

static inline String jsonBool(bool value) {
    return value ? "true" : "false";
}

static String jsonEscape(const String& input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else {
            out += c;
        }
    }
    return out;
}

static bool jsonHas(const String& body, const char* key) {
    String needle = String("\"") + key + "\"";
    return body.indexOf(needle) >= 0;
}

static long jsonLong(const String& body, const char* key, long fallback) {
    String needle = String("\"") + key + "\"";
    int pos = body.indexOf(needle);
    if (pos < 0) return fallback;
    pos = body.indexOf(':', pos + needle.length());
    if (pos < 0) return fallback;
    pos++;
    while (pos < (int)body.length() && isspace((unsigned char)body[pos])) pos++;
    bool neg = false;
    if (pos < (int)body.length() && body[pos] == '-') {
        neg = true;
        pos++;
    }
    long value = 0;
    bool any = false;
    while (pos < (int)body.length() && isdigit((unsigned char)body[pos])) {
        any = true;
        value = value * 10 + (body[pos] - '0');
        pos++;
    }
    if (!any) return fallback;
    return neg ? -value : value;
}

static int jsonInt(const String& body, const char* key, int fallback) {
    return (int)jsonLong(body, key, fallback);
}

static String jsonString(const String& body, const char* key, const String& fallback) {
    String needle = String("\"") + key + "\"";
    int pos = body.indexOf(needle);
    if (pos < 0) return fallback;
    pos = body.indexOf(':', pos + needle.length());
    if (pos < 0) return fallback;
    pos++;
    while (pos < (int)body.length() && isspace((unsigned char)body[pos])) pos++;
    if (pos >= (int)body.length() || body[pos] != '"') return fallback;
    pos++;
    String out;
    while (pos < (int)body.length()) {
        char c = body[pos++];
        if (c == '"') break;
        if (c == '\\' && pos < (int)body.length()) {
            char n = body[pos++];
            if (n == 'n') out += '\n';
            else if (n == 'r') out += '\r';
            else out += n;
        } else {
            out += c;
        }
    }
    return out;
}

static String saneConfigString(String value, const char* fallback, size_t minLen, size_t maxLen) {
    value.trim();
    if (value.length() < minLen || value.length() > maxLen) return String(fallback);
    return value;
}

static String saneOptionalString(String value, size_t maxLen) {
    value.trim();
    if (value.length() > maxLen) value = value.substring(0, maxLen);
    return value;
}

static String saneHostname(String value) {
    value.trim();
    value.toLowerCase();
    String out;
    for (size_t i = 0; i < value.length() && out.length() < 31; i++) {
        char c = value[i];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') out += c;
    }
    if (out.length() == 0) return String(HOSTNAME_DEFAULT);
    return out;
}

static String saneIpString(String value, const char* fallback) {
    value.trim();
    IPAddress ip;
    if (ip.fromString(value)) return value;
    return String(fallback);
}

static bool parseIpString(const String& value, IPAddress& out) {
    return out.fromString(value);
}

static const ModeInfo& modeInfo(FanMode mode) {
    uint8_t idx = (uint8_t)mode;
    if (idx >= (sizeof(MODE_INFO) / sizeof(MODE_INFO[0]))) idx = 0;
    return MODE_INFO[idx];
}

static const char* speedName(FanSpeed speed) {
    if (speed == SPEED_HIGH) return "HIGH";
    if (speed == SPEED_LOW) return "LOW";
    if (speed == SPEED_CUSTOM) return "CUSTOM";
    return "OFF";
}

static const char* powerEventTypeName(uint8_t type) {
    switch ((PowerEventType)type) {
        case POWER_EVENT_BOOT: return "boot";
        case POWER_EVENT_MOTOR_ON: return "motor-on";
        case POWER_EVENT_MOTOR_OFF: return "motor-off";
        case POWER_EVENT_AC_PRESENT: return "ac-present";
        case POWER_EVENT_AC_LOST: return "ac-lost";
        case POWER_EVENT_APP_SESSION: return "app-session";
        case POWER_EVENT_OUTPUT_GAP: return "output-gap";
        case POWER_EVENT_OUTPUT_BLOCKED: return "output-blocked";
        case POWER_EVENT_OUTPUT_RESTORED: return "output-restored";
        case POWER_EVENT_SYSTEM: return "system";
        default: return "unknown";
    }
}

static const char* powerCauseName(uint8_t cause) {
    switch ((PowerEventCause)cause) {
        case POWER_CAUSE_RESET_POWER_ON: return "power-on-reset";
        case POWER_CAUSE_RESET_SOFTWARE: return "software-reset";
        case POWER_CAUSE_RESET_PANIC: return "panic-reset";
        case POWER_CAUSE_RESET_WATCHDOG: return "watchdog-reset";
        case POWER_CAUSE_RESET_BROWNOUT: return "brownout-reset";
        case POWER_CAUSE_RESET_OTHER: return "other-reset";
        case POWER_CAUSE_BOOT_SAFETY: return "boot-safety-off";
        case POWER_CAUSE_AC_CONNECTED_SAFETY: return "ac-connected-safety-off";
        case POWER_CAUSE_AC_SIGNAL_LOST: return "ac-signal-lost";
        case POWER_CAUSE_BUTTON_SHORT: return "physical-button-short";
        case POWER_CAUSE_BUTTON_LONG: return "physical-button-long";
        case POWER_CAUSE_WEB_FAN: return "web-fan-command";
        case POWER_CAUSE_WEB_POWER_OFF: return "web-power-off";
        case POWER_CAUSE_WEB_CONTROL: return "web-control-command";
        case POWER_CAUSE_SCHEDULE_START: return "schedule-start";
        case POWER_CAUSE_SCHEDULE_END: return "schedule-end";
        case POWER_CAUSE_SCHEDULE_EDIT: return "schedule-edit";
        case POWER_CAUSE_DURATION_TIMER: return "duration-timer";
        case POWER_CAUSE_CLOCK_TIMER: return "clock-timer";
        case POWER_CAUSE_TEMP_TARGET: return "temperature-target";
        case POWER_CAUSE_TEMP_RESUME: return "temperature-resume";
        case POWER_CAUSE_TEMP_SENSOR_FAULT: return "temperature-sensor-fault";
        case POWER_CAUSE_OTA_FIRMWARE: return "firmware-ota";
        case POWER_CAUSE_OTA_LITTLEFS: return "littlefs-ota";
        case POWER_CAUSE_ADMIN_REBOOT: return "admin-reboot";
        case POWER_CAUSE_APP_SESSION: return "app-session-check";
        case POWER_CAUSE_FIRE_GAP: return "triac-fire-gap";
        default: return "internal";
    }
}

static PowerEventCause resetCause() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return POWER_CAUSE_RESET_POWER_ON;
        case ESP_RST_SW: return POWER_CAUSE_RESET_SOFTWARE;
        case ESP_RST_PANIC: return POWER_CAUSE_RESET_PANIC;
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT: return POWER_CAUSE_RESET_WATCHDOG;
        case ESP_RST_BROWNOUT: return POWER_CAUSE_RESET_BROWNOUT;
        default: return POWER_CAUSE_RESET_OTHER;
    }
}

static uint8_t speedPercentForMode(FanMode mode) {
    FanSpeed speed = modeInfo(mode).speed;
    if (speed == SPEED_CUSTOM) return cfgCustomSpeedPercent;
    if (speed == SPEED_HIGH) return FAN_HIGH_REFERENCE_PERCENT;
    if (speed == SPEED_LOW) return FAN_LOW_REFERENCE_PERCENT;
    return 0;
}

static void savePowerLog() {
    gPowerLogDirty = true;
    if (gMotorShouldRun || vTriacFireEnabled) return;
    prefs.putBytes("power_log", &gPowerLog, sizeof(gPowerLog));
    gPowerLogDirty = false;
}

static void servicePowerLogPersistence() {
    if (gPowerLogDirty && !gMotorShouldRun && !vTriacFireEnabled) savePowerLog();
}

static void serviceTempHistoryPersistence() {
    if (!gTempHistoryDirty || gMotorShouldRun || vTriacFireEnabled) return;
    prefs.putBytes("th_day", gTempHistoryDay, sizeof(gTempHistoryDay));
    prefs.putBytes("th_f", gTempHistoryF, sizeof(gTempHistoryF));
    gTempHistoryDirty = false;
}

static bool prunePowerLog(uint32_t nowEpoch) {
    if (!nowEpoch) return false;
    uint8_t writeIndex = 0;
    for (uint8_t i = 0; i < gPowerLog.count; i++) {
        const PowerEventRecord& event = gPowerLog.events[i];
        bool currentUnsynced = event.epoch == 0 && event.bootId == gBootId;
        bool recent = event.epoch != 0 &&
                      (event.epoch >= nowEpoch || nowEpoch - event.epoch <= POWER_LOG_RETENTION_SECONDS);
        if (currentUnsynced || recent) {
            if (writeIndex != i) gPowerLog.events[writeIndex] = event;
            writeIndex++;
        }
    }
    bool changed = writeIndex != gPowerLog.count;
    gPowerLog.count = writeIndex;
    return changed;
}

static void loadPowerLog() {
    memset(&gPowerLog, 0, sizeof(gPowerLog));
    if (prefs.getBytesLength("power_log") == sizeof(gPowerLog)) {
        prefs.getBytes("power_log", &gPowerLog, sizeof(gPowerLog));
    }
    if (gPowerLog.magic != POWER_LOG_MAGIC || gPowerLog.version != POWER_LOG_VERSION ||
        gPowerLog.count > POWER_LOG_MAX_EVENTS) {
        memset(&gPowerLog, 0, sizeof(gPowerLog));
        gPowerLog.magic = POWER_LOG_MAGIC;
        gPowerLog.version = POWER_LOG_VERSION;
    }
    gBootId = prefs.getUInt("power_boot", 0) + 1;
    prefs.putUInt("power_boot", gBootId);
}

static void recordPowerEvent(PowerEventType type, PowerEventCause cause, uint32_t detail = 0,
                             FanMode mode = MODE_OFF_LOOP, int speedPercent = -1) {
    uint32_t nowMs = millis();
    uint32_t nowEpoch = localEpochNow();
    prunePowerLog(nowEpoch);

    if (gPowerLog.count) {
        const PowerEventRecord& last = gPowerLog.events[gPowerLog.count - 1];
        uint32_t duplicateWindowMs = type == POWER_EVENT_APP_SESSION ? 60000UL : 1500UL;
        if (last.bootId == gBootId && last.type == type && last.cause == cause &&
            nowMs - last.uptimeMs < duplicateWindowMs) {
            return;
        }
    }
    if (gPowerLog.count >= POWER_LOG_MAX_EVENTS) {
        memmove(&gPowerLog.events[0], &gPowerLog.events[1],
                sizeof(gPowerLog.events[0]) * (POWER_LOG_MAX_EVENTS - 1));
        gPowerLog.count = POWER_LOG_MAX_EVENTS - 1;
    }

    PowerEventRecord& event = gPowerLog.events[gPowerLog.count++];
    event.epoch = nowEpoch;
    event.uptimeMs = nowMs;
    event.bootId = gBootId;
    event.detail = detail;
    event.type = (uint8_t)type;
    event.cause = (uint8_t)cause;
    event.mode = (uint8_t)mode;
    event.speedPercent = speedPercent >= 0 ? constrain(speedPercent, 0, 100) : speedPercentForMode(mode);
    savePowerLog();
}

static void stampCurrentBootPowerEvents() {
    uint32_t nowEpoch = localEpochNow();
    if (!nowEpoch) return;
    uint32_t nowMs = millis();
    bool changed = false;
    for (uint8_t i = 0; i < gPowerLog.count; i++) {
        PowerEventRecord& event = gPowerLog.events[i];
        if (event.bootId != gBootId || event.epoch != 0) continue;
        uint32_t ageSeconds = (nowMs - event.uptimeMs) / 1000UL;
        event.epoch = nowEpoch > ageSeconds ? nowEpoch - ageSeconds : nowEpoch;
        changed = true;
    }
    if (prunePowerLog(nowEpoch)) changed = true;
    if (changed) savePowerLog();
}

static void appendPowerLogJson(String& s) {
    uint32_t nowEpoch = localEpochNow();
    if (prunePowerLog(nowEpoch)) savePowerLog();
    s += "{\"retentionHours\":24,\"capacity\":" + String(POWER_LOG_MAX_EVENTS) +
         ",\"bootId\":" + String(gBootId) + ",\"events\":[";
    bool first = true;
    for (int i = (int)gPowerLog.count - 1; i >= 0; i--) {
        const PowerEventRecord& event = gPowerLog.events[i];
        if (!first) s += ",";
        first = false;
        FanMode mode = event.mode < (sizeof(MODE_INFO) / sizeof(MODE_INFO[0])) ?
                       (FanMode)event.mode : MODE_OFF_LOOP;
        s += "{\"epoch\":" + String(event.epoch) +
             ",\"uptimeMs\":" + String(event.uptimeMs) +
             ",\"bootId\":" + String(event.bootId) +
             ",\"event\":\"" + powerEventTypeName(event.type) +
             "\",\"cause\":\"" + powerCauseName(event.cause) +
             "\",\"mode\":\"" + String(modeInfo(mode).label) +
             "\",\"speedPercent\":" + String(event.speedPercent) +
             ",\"detail\":" + String(event.detail) + "}";
    }
    s += "]}";
}

static bool modeIsActive(FanMode mode) {
    return modeInfo(mode).active;
}

static void enableAllLedSession() {
    for (uint8_t i = 0; i < LED_COUNT; i++) cfgLedEnabled[i] = 1;
}

static void defaultLedConfig() {
    cfgLed[0] = {0, 210, 255};   // LOW
    cfgLed[1] = {0, 170, 255};   // 60F
    cfgLed[2] = {48, 130, 255};  // 65F
    cfgLed[3] = {255, 215, 70};  // 70F
    cfgLed[4] = {0, 255, 110};   // HIGH
    cfgLed[5] = {255, 132, 28};  // 75F
    cfgLed[6] = {255, 58, 82};   // 80F
    enableAllLedSession();
}

static void defaultH11Map() {
    cfgH11Role[0] = H11_SA_A;
    cfgH11Role[1] = H11_SA_B;
    cfgH11Role[2] = H11_SB_A;
    cfgH11Role[3] = H11_SB_B;
}

static bool validLedDataPin(uint8_t pin) {
    return pin == PIN_LED_DATA;
}

static void saveConfig() {
    prefs.putUChar("cfg_ver", CONFIG_SCHEMA_VERSION);
    prefs.putBool("lights", true);
    prefs.putBytes("ledrgb", cfgLed, sizeof(cfgLed));
    prefs.putUChar("led_dim", cfgLedDimmerPercent);
    uint8_t ledEnabledDefaults[LED_COUNT];
    for (uint8_t i = 0; i < LED_COUNT; i++) ledEnabledDefaults[i] = 1;
    prefs.putBytes("leden", ledEnabledDefaults, sizeof(ledEnabledDefaults));
    prefs.putUChar("ledpin", cfgLedDataPin);
    prefs.putBool("btn_en", cfgButtonEnabled);
    prefs.putBool("cust_en", cfgCustomSpeedEnabled);
    prefs.putUChar("cust_pct", cfgCustomSpeedPercent);
    prefs.putBool("sch_en", cfgScheduleEnabled);
    prefs.putUChar("sch_mode", cfgScheduleMode);
    prefs.putUChar("sch_days", cfgScheduleDaysMask);
    prefs.putUShort("sch_start", cfgScheduleStartMin);
    prefs.putUShort("sch_end", cfgScheduleEndMin);
    prefs.putUChar("sch_ddays", cfgDailyDaysMask);
    prefs.putBytes("sch_dstart", cfgDailyStartMin, sizeof(cfgDailyStartMin));
    prefs.putBytes("sch_dend", cfgDailyEndMin, sizeof(cfgDailyEndMin));
    prefs.putUChar("sch_speed", cfgScheduleSpeed);
    prefs.putUChar("sch_pct", cfgScheduleSpeedPercent);
    prefs.putUChar("sch_dim", cfgScheduleLedDimmerPercent);
    prefs.putUChar("sch_rule", cfgScheduleTempRule);
    prefs.putShort("sch_temp", cfgScheduleTempF);
    prefs.putUShort("hi_us", cfgHighDelayUs);
    prefs.putUShort("lo_us", cfgLowDelayUs);
    prefs.putUShort("pulse_us", cfgMocPulseUs);
    prefs.putUChar("hyst10", cfgHystTenthF);
    prefs.putBytes("h11map", cfgH11Role, sizeof(cfgH11Role));
    prefs.putString("ssid", cfgApSsid);
    prefs.putString("wpass", cfgApPass);
    prefs.putString("user", cfgWebUser);
    prefs.putString("webpass", cfgWebPass);
    prefs.putString("adm_user", cfgAdminUser);
    prefs.putString("adm_pass", cfgAdminPass);
    prefs.putString("sta_ssid", cfgStaSsid);
    prefs.putString("sta_pass", cfgStaPass);
    prefs.putBool("sta_static", cfgStaStaticEnabled);
    prefs.putString("sta_ip", cfgStaIp);
    prefs.putString("sta_gw", cfgStaGateway);
    prefs.putString("sta_sub", cfgStaSubnet);
    prefs.putString("sta_dns", cfgStaDns);
    prefs.putString("host", cfgHostname);
    gConfigNeedsSave = false;
}

static void loadConfig() {
    defaultLedConfig();
    defaultH11Map();
    gConfigNeedsSave = false;
    uint8_t schema = prefs.getUChar("cfg_ver", 0);
    cfgLightsOn = true;
    if (prefs.getBytesLength("ledrgb") == sizeof(cfgLed)) {
        prefs.getBytes("ledrgb", cfgLed, sizeof(cfgLed));
    }
    cfgLedDimmerPercent = constrain(prefs.getUChar("led_dim", 100), 0, 100);
    enableAllLedSession();
    cfgLedDataPin = prefs.getUChar("ledpin", PIN_LED_DATA);
    if (!validLedDataPin(cfgLedDataPin)) cfgLedDataPin = PIN_LED_DATA;
    cfgButtonEnabled = prefs.getBool("btn_en", DEFAULT_BUTTON_ENABLED);
    if (schema < CONFIG_SCHEMA_VERSION) {
        cfgButtonEnabled = DEFAULT_BUTTON_ENABLED;
        gConfigNeedsSave = true;
    }
    cfgCustomSpeedEnabled = prefs.getBool("cust_en", false);
    cfgCustomSpeedPercent = constrain(prefs.getUChar("cust_pct", FAN_CUSTOM_DEFAULT_PERCENT),
                                      FAN_CUSTOM_MIN_PERCENT, FAN_CUSTOM_MAX_PERCENT);
    cfgScheduleEnabled = prefs.getBool("sch_en", false);
    cfgScheduleMode = prefs.getUChar("sch_mode", cfgScheduleEnabled ? SCHEDULE_WEEKLY : SCHEDULE_OFF);
    if (cfgScheduleMode > SCHEDULE_DAILY) cfgScheduleMode = SCHEDULE_OFF;
    cfgScheduleEnabled = cfgScheduleMode != SCHEDULE_OFF;
    cfgScheduleDaysMask = prefs.getUChar("sch_days", 0) & 0x7F;
    cfgScheduleStartMin = constrain(prefs.getUShort("sch_start", 8 * 60), 0, MINUTES_PER_DAY - 1);
    cfgScheduleEndMin = constrain(prefs.getUShort("sch_end", 18 * 60), 0, MINUTES_PER_DAY - 1);
    cfgDailyDaysMask = prefs.getUChar("sch_ddays", 0) & 0x7F;
    if (prefs.getBytesLength("sch_dstart") == sizeof(cfgDailyStartMin)) {
        prefs.getBytes("sch_dstart", cfgDailyStartMin, sizeof(cfgDailyStartMin));
    }
    if (prefs.getBytesLength("sch_dend") == sizeof(cfgDailyEndMin)) {
        prefs.getBytes("sch_dend", cfgDailyEndMin, sizeof(cfgDailyEndMin));
    }
    for (uint8_t i = 0; i < 7; i++) {
        cfgDailyStartMin[i] = constrain(cfgDailyStartMin[i], 0, MINUTES_PER_DAY - 1);
        cfgDailyEndMin[i] = constrain(cfgDailyEndMin[i], 0, MINUTES_PER_DAY - 1);
    }
    cfgScheduleSpeed = prefs.getUChar("sch_speed", SPEED_HIGH);
    if (cfgScheduleSpeed != SPEED_HIGH && cfgScheduleSpeed != SPEED_LOW && cfgScheduleSpeed != SPEED_CUSTOM) {
        cfgScheduleSpeed = SPEED_HIGH;
    }
    cfgScheduleSpeedPercent = constrain(prefs.getUChar("sch_pct", FAN_HIGH_REFERENCE_PERCENT),
                                        FAN_CUSTOM_MIN_PERCENT, FAN_CUSTOM_MAX_PERCENT);
    cfgScheduleLedDimmerPercent = constrain(prefs.getUChar("sch_dim", 100), 0, 100);
    cfgScheduleTempRule = prefs.getUChar("sch_rule", SCHEDULE_TEMP_NONE);
    if (cfgScheduleTempRule > SCHEDULE_TEMP_OR) cfgScheduleTempRule = SCHEDULE_TEMP_NONE;
    cfgScheduleTempF = constrain(prefs.getShort("sch_temp", 75), 40, 110);
    if (prefs.getBytesLength("h11map") == sizeof(cfgH11Role)) {
        prefs.getBytes("h11map", cfgH11Role, sizeof(cfgH11Role));
        for (uint8_t i = 0; i < 4; i++) {
            if (cfgH11Role[i] > H11_SB_B) defaultH11Map();
        }
    }
    cfgHighDelayUs = prefs.getUShort("hi_us", DEFAULT_HIGH_DELAY_US);
    cfgLowDelayUs = prefs.getUShort("lo_us", DEFAULT_LOW_DELAY_US);
    cfgMocPulseUs = prefs.getUShort("pulse_us", DEFAULT_MOC_PULSE_US);
    cfgHystTenthF = prefs.getUChar("hyst10", 10);
    cfgApSsid = saneConfigString(prefs.getString("ssid", AP_SSID_DEFAULT), AP_SSID_DEFAULT, 1, 31);
    cfgApPass = saneConfigString(prefs.getString("wpass", AP_PASS_DEFAULT), AP_PASS_DEFAULT, 8, 63);
    cfgWebUser = saneConfigString(prefs.getString("user", WEB_USER_DEFAULT), WEB_USER_DEFAULT, 1, 31);
    cfgWebPass = saneConfigString(prefs.getString("webpass", WEB_PASS_DEFAULT), WEB_PASS_DEFAULT, 1, 63);
    cfgAdminUser = saneConfigString(prefs.getString("adm_user", ADMIN_USER_DEFAULT), ADMIN_USER_DEFAULT, 1, 31);
    cfgAdminPass = saneConfigString(prefs.getString("adm_pass", ADMIN_PASS_DEFAULT), ADMIN_PASS_DEFAULT, 1, 63);
    cfgStaSsid = saneOptionalString(prefs.getString("sta_ssid", STA_SSID_DEFAULT), 32);
    cfgStaPass = saneOptionalString(prefs.getString("sta_pass", STA_PASS_DEFAULT), 63);
    cfgStaStaticEnabled = prefs.getBool("sta_static", true);
    cfgStaIp = saneIpString(prefs.getString("sta_ip", STA_IP_DEFAULT), STA_IP_DEFAULT);
    cfgStaGateway = saneIpString(prefs.getString("sta_gw", STA_GATEWAY_DEFAULT), STA_GATEWAY_DEFAULT);
    cfgStaSubnet = saneIpString(prefs.getString("sta_sub", STA_SUBNET_DEFAULT), STA_SUBNET_DEFAULT);
    cfgStaDns = saneIpString(prefs.getString("sta_dns", STA_DNS_DEFAULT), STA_DNS_DEFAULT);
    cfgHostname = saneHostname(prefs.getString("host", HOSTNAME_DEFAULT));
    if (schema < 4) gConfigNeedsSave = true;
    if (schema < 5) {
        enableAllLedSession();
        gConfigNeedsSave = true;
    }
    if (schema < 6) {
        cfgScheduleEnabled = false;
        cfgScheduleDaysMask = 0;
        cfgScheduleStartMin = 8 * 60;
        cfgScheduleEndMin = 18 * 60;
        cfgScheduleSpeed = SPEED_HIGH;
        cfgScheduleTempRule = SCHEDULE_TEMP_NONE;
        cfgScheduleTempF = 75;
        gConfigNeedsSave = true;
    }
	    if (schema < 7) {
	        cfgLedDimmerPercent = 100;
	        cfgScheduleTempRule = SCHEDULE_TEMP_NONE;
	        cfgScheduleTempF = 75;
	        gConfigNeedsSave = true;
	    }
    if (schema < 8) {
        cfgScheduleSpeedPercent = cfgScheduleSpeed == SPEED_LOW ? FAN_LOW_REFERENCE_PERCENT :
                                  FAN_HIGH_REFERENCE_PERCENT;
        cfgScheduleLedDimmerPercent = cfgLedDimmerPercent;
        gConfigNeedsSave = true;
    }
    if (schema < 9) {
        cfgScheduleMode = cfgScheduleEnabled ? SCHEDULE_WEEKLY : SCHEDULE_OFF;
        cfgDailyDaysMask = 0;
        for (uint8_t i = 0; i < 7; i++) {
            cfgDailyStartMin[i] = cfgScheduleStartMin;
            cfgDailyEndMin[i] = cfgScheduleEndMin;
        }
        gConfigNeedsSave = true;
    }
    cfgScheduleEnabled = cfgScheduleMode != SCHEDULE_OFF;
    cfgHighDelayUs = constrain(cfgHighDelayUs, 0, 8000);
    cfgLowDelayUs = constrain(cfgLowDelayUs, 0, 8000);
    cfgMocPulseUs = constrain(cfgMocPulseUs, 40, 1000);
    cfgHystTenthF = constrain(cfgHystTenthF, 2, 50);
    gMocArmed = false;
}

static void factoryDefaults() {
    defaultLedConfig();
    defaultH11Map();
    cfgLightsOn = true;
    cfgLedDimmerPercent = 100;
    cfgButtonEnabled = DEFAULT_BUTTON_ENABLED;
    cfgCustomSpeedEnabled = false;
    cfgCustomSpeedPercent = FAN_CUSTOM_DEFAULT_PERCENT;
    cfgScheduleEnabled = false;
    cfgScheduleMode = SCHEDULE_OFF;
    cfgScheduleDaysMask = 0;
    cfgScheduleStartMin = 8 * 60;
    cfgScheduleEndMin = 18 * 60;
    cfgDailyDaysMask = 0;
    for (uint8_t i = 0; i < 7; i++) {
        cfgDailyStartMin[i] = 8 * 60;
        cfgDailyEndMin[i] = 18 * 60;
    }
    cfgScheduleSpeed = SPEED_HIGH;
    cfgScheduleSpeedPercent = FAN_HIGH_REFERENCE_PERCENT;
    cfgScheduleLedDimmerPercent = 100;
    cfgScheduleTempRule = SCHEDULE_TEMP_NONE;
    cfgScheduleTempF = 75;
    cfgLedDataPin = PIN_LED_DATA;
    cfgHighDelayUs = DEFAULT_HIGH_DELAY_US;
    cfgLowDelayUs = DEFAULT_LOW_DELAY_US;
    cfgMocPulseUs = DEFAULT_MOC_PULSE_US;
    cfgHystTenthF = 10;
    cfgApSsid = AP_SSID_DEFAULT;
    cfgApPass = AP_PASS_DEFAULT;
    cfgWebUser = WEB_USER_DEFAULT;
    cfgWebPass = WEB_PASS_DEFAULT;
    cfgAdminUser = ADMIN_USER_DEFAULT;
    cfgAdminPass = ADMIN_PASS_DEFAULT;
    cfgStaSsid = STA_SSID_DEFAULT;
    cfgStaPass = STA_PASS_DEFAULT;
    cfgStaStaticEnabled = true;
    cfgStaIp = STA_IP_DEFAULT;
    cfgStaGateway = STA_GATEWAY_DEFAULT;
    cfgStaSubnet = STA_SUBNET_DEFAULT;
    cfgStaDns = STA_DNS_DEFAULT;
    cfgHostname = HOSTNAME_DEFAULT;
    gMode = MODE_OFF_LOOP;
    gMemoryValid = false;
    gMocArmed = false;
    gDurationTimerActive = false;
    gClockTimerActive = false;
    saveConfig();
}

static uint8_t tempLedIndexForSetpoint(int setpointF) {
    if (setpointF == 60) return 1;
    if (setpointF == 65) return 2;
    if (setpointF == 70) return 3;
    if (setpointF == 75) return 5;
    if (setpointF == 80) return 6;
    return 255;
}

static void setPixel(uint8_t idx, Rgb color, uint8_t scale = 255) {
    if (idx >= LED_COUNT) return;
    if (!cfgLedEnabled[idx]) return;
    uint16_t effectiveScale = ((uint16_t)scale * cfgLedDimmerPercent) / 100;
    uint8_t r = (uint16_t)color.r * effectiveScale / 255;
    uint8_t g = (uint16_t)color.g * effectiveScale / 255;
    uint8_t b = (uint16_t)color.b * effectiveScale / 255;
    pixels.setPixelColor(idx, pixels.Color(r, g, b));
}

static void configurePixels() {
    pixels.setPin(cfgLedDataPin);
    pixels.begin();
    pixels.setBrightness(255);
    pixels.clear();
    pixels.show();
    gLedShowCount++;
    gLedLastShowMs = millis();
}

static void renderBoardLed() {
    boardLedPixel.clear();
    if (gBoardLedOn) boardLedPixel.setPixelColor(0, boardLedPixel.Color(255, 0, 0));
    boardLedPixel.show();
}

static void configureBoardLed() {
    gBoardLedOn = false;
    boardLedPixel.setPin(PIN_BOARD_LED);
    boardLedPixel.begin();
    boardLedPixel.setBrightness(64);
    renderBoardLed();
}

static bool ledPreviewActive() {
    if (gLedPreviewMode == LED_PREVIEW_NONE) return false;
    return gLedPreviewUntilMs == 0 || !timeDueUs(millis(), gLedPreviewUntilMs);
}

static bool ledOutputLit(uint8_t idx) {
    if (idx >= LED_COUNT || !cfgLightsOn || !cfgLedEnabled[idx] || cfgLedDimmerPercent == 0) return false;

    if (ledPreviewActive()) {
        if (gLedPreviewMode == LED_PREVIEW_SINGLE) return gLedPreviewIndex == (int8_t)idx;
        if (gLedPreviewMode == LED_PREVIEW_ALL) return true;
        if (gLedPreviewMode == LED_PREVIEW_CHASE) return idx == ((millis() / 220UL) % LED_COUNT);
    }

    const ModeInfo& info = modeInfo(gMode);
    if (info.active) {
        if (info.speed == SPEED_LOW && idx == 0) return true;
        if (info.speed == SPEED_HIGH && idx == 4) return true;
        if (info.speed == SPEED_CUSTOM && (idx == 0 || idx == 4)) return true;
        if (info.thermostat && idx == tempLedIndexForSetpoint(info.setpointF)) return true;
    }
    return gMode == MODE_OFF_MEMORY && gMemoryValid && (idx == 0 || idx == 4);
}

static void clearLedPreview() {
    gLedPreviewMode = LED_PREVIEW_NONE;
    gLedPreviewIndex = -1;
    gLedPreviewUntilMs = 0;
    gLedDataOverride = -1;
}

static void startLedPreviewSingle(uint8_t idx) {
    if (idx >= LED_COUNT) return;
    cfgLightsOn = true;
    gLedDataOverride = -1;
    gLedPreviewMode = LED_PREVIEW_SINGLE;
    gLedPreviewIndex = (int8_t)idx;
    gLedPreviewUntilMs = 0;
}

static void startLedPreviewAll() {
    cfgLightsOn = true;
    gLedDataOverride = -1;
    gLedPreviewMode = LED_PREVIEW_ALL;
    gLedPreviewIndex = -1;
    gLedPreviewUntilMs = 0;
}

static void startLedPreviewChase() {
    cfgLightsOn = true;
    gLedDataOverride = -1;
    gLedPreviewMode = LED_PREVIEW_CHASE;
    gLedPreviewIndex = -1;
    gLedPreviewUntilMs = 0;
}

static void renderLeds() {
    if (gLedDataOverride >= 0) {
        gpio_set_direction((gpio_num_t)cfgLedDataPin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)cfgLedDataPin, gLedDataOverride ? 1 : 0);
        return;
    }
    if (gLedPreviewMode != LED_PREVIEW_NONE && !ledPreviewActive()) clearLedPreview();
    pixels.clear();
    const bool memoryDimmed = gLedPreviewMode == LED_PREVIEW_NONE && gMode == MODE_OFF_MEMORY && gMemoryValid;
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        if (ledOutputLit(i)) setPixel(i, cfgLed[i], memoryDimmed ? 28 : 255);
    }
    pixels.show();
    gLedShowCount++;
    gLedLastShowMs = millis();
}

static void forceMocOff() {
    portENTER_CRITICAL(&gTriacFireMux);
    vTriacFireEnabled = false;
    gpio_set_level((gpio_num_t)PIN_TRIAC_TRIGGER, 0);
    gMocPulseActive = false;
    gLastFireUs = 0;
    gFireLastGapUs = 0;
    gFireMaxGapUs = 0;
    portEXIT_CRITICAL(&gTriacFireMux);
}

static uint8_t clampCustomSpeedPercent(int value) {
    return (uint8_t)constrain(value, FAN_CUSTOM_MIN_PERCENT, FAN_CUSTOM_MAX_PERCENT);
}

static uint16_t customDelayUsForPercent(uint8_t percent) {
    percent = clampCustomSpeedPercent(percent);
    if (FAN_HIGH_REFERENCE_PERCENT <= FAN_LOW_REFERENCE_PERCENT) return cfgHighDelayUs;
    uint16_t span = cfgLowDelayUs > cfgHighDelayUs ? cfgLowDelayUs - cfgHighDelayUs : 0;
    uint8_t belowHigh = FAN_HIGH_REFERENCE_PERCENT - percent;
    uint8_t range = FAN_HIGH_REFERENCE_PERCENT - FAN_LOW_REFERENCE_PERCENT;
    return cfgHighDelayUs + ((uint32_t)span * belowHigh) / range;
}

static bool zcFresh() {
    uint32_t lastRise;
    noInterrupts();
    lastRise = vZcLastRiseUs;
    interrupts();
    return lastRise != 0 && (uint32_t)(micros() - lastRise) < ZC_FRESH_US;
}

static bool fireAllowed() {
    return gMocArmed && gMotorShouldRun && gZcStable && zcFresh() && !gTempSafetyFault;
}

static uint16_t currentFireDelayUs() {
    if (gStartBoostActive) {
        if (!timeDueUs(millis(), gStartBoostUntilMs)) return cfgHighDelayUs;
        gStartBoostActive = false;
    }
    if (gCurrentSpeed == SPEED_LOW) return cfgLowDelayUs;
    if (gCurrentSpeed == SPEED_HIGH) return cfgHighDelayUs;
    if (gCurrentSpeed == SPEED_CUSTOM) return customDelayUsForPercent(cfgCustomSpeedPercent);
    return 0;
}

static void syncTriacFireSnapshot() {
    bool enabled = fireAllowed() && gTriacFireTaskReady;
    uint16_t delayUs = enabled ? currentFireDelayUs() : cfgHighDelayUs;
    portENTER_CRITICAL(&gTriacFireMux);
    vTriacFireDelayUs = delayUs;
    vTriacPulseUs = cfgMocPulseUs;
    vTriacFireEnabled = enabled;
    if (!enabled) {
        gpio_set_level((gpio_num_t)PIN_TRIAC_TRIGGER, 0);
        gMocPulseActive = false;
    }
    portEXIT_CRITICAL(&gTriacFireMux);
}

static void servicePowerDiagnostics() {
    if (!fireAllowed() || !gTriacFireTaskReady) {
        gFireGapLogged = false;
        return;
    }
    uint32_t maxGapUs = gFireMaxGapUs;
    if (!gFireGapLogged && maxGapUs >= FIRE_GAP_WARNING_US) {
        gFireGapLogged = true;
        recordPowerEvent(POWER_EVENT_OUTPUT_GAP, POWER_CAUSE_FIRE_GAP, maxGapUs,
                         gMode, speedPercentForMode(gMode));
    }
}

static void triacFireTask(void*) {
    gTriacFireTaskReady = true;
    for (;;) {
        gTriacTaskStage = 1;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        gTriacTaskStage = 2;
        gTriacWakeCount++;
        gTriacLastWakeMs = millis();

        uint32_t riseUs = vZcLastRiseUs;
        uint32_t pulseWidthUs = vZcPulseWidthUs;
        uint16_t delayUs;
        uint16_t pulseUs;
        bool enabled;
        portENTER_CRITICAL(&gTriacFireMux);
        enabled = vTriacFireEnabled;
        delayUs = vTriacFireDelayUs;
        pulseUs = vTriacPulseUs;
        portEXIT_CRITICAL(&gTriacFireMux);
        if (!enabled) continue;

        uint32_t centerOffsetUs = pulseWidthUs > 0 ? pulseWidthUs / 2 : 800;
        uint32_t fireAtUs = riseUs + centerOffsetUs + delayUs;
        gTriacTaskStage = 3;
        for (;;) {
            uint32_t nowUs = micros();
            if (timeDueUs(nowUs, fireAtUs)) break;
            uint32_t remainingUs = fireAtUs - nowUs;
            if (remainingUs > 1500) {
                vTaskDelay(pdMS_TO_TICKS((remainingUs - 500) / 1000));
            } else if (remainingUs > 40) {
                esp_rom_delay_us(20);
            }
        }

        uint32_t firedUs = micros();
        portENTER_CRITICAL(&gTriacFireMux);
        if (!vTriacFireEnabled) {
            portEXIT_CRITICAL(&gTriacFireMux);
            continue;
        }
        gpio_set_level((gpio_num_t)PIN_TRIAC_TRIGGER, 1);
        gTriacTaskStage = 4;
        gMocPulseActive = true;
        if (gLastFireUs != 0) {
            gFireLastGapUs = firedUs - gLastFireUs;
            if (gFireLastGapUs > gFireMaxGapUs) gFireMaxGapUs = gFireLastGapUs;
        }
        gLastFireUs = firedUs;
        gFirePulseCount++;
        gLastFireMs = millis();
        portEXIT_CRITICAL(&gTriacFireMux);

        esp_rom_delay_us(pulseUs);
        portENTER_CRITICAL(&gTriacFireMux);
        gpio_set_level((gpio_num_t)PIN_TRIAC_TRIGGER, 0);
        gMocPulseActive = false;
        gTriacTaskStage = 5;
        portEXIT_CRITICAL(&gTriacFireMux);
    }
}

static uint8_t owReadPin() {
    uint8_t level = gpio_get_level((gpio_num_t)PIN_TEMP) ? 1 : 0;
    gTempRawLevel = level;
    return level ? HIGH : LOW;
}

static void owBusInit() {
    gpio_set_direction((gpio_num_t)PIN_TEMP, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode((gpio_num_t)PIN_TEMP, GPIO_PULLUP_ONLY);
    gpio_set_level((gpio_num_t)PIN_TEMP, 1);
    gTempRawLevel = gpio_get_level((gpio_num_t)PIN_TEMP) ? 1 : 0;
}

static void owRelease() {
    gpio_set_level((gpio_num_t)PIN_TEMP, 1);
}

static void owLow() {
    gpio_set_level((gpio_num_t)PIN_TEMP, 0);
}

static bool owReset() {
    gTempLastPresence = false;
    gTempPresenceDelayUs = 0;
    owLow();
    delayMicroseconds(500);
    owRelease();
    delayMicroseconds(15);
    for (uint16_t waited = 15; waited <= 260; waited += 5) {
        delayMicroseconds(5);
        if (owReadPin() == LOW) {
            gTempLastPresence = true;
            gTempPresenceDelayUs = waited;
            break;
        }
    }
    delayMicroseconds(260);
    if (gTempLastPresence) gTempResetOkCount++;
    else gTempResetFailCount++;
    return gTempLastPresence;
}

static void owWriteBit(bool bitValue) {
    noInterrupts();
    owLow();
    if (bitValue) {
        delayMicroseconds(6);
        owRelease();
        interrupts();
        delayMicroseconds(64);
    } else {
        delayMicroseconds(60);
        owRelease();
        interrupts();
        delayMicroseconds(10);
    }
}

static bool owReadBit() {
    bool bitValue;
    noInterrupts();
    owLow();
    delayMicroseconds(6);
    owRelease();
    delayMicroseconds(9);
    bitValue = owReadPin() == HIGH;
    interrupts();
    delayMicroseconds(55);
    return bitValue;
}

static void owWriteByte(uint8_t value) {
    for (uint8_t i = 0; i < 8; i++) {
        owWriteBit(value & 0x01);
        value >>= 1;
    }
}

static uint8_t owReadByte() {
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (owReadBit()) value |= (1 << i);
    }
    return value;
}

static uint8_t owCrc8(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *data++;
        for (uint8_t i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

static bool ds18b20StartConversion() {
    if (!owReset()) {
        gTempPresent = false;
        gTempReadErrors++;
        return false;
    }
    owWriteByte(0xCC); // Skip ROM, single-sensor bus.
    owWriteByte(0x44); // Convert T.
    gTempPending = true;
    gTempRequestMs = millis();
    return true;
}

static bool ds18b20ReadScratchpad() {
    uint8_t scratch[9];
    if (!owReset()) {
        gTempPresent = false;
        gTempReadErrors++;
        return false;
    }
    owWriteByte(0xCC);
    owWriteByte(0xBE);
    for (uint8_t i = 0; i < sizeof(scratch); i++) scratch[i] = owReadByte();
    if (owCrc8(scratch, 8) != scratch[8]) {
        gTempPresent = false;
        gTempCrcErrors++;
        gTempReadErrors++;
        return false;
    }
    int16_t raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    gTempC = (float)raw / 16.0f;
    gTempF = gTempC * 9.0f / 5.0f + 32.0f;
    gTempPresent = true;
    gTempLastReadMs = millis();
    recordTempHistory();
    return true;
}

static void serviceTemperature() {
    uint32_t now = millis();
    if (gTempPending) {
        if (now - gTempRequestMs >= 750) {
            ds18b20ReadScratchpad();
            gTempPending = false;
        }
        return;
    }
    if (now - gTempRequestMs >= 2000) {
        ds18b20StartConversion();
    }
}

static void setMode(FanMode mode, PowerEventCause cause = POWER_CAUSE_INTERNAL) {
    if ((uint8_t)mode >= (sizeof(MODE_INFO) / sizeof(MODE_INFO[0]))) mode = MODE_OFF_LOOP;
    gMode = mode;
    gPendingMotorCause = cause;
    if (!modeInfo(mode).thermostat) gThermostatCalling = false;
    if (!modeIsActive(mode)) forceMocOff();
}

static void setModeFromWeb(FanMode mode, PowerEventCause cause = POWER_CAUSE_WEB_FAN) {
    setMode(mode, cause);
    if (modeIsActive(gMode)) {
        gMocArmed = true;
    } else {
        gMocArmed = false;
        forceMocOff();
    }
}

static void setModeFromButton(FanMode mode, PowerEventCause cause) {
    setMode(mode, cause);
    if (modeIsActive(gMode)) {
        if (cfgButtonEnabled) {
            gMocArmed = true;
        } else {
            gMocArmed = false;
            forceMocOff();
        }
    } else {
        gMocArmed = false;
        forceMocOff();
    }
}

static void restoreScheduleOverrides() {
    if (!gScheduleOverridesApplied) return;
    cfgCustomSpeedPercent = gSchedulePreviousSpeedPercent;
    cfgLedDimmerPercent = gSchedulePreviousLedDimmerPercent;
    gScheduleOverridesApplied = false;
    renderLeds();
}

static void cancelScheduleDrive() {
    restoreScheduleOverrides();
    gScheduleDriving = false;
}

static void fullPowerOff(PowerEventCause cause = POWER_CAUSE_INTERNAL, int previousPercentOverride = -1) {
    FanMode previousMode = gMode;
    uint8_t previousPercent = previousPercentOverride >= 0 ?
                              constrain(previousPercentOverride, 0, 100) : speedPercentForMode(previousMode);
    bool wasActive = gMotorShouldRun || modeIsActive(previousMode);
    cancelScheduleDrive();
    gMode = MODE_OFF_LOOP;
    gThermostatCalling = false;
    gMotorShouldRun = false;
    gCurrentSpeed = SPEED_OFF;
    gMotorWasRunning = false;
    gStartBoostActive = false;
    gMocArmed = false;
    gDurationTimerActive = false;
    gClockTimerActive = false;
    forceMocOff();
    gPendingMotorCause = POWER_CAUSE_INTERNAL;
    if (wasActive) {
        recordPowerEvent(POWER_EVENT_MOTOR_OFF, cause, 0, previousMode, previousPercent);
    }
}

static FanMode modeFromOriginalChoice(uint8_t speed, int setpointF) {
    if (speed != SPEED_LOW && speed != SPEED_HIGH) speed = SPEED_HIGH;
    if (setpointF == 0) return speed == SPEED_LOW ? MODE_LOW_CONT : MODE_HIGH_CONT;
    if (speed == SPEED_HIGH) {
        if (setpointF == 60) return MODE_HIGH_60;
        if (setpointF == 65) return MODE_HIGH_65;
        if (setpointF == 70) return MODE_HIGH_70;
        if (setpointF == 75) return MODE_HIGH_75;
        if (setpointF == 80) return MODE_HIGH_80;
    } else {
        if (setpointF == 60) return MODE_LOW_60;
        if (setpointF == 65) return MODE_LOW_65;
        if (setpointF == 70) return MODE_LOW_70;
        if (setpointF == 75) return MODE_LOW_75;
        if (setpointF == 80) return MODE_LOW_80;
    }
    return speed == SPEED_LOW ? MODE_LOW_CONT : MODE_HIGH_CONT;
}

static FanMode nearestOriginalModeForCustom() {
    uint8_t midpoint = (FAN_LOW_REFERENCE_PERCENT + FAN_HIGH_REFERENCE_PERCENT + 1) / 2;
    return cfgCustomSpeedPercent >= midpoint ? MODE_HIGH_CONT : MODE_LOW_CONT;
}

static FanMode nextOriginalLoopMode(FanMode mode) {
    uint8_t next = (uint8_t)mode + 1;
    if (next > (uint8_t)MODE_LOW_80) next = MODE_OFF_LOOP;
    return (FanMode)next;
}

static void buttonCycleOriginal() {
    cancelScheduleDrive();
    if (gMode == MODE_OFF_MEMORY && gMemoryValid) {
        cfgLightsOn = true;
        setModeFromButton(gMemoryMode, POWER_CAUSE_BUTTON_SHORT);
        return;
    }

    if (gMode == MODE_OFF_MEMORY) {
        cfgLightsOn = true;
        setModeFromButton(MODE_HIGH_CONT, POWER_CAUSE_BUTTON_SHORT);
        return;
    }

    if (gMode == MODE_CUSTOM) {
        setModeFromButton(MODE_OFF_LOOP, POWER_CAUSE_BUTTON_SHORT);
        return;
    }

    FanMode base = gMode;
    setModeFromButton(nextOriginalLoopMode(base), POWER_CAUSE_BUTTON_SHORT);
}

static void buttonLongOriginal() {
    cancelScheduleDrive();
    if (modeIsActive(gMode)) {
        gMemoryMode = gMode;
        gMemoryValid = true;
        cfgLightsOn = false;
        clearLedPreview();
        setModeFromButton(MODE_OFF_MEMORY, POWER_CAUSE_BUTTON_LONG);
    } else {
        setModeFromButton(MODE_OFF_LOOP, POWER_CAUSE_BUTTON_LONG);
    }
}

static void serviceButton() {
    uint32_t now = millis();
    bool rawPressed = digitalRead(PIN_BUTTON) == LOW;
    if (rawPressed != gButtonLastRawPressed) {
        gButtonLastRawPressed = rawPressed;
        gButtonLastChangeMs = now;
    }
    if (now - gButtonLastChangeMs < BUTTON_DEBOUNCE_MS) return;
    if (!cfgButtonEnabled) {
        gButtonStablePressed = rawPressed;
        gButtonWasPressed = false;
        return;
    }
    if (rawPressed != gButtonStablePressed) {
        gButtonStablePressed = rawPressed;
        if (gButtonStablePressed) {
            gButtonWasPressed = true;
            gButtonPressStartMs = now;
        } else if (gButtonWasPressed) {
            uint32_t held = now - gButtonPressStartMs;
            gButtonWasPressed = false;
            gButtonLastEventMs = now;
            if (held >= BUTTON_LONG_MS) {
                gButtonLongCount++;
                buttonLongOriginal();
            } else {
                gButtonCount++;
                buttonCycleOriginal();
            }
        }
    }
}

static uint32_t localEpochNow() {
    if (!gTimeSynced) return 0;
    return gEpochAtSync + ((millis() - gMillisAtSync) / 1000UL);
}

static uint8_t localWeekdayMondayFirst(uint32_t epoch) {
    int64_t localSeconds = (int64_t)epoch + (int32_t)gTzOffsetMinutes * 60LL;
    if (localSeconds < 0) localSeconds = 0;
    uint32_t day = (uint32_t)(localSeconds / 86400LL);
    return (uint8_t)((day + 3) % 7);
}

static uint16_t localMinuteOfDay(uint32_t epoch) {
    int64_t localSeconds = (int64_t)epoch + (int32_t)gTzOffsetMinutes * 60LL;
    if (localSeconds < 0) localSeconds = 0;
    return (uint16_t)((localSeconds % 86400LL) / 60LL);
}

static bool scheduleIntervalActive(uint8_t startDay, uint16_t startMin, uint16_t endMin,
                                   uint8_t nowDay, uint16_t nowMin) {
    if (startMin == endMin) return false;
    const uint32_t minutesPerWeek = 7UL * MINUTES_PER_DAY;
    uint32_t start = (uint32_t)startDay * MINUTES_PER_DAY + startMin;
    uint32_t duration = endMin > startMin ? (uint32_t)(endMin - startMin) :
                        (uint32_t)(MINUTES_PER_DAY - startMin + endMin);
    uint32_t end = start + duration;
    uint32_t now = (uint32_t)nowDay * MINUTES_PER_DAY + nowMin;
    return (now >= start && now < end) || (now + minutesPerWeek >= start && now + minutesPerWeek < end);
}

static bool scheduleShouldRunNow() {
    if (cfgScheduleMode == SCHEDULE_OFF || !gTimeSynced) return false;
    uint32_t nowEpoch = localEpochNow();
    uint8_t nowDay = localWeekdayMondayFirst(nowEpoch);
    uint16_t nowMin = localMinuteOfDay(nowEpoch);

    if (cfgScheduleMode == SCHEDULE_WEEKLY) {
        for (uint8_t day = 0; day < 7; day++) {
            if ((cfgScheduleDaysMask & (1 << day)) &&
                scheduleIntervalActive(day, cfgScheduleStartMin, cfgScheduleEndMin, nowDay, nowMin)) {
                return true;
            }
        }
        return false;
    }

    for (uint8_t day = 0; day < 7; day++) {
        if ((cfgDailyDaysMask & (1 << day)) &&
            scheduleIntervalActive(day, cfgDailyStartMin[day], cfgDailyEndMin[day], nowDay, nowMin)) {
            return true;
        }
    }
    return false;
}

static void serviceWeeklySchedule() {
    bool activeNow = scheduleShouldRunNow();
    if (activeNow && !gScheduleWindowWasActive) {
        restoreScheduleOverrides();
        gSchedulePreviousSpeedPercent = cfgCustomSpeedPercent;
        gSchedulePreviousLedDimmerPercent = cfgLedDimmerPercent;
        gScheduleOverridesApplied = true;
        cfgCustomSpeedPercent = cfgScheduleSpeedPercent;
        cfgLedDimmerPercent = cfgScheduleLedDimmerPercent;
        FanMode scheduledMode = cfgScheduleSpeed == SPEED_CUSTOM ? MODE_CUSTOM :
                                modeFromOriginalChoice(cfgScheduleSpeed, 0);
        setModeFromWeb(scheduledMode, POWER_CAUSE_SCHEDULE_START);
        gScheduleDriving = true;
        renderLeds();
    } else if (activeNow && gScheduleDriving) {
        if (cfgCustomSpeedPercent != cfgScheduleSpeedPercent ||
            cfgLedDimmerPercent != cfgScheduleLedDimmerPercent) {
            cfgCustomSpeedPercent = cfgScheduleSpeedPercent;
            cfgLedDimmerPercent = cfgScheduleLedDimmerPercent;
            renderLeds();
        }
    } else if (!activeNow && gScheduleWindowWasActive && gScheduleDriving) {
        fullPowerOff(POWER_CAUSE_SCHEDULE_END);
    }
    gScheduleWindowWasActive = activeNow;
}

static void initTempHistory() {
    for (uint8_t i = 0; i < TEMP_HISTORY_DAYS; i++) gTempHistoryDay[i] = 0;
    for (uint16_t i = 0; i < TEMP_HISTORY_DAYS * TEMP_HISTORY_HOURS; i++) {
        gTempHistoryF[i] = TEMP_HISTORY_EMPTY;
    }
}

static void loadTempHistory() {
    initTempHistory();
    if (prefs.getBytesLength("th_day") == sizeof(gTempHistoryDay)) {
        prefs.getBytes("th_day", gTempHistoryDay, sizeof(gTempHistoryDay));
    }
    if (prefs.getBytesLength("th_f") == sizeof(gTempHistoryF)) {
        prefs.getBytes("th_f", gTempHistoryF, sizeof(gTempHistoryF));
    }
}

static void saveTempHistory() {
    gTempHistoryDirty = true;
    serviceTempHistoryPersistence();
}

static int8_t tempHistoryFindDay(uint32_t day) {
    for (uint8_t i = 0; i < TEMP_HISTORY_DAYS; i++) {
        if (gTempHistoryDay[i] == day) return (int8_t)i;
    }
    return -1;
}

static void tempHistoryClearSlot(uint8_t slot, uint32_t day) {
    if (slot >= TEMP_HISTORY_DAYS) return;
    gTempHistoryDay[slot] = day;
    uint16_t start = (uint16_t)slot * TEMP_HISTORY_HOURS;
    for (uint8_t h = 0; h < TEMP_HISTORY_HOURS; h++) {
        gTempHistoryF[start + h] = TEMP_HISTORY_EMPTY;
    }
}

static uint8_t tempHistorySlotForDay(uint32_t day) {
    int8_t existing = tempHistoryFindDay(day);
    if (existing >= 0) return (uint8_t)existing;

    uint8_t oldest = 0;
    for (uint8_t i = 0; i < TEMP_HISTORY_DAYS; i++) {
        if (gTempHistoryDay[i] == 0) {
            tempHistoryClearSlot(i, day);
            return i;
        }
        if (gTempHistoryDay[i] < gTempHistoryDay[oldest]) oldest = i;
    }
    tempHistoryClearSlot(oldest, day);
    return oldest;
}

static void civilFromDays(int64_t z, int& y, uint8_t& m, uint8_t& d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = (uint32_t)(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = (int)yoe + (int)era * 400;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp = (5 * doy + 2) / 153;
    d = (uint8_t)(doy - (153 * mp + 2) / 5 + 1);
    m = (uint8_t)(mp + (mp < 10 ? 3 : -9));
    y += (m <= 2);
}

static String dayLabel(uint32_t day) {
    int y = 1970;
    uint8_t m = 1;
    uint8_t d = 1;
    civilFromDays((int64_t)day, y, m, d);
    char buf[12];
    snprintf(buf, sizeof(buf), "%04d-%02u-%02u", y, (unsigned)m, (unsigned)d);
    return String(buf);
}

static void recordTempHistory() {
    if (!gTimeSynced || !gTempPresent || isnan(gTempF)) return;
    int64_t localSeconds = (int64_t)localEpochNow() + (int32_t)gTzOffsetMinutes * 60LL;
    if (localSeconds < 0) return;
    uint32_t day = (uint32_t)(localSeconds / 86400LL);
    uint8_t hour = (uint8_t)((localSeconds % 86400LL) / 3600LL);
    uint8_t slot = tempHistorySlotForDay(day);
    uint16_t idx = (uint16_t)slot * TEMP_HISTORY_HOURS + hour;
    if (gTempHistoryF[idx] != TEMP_HISTORY_EMPTY) return;
    gTempHistoryF[idx] = (int16_t)constrain((int)lroundf(gTempF * 10.0f), -1000, 2000);
    saveTempHistory();
}

static void appendTempHistoryJson(String& s) {
    uint8_t order[TEMP_HISTORY_DAYS];
    uint8_t count = 0;
    for (uint8_t i = 0; i < TEMP_HISTORY_DAYS; i++) {
        if (gTempHistoryDay[i] != 0) order[count++] = i;
    }
    for (uint8_t i = 0; i < count; i++) {
        for (uint8_t j = i + 1; j < count; j++) {
            if (gTempHistoryDay[order[j]] > gTempHistoryDay[order[i]]) {
                uint8_t tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    s += "{\"tzOffsetMinutes\":" + String(gTzOffsetMinutes) + ",\"days\":[";
    for (uint8_t n = 0; n < count; n++) {
        uint8_t slot = order[n];
        if (n) s += ",";
        s += "{\"day\":" + String(gTempHistoryDay[slot]) +
             ",\"label\":\"" + dayLabel(gTempHistoryDay[slot]) + "\",\"hours\":[";
        uint16_t start = (uint16_t)slot * TEMP_HISTORY_HOURS;
        for (uint8_t h = 0; h < TEMP_HISTORY_HOURS; h++) {
            if (h) s += ",";
            int16_t v = gTempHistoryF[start + h];
            if (v == TEMP_HISTORY_EMPTY) s += "null";
            else s += String((float)v / 10.0f, 1);
        }
        s += "]}";
    }
    s += "]}";
}

static void serviceOffTimers() {
    uint32_t now = millis();
    if (gDurationTimerActive && timeDueUs(now, gDurationOffAtMs)) {
        fullPowerOff(POWER_CAUSE_DURATION_TIMER);
    }
    if (gClockTimerActive && gTimeSynced && localEpochNow() >= gClockOffAtEpoch) {
        fullPowerOff(POWER_CAUSE_CLOCK_TIMER);
    }
}

static void serviceMotorLogic() {
    const ModeInfo& info = modeInfo(gMode);
    bool wasRunning = gMotorShouldRun;
    gTempSafetyFault = false;
    bool shouldRun = false;
    FanSpeed nextSpeed = SPEED_OFF;

    if (!info.active) {
        gThermostatCalling = false;
    } else if (gMode == MODE_CUSTOM) {
        gThermostatCalling = false;
        shouldRun = cfgCustomSpeedEnabled || gScheduleDriving;
        nextSpeed = shouldRun ? SPEED_CUSTOM : SPEED_OFF;
    } else if (!info.thermostat) {
        gThermostatCalling = false;
        shouldRun = true;
        nextSpeed = info.speed;
    } else {
        bool tempValid = gTempPresent && (millis() - gTempLastReadMs < 10000);
        if (!tempValid || isnan(gTempF)) {
            gTempSafetyFault = true;
            gThermostatCalling = false;
        } else {
            float hyst = (float)cfgHystTenthF / 10.0f;
            float onAt = (float)info.setpointF + (hyst * 0.5f);
            float offAt = (float)info.setpointF - (hyst * 0.5f);
            if (!gThermostatCalling && gTempF >= onAt) gThermostatCalling = true;
            if (gThermostatCalling && gTempF <= offAt) gThermostatCalling = false;
            shouldRun = gThermostatCalling;
            nextSpeed = shouldRun ? info.speed : SPEED_OFF;
        }
    }

    if (shouldRun && !gMotorWasRunning) {
        gStartBoostActive = true;
        gStartBoostUntilMs = millis() + FAN_START_BOOST_MS;
    } else if (!shouldRun) {
        gStartBoostActive = false;
    }
    gMotorWasRunning = shouldRun;
    gMotorShouldRun = shouldRun;
    gCurrentSpeed = nextSpeed;
    if (wasRunning != shouldRun) {
        PowerEventCause cause = gPendingMotorCause;
        if (cause == POWER_CAUSE_INTERNAL) {
            if (info.thermostat) {
                cause = shouldRun ? POWER_CAUSE_TEMP_RESUME :
                        (gTempSafetyFault ? POWER_CAUSE_TEMP_SENSOR_FAULT : POWER_CAUSE_TEMP_TARGET);
            } else {
                cause = POWER_CAUSE_INTERNAL;
            }
        }
        recordPowerEvent(shouldRun ? POWER_EVENT_MOTOR_ON : POWER_EVENT_MOTOR_OFF,
                         cause, 0, gMode,
                         shouldRun && nextSpeed == SPEED_CUSTOM ? cfgCustomSpeedPercent : speedPercentForMode(gMode));
    }
    gPendingMotorCause = POWER_CAUSE_INTERNAL;
    if (!gMotorShouldRun) forceMocOff();
}

static const char* sliderState(bool a, bool b) {
    if (a && b) return "INVALID";
    if (a) return "A";
    if (b) return "B";
    return "OFF";
}

static void decodedSliders(bool& saA, bool& saB, bool& sbA, bool& sbB) {
    saA = saB = sbA = sbB = false;
    for (uint8_t i = 0; i < 4; i++) {
        if (!gH11Active[i]) continue;
        if (cfgH11Role[i] == H11_SA_A) sbA = true;
        else if (cfgH11Role[i] == H11_SA_B) sbB = true;
        else if (cfgH11Role[i] == H11_SB_A) saA = true;
        else if (cfgH11Role[i] == H11_SB_B) saB = true;
    }
}

static void servicePulseMetrics() {
    uint32_t nowMs = millis();
    if (gPrevH11SampleMs == 0) gPrevH11SampleMs = nowMs;
    if (gPrevZcSampleMs == 0) gPrevZcSampleMs = nowMs;

    if (nowMs - gPrevH11SampleMs >= PULSE_SAMPLE_MS) {
        uint32_t elapsed = nowMs - gPrevH11SampleMs;
        uint32_t edges[4];
        uint32_t lastUs[4];
        uint8_t level[4];
        noInterrupts();
        for (uint8_t i = 0; i < 4; i++) {
            edges[i] = vH11Edges[i];
            lastUs[i] = vH11LastEdgeUs[i];
            level[i] = vH11Level[i];
        }
        interrupts();
        uint32_t sampleUs = micros();
        for (uint8_t i = 0; i < 4; i++) {
            uint32_t delta = edges[i] - gPrevH11Edges[i];
            gPrevH11Edges[i] = edges[i];
            gH11EdgeHz[i] = ((float)delta * 1000.0f) / (float)elapsed;
            gH11LastAgeMs[i] = lastUs[i] == 0 ? 0xFFFFFFFFUL : (sampleUs - lastUs[i]) / 1000UL;
            gH11Active[i] = delta >= 2 && gH11LastAgeMs[i] < 300;
            (void)level[i];
        }
        gPrevH11SampleMs = nowMs;
    }

    if (nowMs - gPrevZcSampleMs >= ZC_SAMPLE_MS) {
        uint32_t riseCount;
        uint32_t lastRiseUs;
        uint32_t pulseWidthUs;
        uint32_t intervalUs;
        uint32_t longGapUs;
        noInterrupts();
        riseCount = vZcRiseCount;
        lastRiseUs = vZcLastRiseUs;
        pulseWidthUs = vZcPulseWidthUs;
        intervalUs = vZcLastIntervalUs;
        longGapUs = vZcLongGapPendingUs;
        vZcLongGapPendingUs = 0;
        interrupts();
        uint32_t sampleUs = micros();

        uint32_t elapsed = nowMs - gPrevZcSampleMs;
        uint32_t delta = riseCount - gPrevZcRiseCount;
        gPrevZcRiseCount = riseCount;
        gZcHz = ((float)delta * 1000.0f) / (float)elapsed;
        gZcPulseWidthUs = pulseWidthUs;
        gZcIntervalUs = intervalUs;
        gZcLastRiseSnapshotUs = lastRiseUs;
        gZcLastRiseAgeUs = lastRiseUs == 0 ? UINT32_MAX : sampleUs - lastRiseUs;
        gZcSampleElapsedMs = elapsed;
        bool fresh = lastRiseUs != 0 && gZcLastRiseAgeUs < ZC_FRESH_US;
        bool wasStable = gZcStable;
        bool stableNow = fresh && gZcHz >= 90.0f && gZcHz <= 150.0f;
        uint32_t confirmedGapUs = longGapUs >= AC_RECONNECT_GAP_US ? longGapUs :
                                  (gZcLastRiseAgeUs >= AC_RECONNECT_GAP_US ? gZcLastRiseAgeUs : 0);
        if (gAcPresent && confirmedGapUs) {
            gAcPresent = false;
            recordPowerEvent(POWER_EVENT_AC_LOST, POWER_CAUSE_AC_SIGNAL_LOST,
                             confirmedGapUs, gMode, speedPercentForMode(gMode));
            cfgLightsOn = true;
            enableAllLedSession();
            clearLedPreview();
        }
        if (stableNow) {
            if (gZcOutputBlockedLogged && gMotorShouldRun && gMocArmed) {
                recordPowerEvent(POWER_EVENT_OUTPUT_RESTORED, POWER_CAUSE_AC_SIGNAL_LOST,
                                 intervalUs, gMode, speedPercentForMode(gMode));
            }
            gZcOutputBlockedLogged = false;
            if (!gAcPresent) {
                gAcConnectOffCount++;
                recordPowerEvent(POWER_EVENT_AC_PRESENT, POWER_CAUSE_AC_CONNECTED_SAFETY);
                fullPowerOff(POWER_CAUSE_AC_CONNECTED_SAFETY);
            }
            gAcPresent = true;
            gLastAcStableMs = nowMs;
        } else {
            forceMocOff();
        }
        if (wasStable && !stableNow && gMotorShouldRun && gMocArmed) {
            gZcOutputBlockedLogged = true;
            recordPowerEvent(POWER_EVENT_OUTPUT_BLOCKED, POWER_CAUSE_AC_SIGNAL_LOST,
                             gZcLastRiseAgeUs, gMode, speedPercentForMode(gMode));
        }
        gZcStable = stableNow;
        gPrevZcSampleMs = nowMs;
    }
}

static void IRAM_ATTR onZeroCrossChange() {
    uint32_t now = micros();
    bool level = gpio_get_level((gpio_num_t)PIN_ZERO_CROSS);
    vZcLevel = level;
    vZcEdgeCount++;
    vZcLastEdgeUs = now;
    if (level) {
        if (vZcLastRiseUs != 0) {
            uint32_t intervalUs = now - vZcLastRiseUs;
            vZcLastIntervalUs = intervalUs;
            if (intervalUs >= AC_RECONNECT_GAP_US && intervalUs > vZcLongGapPendingUs) {
                vZcLongGapPendingUs = intervalUs;
            }
        }
        vZcLastRiseUs = now;
        vZcRiseCount++;
        vZcPulseStartUs = now;
        BaseType_t higherPriorityWoken = pdFALSE;
        if (gTriacFireTaskHandle != nullptr) {
            gTriacNotifyCount++;
            vTaskNotifyGiveFromISR(gTriacFireTaskHandle, &higherPriorityWoken);
            if (higherPriorityWoken == pdTRUE) portYIELD_FROM_ISR();
        }
    } else {
        if (vZcPulseStartUs != 0) vZcPulseWidthUs = now - vZcPulseStartUs;
    }
}

static void IRAM_ATTR onH11_1() {
    vH11Edges[0]++;
    vH11LastEdgeUs[0] = micros();
    vH11Level[0] = gpio_get_level((gpio_num_t)PIN_H11_1);
}

static void IRAM_ATTR onH11_2() {
    vH11Edges[1]++;
    vH11LastEdgeUs[1] = micros();
    vH11Level[1] = gpio_get_level((gpio_num_t)PIN_H11_2);
}

static void IRAM_ATTR onH11_3() {
    vH11Edges[2]++;
    vH11LastEdgeUs[2] = micros();
    vH11Level[2] = gpio_get_level((gpio_num_t)PIN_H11_3);
}

static void IRAM_ATTR onH11_4() {
    vH11Edges[3]++;
    vH11LastEdgeUs[3] = micros();
    vH11Level[3] = gpio_get_level((gpio_num_t)PIN_H11_4);
}

static void startWifi() {
    if (gMdnsStarted) {
        MDNS.end();
        gMdnsStarted = false;
    }

    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.disconnect(false, false);
    delay(50);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname(cfgHostname.c_str());
    WiFi.softAP(cfgApSsid.c_str(), cfgApPass.c_str(), 1, false, 4);

    gStaStarted = cfgStaSsid.length() > 0;
    if (!gStaStarted) {
        Serial0.printf("[WIFI] AP only %s at %s\n",
                       cfgApSsid.c_str(), WiFi.softAPIP().toString().c_str());
        return;
    }

    if (cfgStaStaticEnabled) {
        IPAddress ip, gw, subnet, dns;
        if (parseIpString(cfgStaIp, ip) && parseIpString(cfgStaGateway, gw) &&
            parseIpString(cfgStaSubnet, subnet) && parseIpString(cfgStaDns, dns)) {
            WiFi.config(ip, gw, subnet, dns);
        }
    } else {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }
    WiFi.begin(cfgStaSsid.c_str(), cfgStaPass.length() ? cfgStaPass.c_str() : nullptr);
    Serial0.printf("[WIFI] AP %s at %s; STA connecting to %s hostname=%s\n",
                   cfgApSsid.c_str(), WiFi.softAPIP().toString().c_str(),
                   cfgStaSsid.c_str(), cfgHostname.c_str());
}

static void serviceWifi() {
    uint32_t now = millis();
    if (gWifiRestartAtMs && timeDueUs(now, gWifiRestartAtMs)) {
        gWifiRestartAtMs = 0;
        startWifi();
        return;
    }
    if (now - gLastWifiServiceMs < 2000) return;
    gLastWifiServiceMs = now;
    if (!gStaStarted) return;
    if (WiFi.status() == WL_CONNECTED) {
        if (!gMdnsStarted && MDNS.begin(cfgHostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            gMdnsStarted = true;
            Serial0.printf("[WIFI] STA %s mDNS http://%s.local\n",
                           WiFi.localIP().toString().c_str(), cfgHostname.c_str());
        }
    } else {
        if (gMdnsStarted) {
            MDNS.end();
            gMdnsStarted = false;
        }
    }
}

static bool requestHasWebSession() {
    if (gSessionToken.length()) {
        String cookie = webServer.header("Cookie");
        String needle = String("fan_auth=") + gSessionToken;
        if (cookie.indexOf(needle) >= 0) return true;
    }
    return false;
}

static bool webAuthorized() {
    String uri = webServer.uri();
    if (uri == "/" || uri == "/index.html" || uri == "/api/login" ||
        uri == "/api/session" || uri == "/api/logout") {
        return true;
    }
    if (requestHasWebSession()) return true;
    if (cfgWebUser.length() == 0 || cfgWebPass.length() == 0) return true;
    webServer.send(401, "application/json", "{\"ok\":false,\"msg\":\"app-login-required\"}");
    return false;
}

static bool webAdminAuthorized(bool challenge = true) {
    if (cfgAdminUser.length() == 0 || cfgAdminPass.length() == 0) return true;
    if (webServer.authenticate(cfgAdminUser.c_str(), cfgAdminPass.c_str())) return true;
    if (challenge) {
        webServer.requestAuthentication(BASIC_AUTH, "Holmes HWF0910AT Admin",
                                        "{\"ok\":false,\"msg\":\"admin-auth-required\"}");
    } else {
        webServer.send(401, "application/json", "{\"ok\":false,\"msg\":\"admin-auth-required\"}");
    }
    return false;
}

static void webOk(const char* msg) {
    webServer.send(200, "application/json", String("{\"ok\":true,\"msg\":\"") + msg + "\"}");
}

static void webPostLogin() {
    String body = webServer.arg("plain");
    String user = jsonString(body, "user", "");
    String pass = jsonString(body, "pass", "");
    if (user == cfgWebUser && pass == cfgWebPass) {
        String cookie = String("fan_auth=") + gSessionToken + "; Path=/; SameSite=Lax; HttpOnly";
        if (jsonInt(body, "remember", 0)) cookie += "; Max-Age=31536000";
        webServer.sendHeader("Set-Cookie", cookie);
        webOk("login");
        return;
    }
    webServer.send(401, "application/json", "{\"ok\":false,\"msg\":\"bad-login\"}");
}

static void webSession() {
    recordPowerEvent(POWER_EVENT_APP_SESSION, POWER_CAUSE_APP_SESSION, 0, gMode,
                     speedPercentForMode(gMode));
    String s = "{\"ok\":true,\"loggedIn\":" + jsonBool(requestHasWebSession() || cfgWebUser.length() == 0 || cfgWebPass.length() == 0) +
               ",\"user\":\"" + jsonEscape(cfgWebUser) + "\"}";
    webServer.send(200, "application/json", s);
}

static void webPostLogout() {
    webServer.sendHeader("Set-Cookie", "fan_auth=; Path=/; Max-Age=0; SameSite=Lax; HttpOnly");
    webOk("logout");
}

static String fireLockReason() {
    if (!gMocArmed) return "moc-disarmed";
    if (!gMotorShouldRun) return "motor-not-requested";
    if (gTempSafetyFault) return "temperature-fault";
    if (!gZcStable || !zcFresh()) return "zc-not-stable";
    return "ready";
}

static void appendLedJson(String& s) {
    s += "[";
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        if (i) s += ",";
        s += "{\"index\":" + String(i) +
             ",\"label\":\"" + String(LED_LABELS[i]) +
             "\",\"r\":" + String(cfgLed[i].r) +
             ",\"g\":" + String(cfgLed[i].g) +
             ",\"b\":" + String(cfgLed[i].b) +
             ",\"enabled\":" + jsonBool(cfgLedEnabled[i]) +
             ",\"lit\":" + jsonBool(ledOutputLit(i)) + "}";
    }
    s += "]";
}

static void appendH11Json(String& s) {
    uint8_t levels[4];
    noInterrupts();
    for (uint8_t i = 0; i < 4; i++) levels[i] = vH11Level[i];
    interrupts();
    s += "[";
    for (uint8_t i = 0; i < 4; i++) {
        if (i) s += ",";
        s += "{\"index\":" + String(i) +
             ",\"pin\":" + String(i == 0 ? PIN_H11_1 : i == 1 ? PIN_H11_2 : i == 2 ? PIN_H11_3 : PIN_H11_4) +
             ",\"role\":" + String(cfgH11Role[i]) +
             ",\"roleLabel\":\"" + String(H11_ROLE_LABELS[cfgH11Role[i]]) +
             "\",\"level\":" + String((int)levels[i]) +
             ",\"active\":" + jsonBool(gH11Active[i]) +
             ",\"edgeHz\":" + String(gH11EdgeHz[i], 1) +
             ",\"ageMs\":" + String(gH11LastAgeMs[i]) + "}";
    }
    s += "]";
}

static void appendH11ModulesJson(String& s) {
    uint8_t zcLevel;
    uint32_t zcLastRiseUs;
    uint8_t levels[4];
    noInterrupts();
    zcLevel = vZcLevel ? 1 : 0;
    zcLastRiseUs = vZcLastRiseUs;
    for (uint8_t i = 0; i < 4; i++) levels[i] = vH11Level[i];
    interrupts();

    uint32_t nowUs = micros();
    uint32_t zcAgeMs = zcLastRiseUs == 0 ? 0xFFFFFFFFUL : (nowUs - zcLastRiseUs) / 1000UL;
    s += "[";
    s += "{\"index\":0,\"pin\":" + String(PIN_ZERO_CROSS) +
         ",\"roleLabel\":\"ZC\",\"function\":\"zero-cross timing\"" +
         ",\"level\":" + String((int)zcLevel) +
         ",\"active\":" + jsonBool(gZcStable && zcFresh()) +
         ",\"edgeHz\":" + String(gZcHz, 1) +
         ",\"ageMs\":" + String(zcAgeMs) + "}";
    for (uint8_t i = 0; i < 4; i++) {
        s += ",";
        s += "{\"index\":" + String(i + 1) +
             ",\"pin\":" + String(i == 0 ? PIN_H11_1 : i == 1 ? PIN_H11_2 : i == 2 ? PIN_H11_3 : PIN_H11_4) +
             ",\"roleLabel\":\"" + String(H11_ROLE_LABELS[cfgH11Role[i]]) +
             "\",\"function\":\"slider sense\"" +
             ",\"level\":" + String((int)levels[i]) +
             ",\"active\":" + jsonBool(gH11Active[i]) +
             ",\"edgeHz\":" + String(gH11EdgeHz[i], 1) +
             ",\"ageMs\":" + String(gH11LastAgeMs[i]) + "}";
    }
    s += "]";
}

static const esp_partition_t* littleFsPartition() {
    const esp_partition_t* part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, nullptr);
    if (!part) {
        part = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    }
    return part;
}

static void appendPartitionJson(String& s, const esp_partition_t* part) {
    if (!part) {
        s += "null";
        return;
    }
    s += "{\"label\":\"" + String(part->label) +
         "\",\"address\":" + String((uint32_t)part->address) +
         ",\"size\":" + String((uint32_t)part->size) + "}";
}

static void appendWifiJson(String& s) {
    bool staConnected = WiFi.status() == WL_CONNECTED;
    String liveGateway = staConnected ? WiFi.gatewayIP().toString() : String("");
    String liveSubnet = staConnected ? WiFi.subnetMask().toString() : String("");
    String liveDns = staConnected ? WiFi.dnsIP().toString() : String("");
    s += "{\"apSsid\":\"" + jsonEscape(cfgApSsid) +
         "\",\"apIp\":\"" + WiFi.softAPIP().toString() +
         "\",\"staEnabled\":" + jsonBool(cfgStaSsid.length() > 0) +
         ",\"staSsid\":\"" + jsonEscape(cfgStaSsid) +
         "\",\"staConnected\":" + jsonBool(staConnected) +
         ",\"staIp\":\"" + (staConnected ? WiFi.localIP().toString() : String("")) +
         "\",\"staGateway\":\"" + liveGateway +
         "\",\"staSubnet\":\"" + liveSubnet +
         "\",\"staDns\":\"" + liveDns +
         "\",\"staStatic\":" + jsonBool(cfgStaStaticEnabled) +
         ",\"staticIp\":\"" + jsonEscape(cfgStaIp) +
         "\",\"gateway\":\"" + jsonEscape(cfgStaGateway) +
         "\",\"subnet\":\"" + jsonEscape(cfgStaSubnet) +
         "\",\"dns\":\"" + jsonEscape(cfgStaDns) +
         "\",\"hostname\":\"" + jsonEscape(cfgHostname) +
         "\",\"mdns\":" + jsonBool(gMdnsStarted) +
         ",\"rssi\":" + String(staConnected ? WiFi.RSSI() : 0) + "}";
}

static void appendOtaJson(String& s) {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* update = esp_ota_get_next_update_partition(nullptr);
    const esp_partition_t* fs = littleFsPartition();
    s += "{\"running\":";
    appendPartitionJson(s, running);
    s += ",\"next\":";
    appendPartitionJson(s, update);
    s += ",\"fs\":";
    appendPartitionJson(s, fs);
    s += ",\"ready\":" + jsonBool(update && running && update != running) +
         ",\"active\":" + jsonBool(gUpdateActive) +
         ",\"ok\":" + jsonBool(gUpdateOk) +
         ",\"kind\":\"" + jsonEscape(gUpdateKind) +
         "\",\"bytes\":" + String((uint32_t)gUpdateBytes) +
         ",\"total\":" + String((uint32_t)gUpdateTotal) +
         ",\"error\":\"" + jsonEscape(gUpdateError) +
         "\",\"rebootInMs\":" + String(gRebootAtMs ? (int32_t)(gRebootAtMs - millis()) : 0) + "}";
}

static void appendScheduleJson(String& s) {
    bool activeNow = scheduleShouldRunNow();
    s += "{\"enabled\":" + jsonBool(cfgScheduleEnabled) +
         ",\"mode\":" + String((int)cfgScheduleMode) +
         ",\"daysMask\":" + String((int)cfgScheduleDaysMask) +
         ",\"startMin\":" + String((int)cfgScheduleStartMin) +
         ",\"endMin\":" + String((int)cfgScheduleEndMin) +
         ",\"valid\":" + jsonBool(cfgScheduleStartMin != cfgScheduleEndMin) +
         ",\"dailyDaysMask\":" + String((int)cfgDailyDaysMask) +
         ",\"daily\":[";
    for (uint8_t i = 0; i < 7; i++) {
        if (i) s += ",";
        s += "{\"enabled\":" + jsonBool((cfgDailyDaysMask & (1 << i)) != 0) +
             ",\"startMin\":" + String((int)cfgDailyStartMin[i]) +
             ",\"endMin\":" + String((int)cfgDailyEndMin[i]) +
             ",\"valid\":" + jsonBool(cfgDailyStartMin[i] != cfgDailyEndMin[i]) + "}";
    }
    s += "],\"speed\":" + String((int)cfgScheduleSpeed) +
         ",\"speedPercent\":" + String((int)cfgScheduleSpeedPercent) +
         ",\"ledDimmerPercent\":" + String((int)cfgScheduleLedDimmerPercent) +
         ",\"tempRule\":" + String((int)cfgScheduleTempRule) +
         ",\"tempF\":" + String((int)cfgScheduleTempF) +
         ",\"active\":" + jsonBool(activeNow) +
         ",\"driving\":" + jsonBool(gScheduleDriving) + "}";
}

static void webStatus() {
    if (!webAuthorized()) return;
    bool saA, saB, sbA, sbB;
    decodedSliders(saA, saB, sbA, sbB);
    const ModeInfo& info = modeInfo(gMode);
    String s;
    s.reserve(7000);
    s += "{";
    s += "\"name\":\"" + String(FW_NAME) + "\",";
    s += "\"version\":\"" + String(FW_VERSION) + "\",";
    s += "\"uptimeMs\":" + String(millis()) + ",";
    s += "\"mode\":" + String((int)gMode) + ",";
    s += "\"modeId\":\"" + String(info.id) + "\",";
    s += "\"modeLabel\":\"" + String(info.label) + "\",";
    s += "\"speed\":\"" + String(speedName(gCurrentSpeed)) + "\",";
    s += "\"speedPercent\":" + String(gCurrentSpeed == SPEED_CUSTOM ? cfgCustomSpeedPercent :
                                      gCurrentSpeed == SPEED_HIGH ? FAN_HIGH_REFERENCE_PERCENT :
                                      gCurrentSpeed == SPEED_LOW ? FAN_LOW_REFERENCE_PERCENT : 0) + ",";
    s += "\"motorRequested\":" + jsonBool(gMotorShouldRun) + ",";
    s += "\"thermostatCalling\":" + jsonBool(gThermostatCalling) + ",";
    s += "\"tempSafetyFault\":" + jsonBool(gTempSafetyFault) + ",";
    s += "\"mocArmed\":" + jsonBool(gMocArmed) + ",";
    s += "\"fireAllowed\":" + jsonBool(fireAllowed()) + ",";
    s += "\"fireLock\":\"" + fireLockReason() + "\",";
    s += "\"mocPulseActive\":" + jsonBool(gMocPulseActive) + ",";
    s += "\"firePulseCount\":" + String(gFirePulseCount) + ",";
    s += "\"lastFireMs\":" + String(gLastFireMs) + ",";
    s += "\"fireTaskReady\":" + jsonBool(gTriacFireTaskReady) + ",";
    s += "\"fireLastGapUs\":" + String(gFireLastGapUs) + ",";
    s += "\"fireMaxGapUs\":" + String(gFireMaxGapUs) + ",";
    s += "\"fireTaskEnabled\":" + jsonBool(vTriacFireEnabled) + ",";
    s += "\"fireTaskNotifyCount\":" + String(gTriacNotifyCount) + ",";
    s += "\"fireTaskWakeCount\":" + String(gTriacWakeCount) + ",";
    s += "\"fireTaskLastWakeMs\":" + String(gTriacLastWakeMs) + ",";
    s += "\"fireTaskStage\":" + String((int)gTriacTaskStage) + ",";
    s += "\"fireTaskStackWords\":" + String(gTriacFireTaskHandle ? uxTaskGetStackHighWaterMark(gTriacFireTaskHandle) : 0) + ",";
    s += "\"fanControl\":{\"customEnabled\":" + jsonBool(cfgCustomSpeedEnabled) +
         ",\"customSpeedPercent\":" + String(cfgCustomSpeedPercent) +
         ",\"customDelayUs\":" + String(customDelayUsForPercent(cfgCustomSpeedPercent)) +
         ",\"highDelayUs\":" + String(cfgHighDelayUs) +
         ",\"lowDelayUs\":" + String(cfgLowDelayUs) +
         ",\"startBoostActive\":" + jsonBool(gStartBoostActive && !timeDueUs(millis(), gStartBoostUntilMs)) +
         ",\"startBoostRemainingMs\":" + String(gStartBoostActive && !timeDueUs(millis(), gStartBoostUntilMs) ? (uint32_t)(gStartBoostUntilMs - millis()) : 0) +
         ",\"startBoostMs\":" + String(FAN_START_BOOST_MS) +
         ",\"buttonEnabled\":" + jsonBool(cfgButtonEnabled) + "},";
	    s += "\"lightsOn\":" + jsonBool(cfgLightsOn) + ",";
	    s += "\"ledDimmerPercent\":" + String((int)cfgLedDimmerPercent) + ",";
	    s += "\"ledPreview\":{\"active\":" + jsonBool(ledPreviewActive()) +
         ",\"mode\":" + String((int)gLedPreviewMode) +
         ",\"index\":" + String((int)gLedPreviewIndex) +
         ",\"hold\":" + jsonBool(gLedPreviewUntilMs == 0 && gLedPreviewMode != LED_PREVIEW_NONE) +
         ",\"remainingMs\":" + String(ledPreviewActive() && gLedPreviewUntilMs != 0 ? (uint32_t)(gLedPreviewUntilMs - millis()) : 0) +
         ",\"dataPin\":" + String((int)cfgLedDataPin) +
         ",\"dataLevel\":" + String((int)gpio_get_level((gpio_num_t)cfgLedDataPin)) +
         ",\"dataOverride\":" + String((int)gLedDataOverride) +
         ",\"showCount\":" + String(gLedShowCount) +
         ",\"lastShowMs\":" + String(gLedLastShowMs) + "},";
    s += "\"boardLed\":{\"pin\":" + String(PIN_BOARD_LED) +
         ",\"on\":" + jsonBool(gBoardLedOn) +
         ",\"kind\":\"gpio48-neopixel\"},";
    s += "\"zc\":{\"pin\":" + String(PIN_ZERO_CROSS) +
         ",\"stable\":" + jsonBool(gZcStable) +
         ",\"hz\":" + String(gZcHz, 1) +
         ",\"pulseWidthUs\":" + String(gZcPulseWidthUs) +
         ",\"intervalUs\":" + String(gZcIntervalUs) +
         ",\"ageUs\":" + String(gZcLastRiseAgeUs) +
         ",\"sampleElapsedMs\":" + String(gZcSampleElapsedMs) +
         ",\"fresh\":" + jsonBool(zcFresh()) + "},";
    s += "\"h11Modules\":";
    appendH11ModulesJson(s);
    s += ",";
    s += "\"temp\":{\"pin\":" + String(PIN_TEMP) +
         ",\"present\":" + jsonBool(gTempPresent) +
         ",\"c\":" + (gTempPresent ? String(gTempC, 2) : String("null")) +
         ",\"f\":" + (gTempPresent ? String(gTempF, 2) : String("null")) +
         ",\"ageMs\":" + String(gTempLastReadMs ? millis() - gTempLastReadMs : 0) +
         ",\"errors\":" + String(gTempReadErrors) +
         ",\"rawLevel\":" + String((int)owReadPin()) +
         ",\"lastPresence\":" + jsonBool(gTempLastPresence) +
         ",\"presenceDelayUs\":" + String(gTempPresenceDelayUs) +
         ",\"resetOk\":" + String(gTempResetOkCount) +
         ",\"resetFail\":" + String(gTempResetFailCount) +
         ",\"crcErrors\":" + String(gTempCrcErrors) + "},";
    s += "\"button\":{\"pin\":" + String(PIN_BUTTON) +
         ",\"pressed\":" + jsonBool(gButtonStablePressed) +
         ",\"enabled\":" + jsonBool(cfgButtonEnabled) +
         ",\"canArmMoc\":" + jsonBool(cfgButtonEnabled) +
         ",\"shortCount\":" + String(gButtonCount) +
         ",\"longCount\":" + String(gButtonLongCount) +
         ",\"lastEventMs\":" + String(gButtonLastEventMs) + "},";
    s += "\"sliders\":{\"sa\":\"" + String(sliderState(saA, saB)) +
         "\",\"sb\":\"" + String(sliderState(sbA, sbB)) + "\"},";
    s += "\"h11\":";
    appendH11Json(s);
    s += ",";
    s += "\"leds\":";
    appendLedJson(s);
    s += ",";
    s += "\"time\":{\"synced\":" + jsonBool(gTimeSynced) +
         ",\"epoch\":" + String(localEpochNow()) +
         ",\"tzOffsetMinutes\":" + String(gTzOffsetMinutes) +
         ",\"durationTimer\":" + jsonBool(gDurationTimerActive) +
         ",\"durationOffInMs\":" + String(gDurationTimerActive ? (int32_t)(gDurationOffAtMs - millis()) : 0) +
         ",\"clockTimer\":" + jsonBool(gClockTimerActive) +
         ",\"clockOffAtEpoch\":" + String(gClockOffAtEpoch) + "},";
    s += "\"schedule\":";
    appendScheduleJson(s);
    s += ",";
    s += "\"ac\":{\"present\":" + jsonBool(gAcPresent) +
         ",\"connectOffCount\":" + String(gAcConnectOffCount) +
         ",\"lastStableMs\":" + String(gLastAcStableMs) + "},";
    s += "\"wifi\":";
    appendWifiJson(s);
    s += ",\"ota\":";
    appendOtaJson(s);
    s += "}";
    webServer.send(200, "application/json", s);
}

static void webPowerLog() {
    if (!webAuthorized()) return;
    String s;
    s.reserve(9000);
    appendPowerLogJson(s);
    webServer.send(200, "application/json", s);
}

static void webTempHistory() {
    if (!webAuthorized()) return;
    String s;
    s.reserve(2200);
    appendTempHistoryJson(s);
    webServer.send(200, "application/json", s);
}

static void webConfig() {
    if (!webAuthorized()) return;
    String s;
    s.reserve(2200);
    s += "{";
	    s += "\"lightsOn\":" + jsonBool(cfgLightsOn) + ",";
	    s += "\"ledDimmerPercent\":" + String((int)cfgLedDimmerPercent) + ",";
	    s += "\"ledDataPin\":" + String((int)cfgLedDataPin) + ",";
    s += "\"mocArmed\":" + jsonBool(gMocArmed) + ",";
    s += "\"mode\":" + String((int)gMode) + ",";
    s += "\"buttonEnabled\":" + jsonBool(cfgButtonEnabled) + ",";
    s += "\"customSpeedEnabled\":" + jsonBool(cfgCustomSpeedEnabled) + ",";
    s += "\"customSpeedPercent\":" + String(cfgCustomSpeedPercent) + ",";
    s += "\"highDelayUs\":" + String(cfgHighDelayUs) + ",";
    s += "\"lowDelayUs\":" + String(cfgLowDelayUs) + ",";
    s += "\"mocPulseUs\":" + String(cfgMocPulseUs) + ",";
    s += "\"hystTenthF\":" + String(cfgHystTenthF) + ",";
    s += "\"settings\":{\"ssid\":\"" + jsonEscape(cfgApSsid) +
         "\",\"wifiPass\":\"" + jsonEscape(cfgApPass) +
         "\",\"webUser\":\"" + jsonEscape(cfgWebUser) +
         "\",\"webPass\":\"" + jsonEscape(cfgWebPass) +
         "\",\"apSsid\":\"" + jsonEscape(cfgApSsid) +
         "\",\"apPass\":\"" + jsonEscape(cfgApPass) +
         "\",\"staSsid\":\"" + jsonEscape(cfgStaSsid) +
         "\",\"staPass\":\"" + jsonEscape(cfgStaPass) +
         "\",\"staStatic\":" + jsonBool(cfgStaStaticEnabled) +
         ",\"staticIp\":\"" + jsonEscape(cfgStaIp) +
         "\",\"gateway\":\"" + jsonEscape(cfgStaGateway) +
         "\",\"subnet\":\"" + jsonEscape(cfgStaSubnet) +
         "\",\"dns\":\"" + jsonEscape(cfgStaDns) +
         "\",\"hostname\":\"" + jsonEscape(cfgHostname) +
         "\",\"adminUser\":\"" + jsonEscape(cfgAdminUser) + "\"},";
    s += "\"wifi\":";
    appendWifiJson(s);
    s += ",\"ota\":";
    appendOtaJson(s);
    s += ",";
    s += "\"schedule\":";
    appendScheduleJson(s);
    s += ",";
    s += "\"h11Map\":[";
    for (uint8_t i = 0; i < 4; i++) {
        if (i) s += ",";
        s += String((int)cfgH11Role[i]);
    }
    s += "],\"leds\":";
    appendLedJson(s);
    s += "}";
    webServer.send(200, "application/json", s);
}

static void webRoot() {
	    if (!webAuthorized()) return;
	    File f = LittleFS.open("/index.html", "r");
	    if (!f) {
	        webServer.send(500, "text/plain", "Missing /index.html in LittleFS. Upload the data folder.");
        return;
    }
    webServer.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    webServer.sendHeader("Pragma", "no-cache");
    webServer.sendHeader("Expires", "0");
	    webServer.streamFile(f, "text/html");
	    f.close();
	}

static void webServeStaticFile(const char* path, const char* mime) {
	    File f = LittleFS.open(path, "r");
	    if (!f) {
	        webServer.send(404, "text/plain", "Not found");
	        return;
	    }
	    webServer.sendHeader("Cache-Control", "public, max-age=86400");
	    webServer.streamFile(f, mime);
	    f.close();
	}

static void webPostControl() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    int mode = jsonInt(body, "mode", -1);
    if (mode >= 0 && mode <= (int)MODE_OFF_MEMORY) {
        cancelScheduleDrive();
        setMode((FanMode)mode, POWER_CAUSE_WEB_CONTROL);
    }
	    int lights = jsonInt(body, "lightsOn", -1);
	    if (lights >= 0) cfgLightsOn = lights != 0;
	    if (jsonHas(body, "ledDimmerPercent")) {
	        cfgLedDimmerPercent = constrain(jsonInt(body, "ledDimmerPercent", cfgLedDimmerPercent), 0, 100);
	        renderLeds();
	    }
	    int buttonEnabled = jsonInt(body, "buttonEnabled", -1);
    if (buttonEnabled >= 0) {
        cfgButtonEnabled = buttonEnabled != 0;
        if (!cfgButtonEnabled) gButtonWasPressed = false;
    }
    int customEnabled = jsonInt(body, "customSpeedEnabled", -1);
    if (customEnabled >= 0) {
        cfgCustomSpeedEnabled = customEnabled != 0;
        gPendingMotorCause = POWER_CAUSE_WEB_CONTROL;
    }
    if (jsonHas(body, "speedPercent")) {
        cfgCustomSpeedPercent = clampCustomSpeedPercent(jsonInt(body, "speedPercent", cfgCustomSpeedPercent));
    }
    int armed = jsonInt(body, "mocArmed", -1);
    if (armed >= 0) {
        bool wasArmed = gMocArmed;
        gMocArmed = armed != 0;
        if (!gMocArmed) forceMocOff();
        if (wasArmed != gMocArmed && gMotorShouldRun) {
            recordPowerEvent(gMocArmed ? POWER_EVENT_OUTPUT_RESTORED : POWER_EVENT_OUTPUT_BLOCKED,
                             POWER_CAUSE_WEB_CONTROL, 0, gMode, speedPercentForMode(gMode));
        }
    }
    if (jsonInt(body, "save", 0)) saveConfig();
    webOk("control");
}

static void webPostFan() {
    if (!webAuthorized()) return;
    cancelScheduleDrive();
    String body = webServer.arg("plain");
    if (jsonInt(body, "off", 0)) {
        fullPowerOff(POWER_CAUSE_WEB_POWER_OFF);
        webOk("fan-off");
        return;
    }

    if (jsonHas(body, "customEnabled")) {
        cfgCustomSpeedEnabled = jsonInt(body, "customEnabled", cfgCustomSpeedEnabled ? 1 : 0) != 0;
    }
    cfgCustomSpeedPercent = clampCustomSpeedPercent(jsonInt(body, "speedPercent", cfgCustomSpeedPercent));

    if (cfgCustomSpeedEnabled) {
        setModeFromWeb(MODE_CUSTOM, POWER_CAUSE_WEB_FAN);
    } else {
        uint8_t selectedSpeed = (uint8_t)jsonInt(body, "originalSpeed", SPEED_HIGH);
        int setpointF = jsonInt(body, "setpointF", 0);
        FanMode mode = modeFromOriginalChoice(selectedSpeed, setpointF);
        setModeFromWeb(mode, POWER_CAUSE_WEB_FAN);
    }

    if (jsonInt(body, "save", 1)) saveConfig();
    webOk("fan");
}

static void webPostLed() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    int idx = jsonInt(body, "index", -1);
    if (idx < 0 || idx >= LED_COUNT) {
        webServer.send(400, "application/json", "{\"ok\":false,\"msg\":\"bad-led-index\"}");
        return;
    }
    cfgLed[idx].r = constrain(jsonInt(body, "r", cfgLed[idx].r), 0, 255);
    cfgLed[idx].g = constrain(jsonInt(body, "g", cfgLed[idx].g), 0, 255);
    cfgLed[idx].b = constrain(jsonInt(body, "b", cfgLed[idx].b), 0, 255);
    if (jsonHas(body, "enabled")) cfgLedEnabled[idx] = jsonInt(body, "enabled", cfgLedEnabled[idx]) ? 1 : 0;
    bool preview = jsonInt(body, "preview", 1) != 0;
    if (preview && cfgLedEnabled[idx]) {
        startLedPreviewSingle((uint8_t)idx);
    } else {
        clearLedPreview();
        renderLeds();
    }
    if (jsonInt(body, "save", 0)) saveConfig();
    webOk("led");
}

static void webPostLedAll() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    Rgb color;
    color.r = constrain(jsonInt(body, "r", cfgLed[0].r), 0, 255);
    color.g = constrain(jsonInt(body, "g", cfgLed[0].g), 0, 255);
    color.b = constrain(jsonInt(body, "b", cfgLed[0].b), 0, 255);
	    for (uint8_t i = 0; i < LED_COUNT; i++) cfgLed[i] = color;
	    if (jsonHas(body, "ledDimmerPercent")) {
	        cfgLedDimmerPercent = constrain(jsonInt(body, "ledDimmerPercent", cfgLedDimmerPercent), 0, 100);
	    }
	    if (jsonHas(body, "enabled")) {
	        uint8_t enabled = jsonInt(body, "enabled", 1) ? 1 : 0;
        for (uint8_t i = 0; i < LED_COUNT; i++) cfgLedEnabled[i] = enabled;
    }
    bool preview = jsonInt(body, "preview", 1) != 0;
    if (preview) {
        startLedPreviewAll();
    } else {
        clearLedPreview();
        renderLeds();
    }
    if (jsonInt(body, "save", 0)) saveConfig();
    webOk("led-all");
}

static void webPostLedPreviewClear() {
    if (!webAuthorized()) return;
    clearLedPreview();
    renderLeds();
    webOk("led-preview-clear");
}

static void webPostLedFunction() {
    if (!webAuthorized()) return;
    cfgLightsOn = true;
    enableAllLedSession();
    clearLedPreview();
    renderLeds();
    webOk("led-function");
}

static void webPostBoardLed() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    int on = jsonInt(body, "on", -1);
    gBoardLedOn = on >= 0 ? (on != 0) : !gBoardLedOn;
    renderBoardLed();
    webOk("board-led");
}

static void webPostLedChase() {
    if (!webAuthorized()) return;
    startLedPreviewChase();
    renderLeds();
    webOk("led-chase");
}

static void webPostLedPin() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    uint8_t nextPin = (uint8_t)jsonInt(body, "pin", cfgLedDataPin);
    if (!validLedDataPin(nextPin)) {
        webServer.send(400, "application/json", "{\"ok\":false,\"msg\":\"bad-led-pin\"}");
        return;
    }
    pixels.clear();
    pixels.show();
    gpio_set_direction((gpio_num_t)cfgLedDataPin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)cfgLedDataPin, 0);
    cfgLedDataPin = nextPin;
    gLedDataOverride = -1;
    configurePixels();
    startLedPreviewAll();
    if (jsonInt(body, "save", 0)) saveConfig();
    renderLeds();
    webOk("led-pin");
}

static void webPostLedData() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    int level = jsonInt(body, "level", -1);
    if (level == 0 || level == 1) {
        clearLedPreview();
        pixels.clear();
        pixels.show();
        gLedDataOverride = level;
        gpio_set_direction((gpio_num_t)cfgLedDataPin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)cfgLedDataPin, level);
        webOk(level ? "led-data-high" : "led-data-low");
        return;
    }
    gLedDataOverride = -1;
    configurePixels();
    startLedPreviewAll();
    renderLeds();
    webOk("led-data-auto");
}

static void webPostButtonCycle() {
    if (!webAuthorized()) return;
    buttonCycleOriginal();
    webOk("button-cycle");
}

static void webPostPowerOff() {
    if (!webAuthorized()) return;
    fullPowerOff(POWER_CAUSE_WEB_POWER_OFF);
    webOk("power-off");
}

static void webPostTimer() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    if (jsonInt(body, "clear", 0)) {
        gDurationTimerActive = false;
        gClockTimerActive = false;
        webOk("timer-clear");
        return;
    }
    int minutes = jsonInt(body, "durationMinutes", -1);
    if (minutes > 0 && minutes <= 1440) {
        uint8_t speed = (uint8_t)jsonInt(body, "speed", SPEED_HIGH);
        if (speed != SPEED_HIGH && speed != SPEED_LOW) speed = SPEED_HIGH;
        cancelScheduleDrive();
        setModeFromWeb(modeFromOriginalChoice(speed, 0), POWER_CAUSE_WEB_FAN);
        gDurationTimerActive = true;
        gDurationOffAtMs = millis() + (uint32_t)minutes * 60UL * 1000UL;
    }
    long offAtEpoch = jsonLong(body, "offAtEpoch", -1);
    if (offAtEpoch > 0) {
        gClockTimerActive = true;
        gClockOffAtEpoch = (uint32_t)offAtEpoch;
    }
    webOk("timer");
}

static void webPostSchedule() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    bool scheduleWasDriving = gScheduleDriving;
    uint8_t runningSchedulePercent = speedPercentForMode(gMode);
    cancelScheduleDrive();
    if (jsonHas(body, "mode")) {
        cfgScheduleMode = (uint8_t)jsonInt(body, "mode", cfgScheduleMode);
        if (cfgScheduleMode > SCHEDULE_DAILY) cfgScheduleMode = SCHEDULE_OFF;
    } else if (jsonHas(body, "enabled")) {
        cfgScheduleMode = jsonInt(body, "enabled", 0) ? SCHEDULE_WEEKLY : SCHEDULE_OFF;
    }
    cfgScheduleEnabled = cfgScheduleMode != SCHEDULE_OFF;
    if (jsonHas(body, "daysMask")) cfgScheduleDaysMask = (uint8_t)jsonInt(body, "daysMask", cfgScheduleDaysMask) & 0x7F;
    if (jsonHas(body, "startMin")) cfgScheduleStartMin = constrain(jsonInt(body, "startMin", cfgScheduleStartMin), 0, MINUTES_PER_DAY - 1);
    if (jsonHas(body, "endMin")) cfgScheduleEndMin = constrain(jsonInt(body, "endMin", cfgScheduleEndMin), 0, MINUTES_PER_DAY - 1);
    if (jsonHas(body, "dailyDaysMask")) cfgDailyDaysMask = (uint8_t)jsonInt(body, "dailyDaysMask", cfgDailyDaysMask) & 0x7F;
    for (uint8_t i = 0; i < 7; i++) {
        String startKey = String("d") + i + "StartMin";
        String endKey = String("d") + i + "EndMin";
        if (jsonHas(body, startKey.c_str())) {
            cfgDailyStartMin[i] = constrain(jsonInt(body, startKey.c_str(), cfgDailyStartMin[i]), 0, MINUTES_PER_DAY - 1);
        }
        if (jsonHas(body, endKey.c_str())) {
            cfgDailyEndMin[i] = constrain(jsonInt(body, endKey.c_str(), cfgDailyEndMin[i]), 0, MINUTES_PER_DAY - 1);
        }
    }
    if (jsonHas(body, "speed")) {
        cfgScheduleSpeed = (uint8_t)jsonInt(body, "speed", cfgScheduleSpeed);
        if (cfgScheduleSpeed != SPEED_HIGH && cfgScheduleSpeed != SPEED_LOW && cfgScheduleSpeed != SPEED_CUSTOM) {
            cfgScheduleSpeed = SPEED_HIGH;
        }
    }
    if (jsonHas(body, "speedPercent")) {
        cfgScheduleSpeedPercent = clampCustomSpeedPercent(jsonInt(body, "speedPercent", cfgScheduleSpeedPercent));
    }
    if (jsonHas(body, "ledDimmerPercent")) {
        cfgScheduleLedDimmerPercent = constrain(jsonInt(body, "ledDimmerPercent", cfgScheduleLedDimmerPercent), 0, 100);
    }
    if (jsonHas(body, "tempRule")) {
        cfgScheduleTempRule = (uint8_t)jsonInt(body, "tempRule", cfgScheduleTempRule);
        if (cfgScheduleTempRule > SCHEDULE_TEMP_OR) cfgScheduleTempRule = SCHEDULE_TEMP_NONE;
    }
    if (jsonHas(body, "tempF")) cfgScheduleTempF = constrain(jsonInt(body, "tempF", cfgScheduleTempF), 40, 110);
    gScheduleWindowWasActive = false;
    if (scheduleWasDriving && !scheduleShouldRunNow()) {
        fullPowerOff(POWER_CAUSE_SCHEDULE_EDIT, runningSchedulePercent);
    }
    if (jsonInt(body, "save", 1)) saveConfig();
    webOk("schedule");
}

static void webPostTime() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    long epoch = jsonLong(body, "epoch", 0);
    if (epoch > 0) {
        gEpochAtSync = (uint32_t)epoch;
        gMillisAtSync = millis();
        gTzOffsetMinutes = (int16_t)constrain(jsonInt(body, "tzOffsetMinutes", 0), -14 * 60, 14 * 60);
        gTimeSynced = true;
        stampCurrentBootPowerEvents();
    }
    webOk("time");
}

static void webPostWifiScan() {
    if (!webAuthorized()) return;
    int16_t state = WiFi.scanComplete();
    if (state == WIFI_SCAN_RUNNING) {
        webServer.send(200, "application/json", "{\"ok\":true,\"scanning\":true}");
        return;
    }
    if (state >= 0) WiFi.scanDelete();
    int16_t started = WiFi.scanNetworks(true, false, false, 120, 0);
    if (started == WIFI_SCAN_FAILED) {
        webServer.send(503, "application/json", "{\"ok\":false,\"msg\":\"scan-start-failed\"}");
        return;
    }
    webServer.send(200, "application/json", "{\"ok\":true,\"scanning\":true}");
}

static void webGetWifiScan() {
    if (!webAuthorized()) return;
    int16_t count = WiFi.scanComplete();
    if (count == WIFI_SCAN_RUNNING) {
        webServer.send(200, "application/json", "{\"ok\":true,\"scanning\":true,\"networks\":[]}");
        return;
    }
    if (count == WIFI_SCAN_FAILED) {
        webServer.send(200, "application/json", "{\"ok\":false,\"scanning\":false,\"msg\":\"scan-failed\",\"networks\":[]}");
        return;
    }

    String s;
    s.reserve(160 + (count > 0 ? count : 0) * 96);
    s += "{\"ok\":true,\"scanning\":false,\"networks\":[";
    for (int16_t i = 0; i < count; i++) {
        if (i) s += ",";
        uint8_t auth = WiFi.encryptionType(i);
        s += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) +
             "\",\"rssi\":" + String(WiFi.RSSI(i)) +
             ",\"channel\":" + String(WiFi.channel(i)) +
             ",\"auth\":" + String(auth) +
             ",\"secure\":" + jsonBool(auth != WIFI_AUTH_OPEN) + "}";
    }
    s += "]}";
    WiFi.scanDelete();
    webServer.send(200, "application/json", s);
}

static void webPostWifiStatic() {
    if (!webAuthorized()) return;
    if (WiFi.status() != WL_CONNECTED) {
        webServer.send(409, "application/json", "{\"ok\":false,\"msg\":\"sta-not-connected\"}");
        return;
    }

    String body = webServer.arg("plain");
    int lastOctet = jsonInt(body, "lastOctet", -1);
    if (lastOctet < 2 || lastOctet > 254) {
        webServer.send(400, "application/json", "{\"ok\":false,\"msg\":\"invalid-last-octet\"}");
        return;
    }

    IPAddress current = WiFi.localIP();
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress dns = WiFi.dnsIP();
    IPAddress candidate = current;
    candidate[3] = (uint8_t)lastOctet;
    if (((uint32_t)candidate & (uint32_t)subnet) != ((uint32_t)current & (uint32_t)subnet) ||
        candidate == gateway) {
        webServer.send(400, "application/json", "{\"ok\":false,\"msg\":\"invalid-static-ip\"}");
        return;
    }
    if ((uint32_t)dns == 0) dns = gateway;

    cfgStaStaticEnabled = true;
    cfgStaIp = candidate.toString();
    cfgStaGateway = gateway.toString();
    cfgStaSubnet = subnet.toString();
    cfgStaDns = dns.toString();
    saveConfig();
    gWifiRestartAtMs = millis() + 600;
    String s = "{\"ok\":true,\"msg\":\"static-ip-saved\",\"ip\":\"" + cfgStaIp + "\"}";
    webServer.send(200, "application/json", s);
}

static void webPostSettings() {
    if (!webAuthorized()) return;
    String body = webServer.arg("plain");
    String oldApSsid = cfgApSsid;
    String oldApPass = cfgApPass;
    String oldStaSsid = cfgStaSsid;
    String oldStaPass = cfgStaPass;
    bool oldStaStatic = cfgStaStaticEnabled;
    String oldStaIp = cfgStaIp;
    String oldStaGateway = cfgStaGateway;
    String oldStaSubnet = cfgStaSubnet;
    String oldStaDns = cfgStaDns;
    String oldHostname = cfgHostname;

    cfgHighDelayUs = constrain(jsonInt(body, "highDelayUs", cfgHighDelayUs), 0, 8000);
    cfgLowDelayUs = constrain(jsonInt(body, "lowDelayUs", cfgLowDelayUs), 0, 8000);
    cfgMocPulseUs = constrain(jsonInt(body, "mocPulseUs", cfgMocPulseUs), 40, 1000);
    cfgHystTenthF = constrain(jsonInt(body, "hystTenthF", cfgHystTenthF), 2, 50);
    for (uint8_t i = 0; i < 4; i++) {
        String key = String("role") + i;
        int role = jsonInt(body, key.c_str(), cfgH11Role[i]);
        if (role >= 0 && role <= 3) cfgH11Role[i] = role;
    }
    cfgApSsid = saneConfigString(jsonString(body, "apSsid", jsonString(body, "ssid", cfgApSsid)), AP_SSID_DEFAULT, 1, 31);
    cfgApPass = saneConfigString(jsonString(body, "apPass", jsonString(body, "wifiPass", cfgApPass)), AP_PASS_DEFAULT, 8, 63);
    cfgWebUser = saneConfigString(jsonString(body, "webUser", cfgWebUser), WEB_USER_DEFAULT, 1, 31);
    cfgWebPass = saneConfigString(jsonString(body, "webPass", cfgWebPass), WEB_PASS_DEFAULT, 1, 63);
    cfgStaSsid = saneOptionalString(jsonString(body, "staSsid", cfgStaSsid), 32);
    cfgStaPass = saneOptionalString(jsonString(body, "staPass", cfgStaPass), 63);
    if (jsonHas(body, "staStatic")) cfgStaStaticEnabled = jsonInt(body, "staStatic", cfgStaStaticEnabled ? 1 : 0) != 0;
    cfgStaIp = saneIpString(jsonString(body, "staticIp", cfgStaIp), STA_IP_DEFAULT);
    cfgStaGateway = saneIpString(jsonString(body, "gateway", cfgStaGateway), STA_GATEWAY_DEFAULT);
    cfgStaSubnet = saneIpString(jsonString(body, "subnet", cfgStaSubnet), STA_SUBNET_DEFAULT);
    cfgStaDns = saneIpString(jsonString(body, "dns", cfgStaDns), STA_DNS_DEFAULT);
    cfgHostname = saneHostname(jsonString(body, "hostname", cfgHostname));
    saveConfig();
    bool wifiChanged = oldApSsid != cfgApSsid || oldApPass != cfgApPass ||
                       oldStaSsid != cfgStaSsid || oldStaPass != cfgStaPass ||
                       oldStaStatic != cfgStaStaticEnabled || oldStaIp != cfgStaIp ||
                       oldStaGateway != cfgStaGateway || oldStaSubnet != cfgStaSubnet ||
                       oldStaDns != cfgStaDns || oldHostname != cfgHostname;
    if (wifiChanged) gWifiRestartAtMs = millis() + 600;
    webOk("settings");
}

static void webPostSave() {
    if (!webAuthorized()) return;
    saveConfig();
    webOk("saved");
}

static void webPostRestore() {
    if (!webAuthorized()) return;
    loadConfig();
    if (gConfigNeedsSave) saveConfig();
    startWifi();
    webOk("restored");
}

static void webPostFactoryReset() {
    if (!webAuthorized()) return;
    factoryDefaults();
    startWifi();
    webOk("factory-reset");
}

static void webAdminStatus() {
    if (!webAdminAuthorized()) return;
    String s;
    s.reserve(1200);
    s += "{\"ok\":true,\"adminUser\":\"" + jsonEscape(cfgAdminUser) + "\",\"wifi\":";
    appendWifiJson(s);
    s += ",\"ota\":";
    appendOtaJson(s);
    s += "}";
    webServer.send(200, "application/json", s);
}

static void webPostAdminCredentials() {
    if (!webAdminAuthorized()) return;
    String body = webServer.arg("plain");
    String nextUser = saneConfigString(jsonString(body, "user", cfgAdminUser), ADMIN_USER_DEFAULT, 1, 31);
    String nextPass = saneConfigString(jsonString(body, "pass", cfgAdminPass), ADMIN_PASS_DEFAULT, 1, 63);
    cfgAdminUser = nextUser;
    cfgAdminPass = nextPass;
    saveConfig();
    webOk("admin-credentials");
}

static void resetUpdateState(const char* kind) {
    gUpdateActive = true;
    gUpdateOk = false;
    gUpdateKind = kind;
    gUpdateError = "";
    gUpdateBytes = 0;
    gUpdateTotal = 0;
    gUpdateStartedMs = millis();
    gRebootAtMs = 0;
}

static void failUpdate(const String& message) {
    gUpdateError = message;
    gUpdateOk = false;
}

static void webAdminUpdateUpload(int command, const char* kind) {
    if (!webAdminAuthorized(false)) return;
    HTTPUpload& upload = webServer.upload();

    if (upload.status == UPLOAD_FILE_START) {
        PowerEventCause updateCause = command == U_FLASHFS ? POWER_CAUSE_OTA_LITTLEFS : POWER_CAUSE_OTA_FIRMWARE;
        recordPowerEvent(POWER_EVENT_SYSTEM, updateCause, 0, gMode, speedPercentForMode(gMode));
        fullPowerOff(updateCause);
        clearLedPreview();
        renderLeds();
        resetUpdateState(kind);
        if (command == U_FLASHFS) LittleFS.end();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, command)) {
            failUpdate(Update.errorString());
            Serial0.printf("[OTA] %s begin failed: %s\n", kind, gUpdateError.c_str());
        } else {
            Serial0.printf("[OTA] %s upload start: %s\n", kind, upload.filename.c_str());
        }
        return;
    }

    if (!gUpdateActive || gUpdateError.length()) return;

    if (upload.status == UPLOAD_FILE_WRITE) {
        gUpdateBytes = upload.totalSize + upload.currentSize;
        gUpdateTotal = gUpdateBytes;
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            failUpdate(Update.errorString());
            Serial0.printf("[OTA] %s write failed: %s\n", kind, gUpdateError.c_str());
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        gUpdateBytes = upload.totalSize;
        gUpdateTotal = upload.totalSize;
        if (Update.end(true)) {
            gUpdateOk = true;
            gUpdateError = "";
            Serial0.printf("[OTA] %s update ok, %u bytes\n", kind, (unsigned)gUpdateBytes);
        } else {
            failUpdate(Update.errorString());
            Serial0.printf("[OTA] %s end failed: %s\n", kind, gUpdateError.c_str());
        }
        gUpdateActive = false;
        return;
    }

    if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
        failUpdate("aborted");
        gUpdateActive = false;
        Serial0.printf("[OTA] %s aborted\n", kind);
    }
}

static void webAdminUpdateFinish() {
    if (!webAdminAuthorized()) return;
    if (gUpdateOk && !Update.hasError()) {
        gRebootAtMs = millis() + 1200;
        String s = "{\"ok\":true,\"msg\":\"update-ok\",\"kind\":\"" + jsonEscape(gUpdateKind) +
                   "\",\"bytes\":" + String((uint32_t)gUpdateBytes) +
                   ",\"rebootInMs\":1200}";
        webServer.send(200, "application/json", s);
    } else {
        String s = "{\"ok\":false,\"msg\":\"update-failed\",\"kind\":\"" + jsonEscape(gUpdateKind) +
                   "\",\"error\":\"" + jsonEscape(gUpdateError.length() ? gUpdateError : String(Update.errorString())) + "\"}";
        webServer.send(500, "application/json", s);
    }
}

static void webPostAdminReboot() {
    if (!webAdminAuthorized()) return;
    recordPowerEvent(POWER_EVENT_SYSTEM, POWER_CAUSE_ADMIN_REBOOT, 0, gMode, speedPercentForMode(gMode));
    fullPowerOff(POWER_CAUSE_ADMIN_REBOOT);
    gRebootAtMs = millis() + 800;
    webOk("reboot");
}

static void webNotFound() {
    if (!webAuthorized()) return;
    webServer.send(404, "text/plain", "Not found");
}

static void startWeb() {
    if (!LittleFS.begin(true)) {
        Serial0.println("[WEB] LittleFS mount failed");
    }
    gSessionToken = prefs.getString("session", "");
    if (gSessionToken.length() != 16) {
        char token[17];
        snprintf(token, sizeof(token), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
        gSessionToken = token;
        prefs.putString("session", gSessionToken);
    }
    startWifi();
    const char* headerKeys[] = {"Cookie", "Authorization"};
	    webServer.collectHeaders(headerKeys, 2);
	    webServer.on("/", HTTP_GET, webRoot);
	    webServer.on("/index.html", HTTP_GET, webRoot);
	    webServer.on("/manifest.webmanifest", HTTP_GET, []() { webServeStaticFile("/manifest.webmanifest", "application/manifest+json"); });
	    webServer.on("/service-worker.js", HTTP_GET, []() {
	        File f = LittleFS.open("/service-worker.js", "r");
	        if (!f) {
	            webServer.send(404, "text/plain", "Not found");
	            return;
	        }
	        webServer.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
	        webServer.streamFile(f, "application/javascript");
	        f.close();
	    });
	    webServer.on("/favicon-32.png", HTTP_GET, []() { webServeStaticFile("/favicon-32.png", "image/png"); });
	    webServer.on("/apple-touch-icon.png", HTTP_GET, []() { webServeStaticFile("/apple-touch-icon.png", "image/png"); });
	    webServer.on("/app-icon-192.png", HTTP_GET, []() { webServeStaticFile("/app-icon-192.png", "image/png"); });
	    webServer.on("/app-icon-512.png", HTTP_GET, []() { webServeStaticFile("/app-icon-512.png", "image/png"); });
	    webServer.on("/api/login", HTTP_POST, webPostLogin);
    webServer.on("/api/session", HTTP_GET, webSession);
    webServer.on("/api/logout", HTTP_POST, webPostLogout);
    webServer.on("/api/status", HTTP_GET, webStatus);
    webServer.on("/api/power-log", HTTP_GET, webPowerLog);
    webServer.on("/api/temp-history", HTTP_GET, webTempHistory);
    webServer.on("/api/config", HTTP_GET, webConfig);
    webServer.on("/api/control", HTTP_POST, webPostControl);
    webServer.on("/api/fan", HTTP_POST, webPostFan);
    webServer.on("/api/led", HTTP_POST, webPostLed);
    webServer.on("/api/led-all", HTTP_POST, webPostLedAll);
    webServer.on("/api/led-preview-clear", HTTP_POST, webPostLedPreviewClear);
    webServer.on("/api/led-function", HTTP_POST, webPostLedFunction);
    webServer.on("/api/board-led", HTTP_POST, webPostBoardLed);
    webServer.on("/api/led-chase", HTTP_POST, webPostLedChase);
    webServer.on("/api/led-pin", HTTP_POST, webPostLedPin);
    webServer.on("/api/led-data", HTTP_POST, webPostLedData);
    webServer.on("/api/button-cycle", HTTP_POST, webPostButtonCycle);
    webServer.on("/api/power-off", HTTP_POST, webPostPowerOff);
    webServer.on("/api/timer", HTTP_POST, webPostTimer);
    webServer.on("/api/schedule", HTTP_POST, webPostSchedule);
    webServer.on("/api/time", HTTP_POST, webPostTime);
    webServer.on("/api/wifi/scan", HTTP_POST, webPostWifiScan);
    webServer.on("/api/wifi/scan", HTTP_GET, webGetWifiScan);
    webServer.on("/api/wifi/static", HTTP_POST, webPostWifiStatic);
    webServer.on("/api/settings", HTTP_POST, webPostSettings);
    webServer.on("/api/save", HTTP_POST, webPostSave);
    webServer.on("/api/restore", HTTP_POST, webPostRestore);
    webServer.on("/api/factory-reset", HTTP_POST, webPostFactoryReset);
    webServer.on("/api/admin/status", HTTP_GET, webAdminStatus);
    webServer.on("/api/admin/credentials", HTTP_POST, webPostAdminCredentials);
    webServer.on("/api/admin/reboot", HTTP_POST, webPostAdminReboot);
    webServer.on("/api/admin/update/firmware", HTTP_POST, webAdminUpdateFinish, []() {
        webAdminUpdateUpload(U_FLASH, "firmware");
    });
    webServer.on("/api/admin/update/littlefs", HTTP_POST, webAdminUpdateFinish, []() {
        webAdminUpdateUpload(U_FLASHFS, "littlefs");
    });
    webServer.onNotFound(webNotFound);
    webServer.begin();
    Serial0.printf("[WEB] user=%s admin=%s\n", cfgWebUser.c_str(), cfgAdminUser.c_str());
}

static void serviceScheduledReboot() {
    if (!gRebootAtMs) return;
    forceMocOff();
    if (timeDueUs(millis(), gRebootAtMs)) {
        delay(80);
        ESP.restart();
    }
}

void setup() {
    Serial0.begin(115200);
    delay(200);
    Serial0.printf("\n[%s] %s\n", FW_NAME, FW_VERSION);

    pinMode(PIN_TRIAC_TRIGGER, OUTPUT);
    forceMocOff();

    pinMode(PIN_ZERO_CROSS, INPUT_PULLUP);
    pinMode(PIN_H11_1, INPUT_PULLUP);
    pinMode(PIN_H11_2, INPUT_PULLUP);
    pinMode(PIN_H11_3, INPUT_PULLUP);
    pinMode(PIN_H11_4, INPUT_PULLUP);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    owBusInit();

    prefs.begin("hfan", false);
    loadConfig();
    loadTempHistory();
    loadPowerLog();
    recordPowerEvent(POWER_EVENT_BOOT, resetCause());
    fullPowerOff(POWER_CAUSE_BOOT_SAFETY);
    if (gConfigNeedsSave) saveConfig();

    configurePixels();
    configureBoardLed();
    clearLedPreview();
    renderLeds();

    BaseType_t fireTaskCreated = xTaskCreatePinnedToCore(
        triacFireTask, "triac-fire", 3072, nullptr, configMAX_PRIORITIES - 1,
        &gTriacFireTaskHandle, 0);
    if (fireTaskCreated != pdPASS) {
        gTriacFireTaskHandle = nullptr;
        gTriacFireTaskReady = false;
        Serial0.println("[MOC] Failed to create TRIAC fire task; output remains disabled");
    }

    attachInterrupt(digitalPinToInterrupt(PIN_ZERO_CROSS), onZeroCrossChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_H11_1), onH11_1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_H11_2), onH11_2, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_H11_3), onH11_3, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_H11_4), onH11_4, CHANGE);

    startWeb();
    ds18b20StartConversion();
}

void loop() {
    webServer.handleClient();
    serviceScheduledReboot();
    if (gRebootAtMs || gUpdateActive) {
        forceMocOff();
        return;
    }
    serviceWifi();
    serviceButton();
    serviceTemperature();
    servicePulseMetrics();
    serviceOffTimers();
    serviceWeeklySchedule();
    serviceMotorLogic();
    syncTriacFireSnapshot();
    servicePowerDiagnostics();
    servicePowerLogPersistence();
    serviceTempHistoryPersistence();

    static uint32_t lastLedMs = 0;
    uint32_t now = millis();
    if (now - lastLedMs >= 80) {
        lastLedMs = now;
        renderLeds();
    }
}
