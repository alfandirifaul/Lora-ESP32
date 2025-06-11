#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <Arduino.h>

String getCurrentTime();
String formatUptime(unsigned long seconds);
String getLastDetectionTime(unsigned long lastDetectionTime);

#endif // TIMEUTILS_H