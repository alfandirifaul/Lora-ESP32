#ifndef LORARECEIVER_H
#define LORARECEIVER_H

#include <Arduino.h>

class LoRaReceiver {
public:
    LoRaReceiver();
    void receiveData();
    String getLastReceivedData() const;

private:
    String lastReceivedData;
};

#endif // LORARECEIVER_H