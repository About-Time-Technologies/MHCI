#include "Timeout.hpp"

const char* Timeout::TAG = "Timeout";

void Timeout::setInterval(const long _interval) {
    interval = _interval;
    ESP_LOGV(TAG, "Updated timer interval from %i to %i", _interval, interval);
}

Timeout::Timeout(const long _interval) {
    setInterval(_interval);
}

Timeout::~Timeout() {}

long Timeout::getInterval() {
    return interval;
}

void Timeout::start(const long _now) {
    startTime = _now;
    active = true;
}

void Timeout::startIfNotStarted(const long _now) {
    if (active) return;

    start(_now);
}

void Timeout::stop() {
    active = false;
}

bool Timeout::toggle(const long _now) {
    if (active) {
        stop();
    } else {
        start(_now);
    }

    return active;
}

void Timeout::resetTimer(const long _now) {
    startTime = _now;
}

bool Timeout::checkTimeout(const long _now) {
    if (!checkTimeIfActive(_now)) return false;

    // Timeout has occured so we will stop the active state and return true
    stop();
    return true;
}

bool Timeout::checkTimeoutAndRestart(const long _now) {
    if (!checkTimeIfActive(_now)) return false;

    // Timeout has occured so we will stop the active state and return true
    start(_now);
    return true;
}

bool Timeout::checkTimeoutNoStop(const long _now) {
    return checkTimeIfActive(_now);
}

bool Timeout::isActive() {
    return active;
}

String Timeout::isActiveReadable() {
    if (active) 
        return "active";
    else 
        return "inactive";
}


bool Timeout::checkTime(const long _now) {
    if (_now - startTime < interval) {
        return false;
    }

    return true;
}

bool Timeout::checkTimeIfActive(const long _now) {
    if (!active) return false;

    return checkTime(_now);
}

long Timeout::timeLeft(const long _now) {
    if (!active) return interval;

    return ((_now - startTime) - interval);
}