#include "../firmware/Holmes_HWF0910AT_Smart_Mod/ClockSyncPolicy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint32_t localDay(time_t epoch) {
    struct tm local;
    localtime_r(&epoch, &local);
    return (uint32_t)(local.tm_year + 1900) * 1000 + local.tm_yday + 1;
}

int main() {
    ClockSyncPolicy clock;
    clock.request(CLOCK_BOOT);
    assert(!clock.shouldAttempt(1000, false));
    assert(clock.shouldAttempt(1000, true));
    clock.attemptedAt(1000);
    clock.request(CLOCK_LAN);
    clock.request(CLOCK_AC);
    clock.request(CLOCK_AP_CLIENT);
    clock.request(CLOCK_APP);
    assert(clock.pending == 31);
    assert(!clock.shouldAttempt(15999, true));
    assert(clock.shouldAttempt(16000, true));
    clock.ntpCompleted(1791479048);
    assert(clock.pending == 0 && clock.completions == 1);

    clock.observeDay(2026281);
    assert(clock.midnightRequests == 0);
    clock.observeDay(2026282);
    assert(clock.pending == CLOCK_MIDNIGHT && clock.midnightRequests == 1);
    clock.observeDay(2026282);
    clock.observeDay(2026281);
    clock.observeDay(2026282);
    assert(clock.midnightRequests == 1);
    // Manual time updates cannot discharge the automatic midnight obligation.
    clock.lastManualEpoch = 1791565448;
    assert(clock.pending == CLOCK_MIDNIGHT);
    assert(!clock.shouldAttempt(30000, false));
    assert(clock.shouldAttempt(30000, true));
    clock.ntpCompleted(1791565449);
    clock.observeDay(2026283);
    assert(clock.midnightRequests == 2 && clock.pending == CLOCK_MIDNIGHT);

    ClockSyncPolicy yearEnd;
    yearEnd.observeDay(2026365);
    yearEnd.observeDay(2027001);
    assert(yearEnd.midnightRequests == 1);
    yearEnd.attemptedAt(UINT32_MAX - 1000);
    assert(!yearEnd.shouldAttempt(1000, true));
    assert(yearEnd.shouldAttempt(14000, true));

    setenv("TZ", "MST7MDT,M3.2.0,M11.1.0", 1);
    tzset();
    struct tm local = {};
    local.tm_year = 2026 - 1900;
    local.tm_mon = 9;
    local.tm_mday = 8;
    local.tm_hour = 23;
    local.tm_min = 59;
    local.tm_sec = 59;
    local.tm_isdst = -1;
    time_t midnight = mktime(&local) + 1;
    ClockSyncPolicy daily;
    daily.observeDay(localDay(midnight - 1));
    daily.lastManualEpoch = (uint32_t)(midnight - 1);
    daily.observeDay(localDay(midnight));
    assert(daily.midnightRequests == 1 && daily.pending == CLOCK_MIDNIGHT);
    daily.ntpCompleted((uint32_t)midnight);
    for (time_t epoch = midnight + 1; epoch < midnight + 86400; epoch += 60) {
        daily.observeDay(localDay(epoch));
    }
    assert(daily.midnightRequests == 1);
    daily.observeDay(localDay(midnight + 86400));
    assert(daily.midnightRequests == 2);

    // The repeated hour at Denver's fall DST transition is not midnight.
    local.tm_mon = 10;
    local.tm_mday = 1;
    local.tm_hour = 0;
    local.tm_min = local.tm_sec = 0;
    local.tm_isdst = -1;
    time_t dstDay = mktime(&local);
    ClockSyncPolicy dst;
    dst.observeDay(localDay(dstDay));
    dst.observeDay(localDay(dstDay + 86400));
    assert(dst.midnightRequests == 0);
    dst.observeDay(localDay(dstDay + 90000));
    assert(dst.midnightRequests == 1);
    puts("Clock policy: reconnections, midnight, manual independence, offline retry and rollover passed");
}
