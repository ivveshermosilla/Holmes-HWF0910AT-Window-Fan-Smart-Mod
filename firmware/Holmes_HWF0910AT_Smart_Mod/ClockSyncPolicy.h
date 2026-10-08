#pragma once
#include <stdint.h>

enum ClockSyncReason : uint8_t {
    CLOCK_BOOT = 1,
    CLOCK_LAN = 2,
    CLOCK_AC = 4,
    CLOCK_AP_CLIENT = 8,
    CLOCK_APP = 16,
    CLOCK_MIDNIGHT = 32
};

struct ClockSyncPolicy {
    uint8_t pending = 0;
    uint8_t lastReasons = 0;
    uint32_t requests = 0;
    uint32_t attempts = 0;
    uint32_t completions = 0;
    uint32_t midnightRequests = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t lastNtpEpoch = 0;
    uint32_t lastManualEpoch = 0;
    uint32_t lastDay = 0;
    bool attempted = false;

    void request(uint8_t reason) {
        pending |= reason;
        lastReasons = reason;
        ++requests;
        if (reason & CLOCK_MIDNIGHT) ++midnightRequests;
    }

    void observeDay(uint32_t day) {
        if (lastDay && day > lastDay) request(CLOCK_MIDNIGHT);
        // Clock corrections backward must not repeat an already observed midnight.
        if (day > lastDay) lastDay = day;
    }

    bool shouldAttempt(uint32_t nowMs, bool connected) const {
        return pending && connected &&
               (!attempted || (uint32_t)(nowMs - lastAttemptMs) >= 15000);
    }

    void attemptedAt(uint32_t nowMs) {
        attempted = true;
        lastAttemptMs = nowMs;
        ++attempts;
    }

    void ntpCompleted(uint32_t epoch) {
        lastNtpEpoch = epoch;
        pending = 0;
        ++completions;
    }
};
