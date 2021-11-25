#include "settings.h"
#include "vehicle.h"
#include "Led.h"
#include "Button.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void buttonPushed();
void publish(String payload, DataQueue::PublishStatus status);

Led ledOrange(A0, 63);
Led ledBlue(D7, 255);
Led ledGreen(D8, 40);
Button button(A2, true, false, buttonPushed, NULL);

DataQueue dataQ(VEHICLE_NAME, publish);
Dispatcher *dispatcher;

bool loggingEnabled = LOGGING_EN_AT_BOOT;
bool error = false;
long unsigned int lastDebugSensor = 0;

// Wrapper for current time, in case it's invalid
String currentTime(){
    if(Time.isValid()){
        return Time.timeStr();
    }else{
        return "NOT VALID";
    }
}

// Publish a message
void publish(String payload, DataQueue::PublishStatus status) {
    switch (status) {
        case DataQueue::PublishingAtMaxFrequency:
            DEBUG_SERIAL_LN("WARNING: Currently Publishing at Max Frequency");
            error = false;
            break;
        case DataQueue::DataBufferOverflow:
            DEBUG_SERIAL_LN("ERROR: Json Writer Data Buffer has Overflowed");
            error = true;
            break;
        default:
            error = false;
            break;
    }

    DEBUG_SERIAL_LN("---- PUBLISH MESSAGE ----");
    DEBUG_SERIAL_LN(String(VEHICLE_NAME) + " - Publish " + (PUBLISH_EN ? "ENABLED" : "DISABLED") + " - " + currentTime());
    DEBUG_SERIAL_LN(payload);
    DEBUG_SERIAL_LN("");
    DEBUG_SERIAL_LN("Publish Queue Size: " + String(dataQ.getNumEventsInQueue()));
    DEBUG_SERIAL_LN("");
}

// Output sensor data over serial
void debugSensors(){
    DEBUG_SERIAL_LN("---- SENSOR DATA ----");
    DEBUG_SERIAL_LN(String(VEHICLE_NAME) + " - " + currentTime());
    CurrentVehicle::debugSensorData();

    DEBUG_SERIAL_LN("Free Memory: " + String(System.freeMemory()/1000) + "kB / 128kB");
    DEBUG_SERIAL_LN("");
}

// Toggle logging enabled on and off
void buttonPushed(){
    if(loggingEnabled){
        loggingEnabled = false;
        DEBUG_SERIAL_LN("#### Logging has been DISABLED (button)");
    }else{
        loggingEnabled = true;
        DEBUG_SERIAL_LN("#### Logging has been ENABLED (button)");
    }
}

// Handle User Interface Functionality
void handleUI(){
    ledOrange.handle();
    ledBlue.handle();
    ledGreen.handle();
    button.handle();

    // Green Light Behaviour
    if(Time.isValid()){
        ledGreen.on();
    }else{
        ledGreen.flashRepeat(LED_FLASH_INT);
    }

    // Blue LED Behaviour
    if(loggingEnabled){
        if(Time.isValid() && !error){
            ledBlue.on();
        }else{
            ledBlue.flashRepeat(LED_FLASH_INT);
        }
    }else{
        ledBlue.off();
    }

    // Orange LED Behaviour
    if(error){
        ledOrange.flashRepeat(LED_FLASH_INT);
    }else{
        ledOrange.off();
    }

}

// Enable Logging Remotely
int remoteEnableLogging(String command){
    loggingEnabled = true;
    DEBUG_SERIAL_LN("#### Logging has been ENABLED (remote)");
    return 1;
}

// Disable Logging Remotely
int remoteDisableLogging(String command){
    loggingEnabled = false;
    DEBUG_SERIAL_LN("#### Logging has been DISABLED (remote)");
    return 1;
}

/**
 * SETUP
 * */
void setup() {
    if(DEBUG_SERIAL_EN){
        Serial.begin(115200);
    }

    // Start i2c with clock speed of 400 KHz
    // This requires the pull-up resistors to be removed on i2c bus
    Wire.setClock(400000);
    Wire.begin();

    // Define Remote Functions
    Particle.function("enableLogging", remoteEnableLogging);
    Particle.function("disableLogging", remoteDisableLogging);

    Time.zone(TIME_ZONE);

    for (unsigned i = 0; sensors[i]; i++) {
        sensors[i]->begin();
    }

    dispatcher = CurrentVehicle::buildDispatcher();

    DEBUG_SERIAL_LN("---- TELEMETRY ONLINE - " + String(VEHICLE_NAME) + " ----");
}

/**
 * LOOP
 * */
void loop() {
    // Sensor Handlers
    for (unsigned i = 0; sensors[i]; i++) {
        sensors[i]->handle();
    }

    dataQ.loop();

    if(loggingEnabled && Time.isValid()){
        dispatcher->loop();
    }

    handleUI();

    if(DEBUG_SENSOR_INT && millis() > lastDebugSensor + (DEBUG_SENSOR_INT * 1000)){
        lastDebugSensor = millis();
        debugSensors();
    }

    // If no valid time pulled from cellular, attempt to get valid time from GPS
    if(!Time.isValid()){
        if(gps.getTimeValid()){
            Time.setTime(gps.getUnixTime());
        }
    }

}

