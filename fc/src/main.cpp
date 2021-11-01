#include "Particle.h"
#include "settings.h"
#include "DataQueue.h"
#include "Led.h"

#include "Sensor.h"
#include "SensorGps.h"
#include "SensorThermo.h"
#include "SensorSigStrength.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SensorGps gps(new SFE_UBLOX_GNSS(), GPS_UPDATE_FREQUENCY);
SensorThermo thermo1(&SPI, A5);
SensorThermo thermo2(&SPI, A4);
SensorSigStrength sigStrength;


Sensor *sensors[4] = {&gps, &thermo1, &thermo2, &sigStrength};

Led led_orange(A0, 63);
// Blue LED to flash on startup, go solid when valid time has been established
Led led_blue(D7, 255);
Led led_green(D8, 40);

DataQueue dataQ;

uint32_t lastPublish = 0;

/**
 * Publishes a new message to Particle Cloud
 * */
void publishMessage() {
    long start, json_build_time;
    if (DEBUG_CPU_TIME) {
        start = micros();
    }

    if (DEBUG_CPU_TIME) {
        json_build_time = micros() - start;
    }

    DEBUG_SERIAL("------------------------");
    if(PUBLISH_ENABLED){
        DEBUG_SERIAL("Publish - ENABLED - Message: ");
        // Publish to Particle Cloud
        DEBUG_SERIAL(dataQ.publish("Proto", PRIVATE, WITH_ACK));
    }else{
        DEBUG_SERIAL("Publish - DISABLED - Message: ");
        DEBUG_SERIAL(dataQ.resetData());
    }

    // Any sensors that are working but not yet packaged for publish
    DEBUG_SERIAL("\nNot in Message: ");
    DEBUG_SERIAL("Probe Temperature (Thermo1): " + String(thermo1.getProbeTemp()) + "C");
    DEBUG_SERIAL("Probe Temperature (Thermo2): " + String(thermo2.getProbeTemp()) + "C");
    DEBUG_SERIAL("Internal Temperature (Thermo2): " + String(thermo2.getInternalTemp()) + "C");
    DEBUG_SERIAL("Longitude: " + String(gps.getLongitude()));
    DEBUG_SERIAL("Latitude: " + String(gps.getLatitude()));
    DEBUG_SERIAL("Horizontal Speed: " + String(gps.getHorizontalSpeed()) + "m/s");
    DEBUG_SERIAL("Horizontal Acceleration: " + String(gps.getHorizontalAcceleration()) + "m/s^2");
    DEBUG_SERIAL("Altitude: " + String(gps.getAltitude()) + "m");
    DEBUG_SERIAL("Vertical Acceleration: " + String(gps.getHorizontalAcceleration()) + "m/s^2");
    DEBUG_SERIAL("Horizontal Accuracy: " + String(gps.getHorizontalAccuracy()) + "m");
    DEBUG_SERIAL("Vertical Accuracy: " + String(gps.getVerticalAccuracy()) + "m");    
    DEBUG_SERIAL("Satellites in View: " + String(gps.getSatellitesInView()));  
    DEBUG_SERIAL("Time (UTC): " + Time.timeStr());
    DEBUG_SERIAL("Signal Strength: " + String(sigStrength.getStrength()) + "%");
    DEBUG_SERIAL("Signal Quality: " + String(sigStrength.getQuality()) + "%");

    DEBUG_SERIAL();
    
    if(DEBUG_MEM){
        DEBUG_SERIAL("\nFREE RAM: " + String(System.freeMemory()) + "B / 128000B");
    }

    // Output CPU time in microseconds spent on each task
    if (DEBUG_CPU_TIME) {
        DEBUG_SERIAL("\nCPU Time:");
        DEBUG_SERIAL("Build JSON Message: " + String(json_build_time) + "us");
        for (Sensor *s : sensors) {
            DEBUG_SERIAL(s->getHumanName() + " handle: " + String(s->getLongestHandleTime()) + "us");
        }
        DEBUG_SERIAL();
    }
}

/**
 * 
 * SETUP
 * 
 * */
void setup() {
    if(DEBUG_SERIAL_ENABLE){
        Serial.begin(115200);
    }

    // Start i2c with clock speed of 400 KHz
    // This requires the pull-up resistors to be removed on i2c bus
    Wire.setClock(400000);
    Wire.begin();

    Time.zone(TIME_ZONE);

    for (Sensor *s : sensors) {
        s->begin();
    }

    led_blue.flashRepeat(500);

    DEBUG_SERIAL("TELEMETRY ONLINE - FC");
}

/**
 * 
 * LOOP
 * 
 * */
void loop() {
    // Sensor Handlers
    for (Sensor *s : sensors) {
        if (DEBUG_CPU_TIME) {
            s->benchmarkedHandle();
        } else {
            s->handle();
        }
    }

    dataQ.loop();

    // LED Handlers
    led_orange.handle();
    led_blue.handle();
    led_green.handle();

    // Publish a message every publish interval
    if (millis() - lastPublish >= PUBLISH_INTERVAL_MS){
        // If no valid time pulled from cellular, attempt to get valid time from GPS (should be faster)
        if(!Time.isValid()){
            if(gps.getTimeValid()){
                led_blue.on();
                Time.setTime(gps.getUnixTime());
            }
        }else{
            led_blue.on();
        }

        lastPublish = millis();
        publishMessage();
    }

    
}



