#pragma once

#include "Arduino.h"
#include "esp_log.h"


class Timeout {
    
public:
    Timeout(const long _interval);
    ~Timeout();

    void setInterval(const long _interval);
    long getInterval();

    bool checkTimeout(const long _now);
    bool checkTimeoutAndRestart(const long _now);
    bool checkTimeoutNoStop(const long _now);

    void start(const long _now);
    void startIfNotStarted(const long _now);
    void stop();
    bool toggle(const long _now);

    void resetTimer(const long _now);

    bool isActive();
    String isActiveReadable();

    long timeLeft(const long _now);

private:
    static const char* TAG;

    bool checkTime(const long _now);
    bool checkTimeIfActive(const long _now);
    long interval;
    long startTime = 0;
    bool active = false;
};