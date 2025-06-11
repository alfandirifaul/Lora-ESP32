#include "LoRaReceiver.h"
#include <LoRa.h>

LoRaReceiver::LoRaReceiver() {
    // Constructor implementation
}

void LoRaReceiver::begin() {
    LoRa.begin(433E6); // Initialize LoRa with the specified frequency
    LoRa.onReceive(onReceive); // Set the receive callback
    LoRa.receive(); // Start listening for incoming packets
}

void LoRaReceiver::onReceive(int packetSize) {
    if (packetSize == 0) return; // No packet received

    String receivedData = "";
    while (LoRa.available()) {
        receivedData += (char)LoRa.read(); // Read the incoming data
    }

    lastReceivedData = receivedData; // Store the last received data
    // Additional processing can be done here
}

String LoRaReceiver::getLastReceivedData() {
    return lastReceivedData; // Return the last received data
}