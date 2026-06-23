#include <cstdint>
#include <Arduino.h>

class AnalogReader {
    private:
        static const int arraySize = 8;

        int pin;
        float voltagePin;

        int index = 0;
        int sum = 0;
        int rawValues[arraySize] = {0};

        int avgRawValue_ = 0;
        int rawValue_ = 0;
        int milliVoltsValue_ = 0;

    public:
    const int& avgRawValue = avgRawValue_;
    const int& rawValue = rawValue_;
    const int& milliVoltsValue = milliVoltsValue_;


    AnalogReader(int analogPin, float voltagePin = 3.3) : pin(analogPin), voltagePin(voltagePin) {
    }

    void setup() {
        analogReadResolution(12);
        analogSetAttenuation(ADC_11db);
    }

    void read() {
        rawValue_ = analogRead(pin);
        milliVoltsValue_ = analogReadMilliVolts(pin);

        sum -= rawValues[index];
        rawValues[index] = rawValue_;
        sum += rawValues[index];
        index = (index + 1) % arraySize;
        avgRawValue_ = sum / arraySize;
    }

    float avgVoltage() {
        return avgRawValue_ * (voltagePin / 4095.0);
    }

    float percentageErrorVoltage() {
        float miliVoltage = (rawValue_ * (voltagePin / 4095.0)) * 1000;
        return (milliVoltsValue_ - miliVoltage) / miliVoltage * 100;
    }
};