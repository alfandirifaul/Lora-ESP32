#include "TimeUtils.h"

unsigned long millisToSeconds(unsigned long millis) {
    return millis / 1000;
}

String formatTime(unsigned long seconds) {
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;

    String formattedTime = String(hours) + "h " + String(minutes) + "m " + String(secs) + "s";
    return formattedTime;
}