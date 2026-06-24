#include <cstdint>
#include <Arduino.h>

class AnalogReader {
    private:
        static const int arraySize = 8;

        int pin;

        int index = 0;
        int sum = 0;
        int rawValues[arraySize] = {0};

        int avgRawValue_ = 0;
        int rawValue_ = 0;

    public:
    const int& avgValue = avgRawValue_;
    const int& value = rawValue_;

    AnalogReader(int analogPin) : pin(analogPin) {
    }

    void read() {
        rawValue_ = analogRead(pin);

        sum -= rawValues[index];
        rawValues[index] = rawValue_;
        sum += rawValues[index];
        index = (index + 1) % arraySize;
        avgRawValue_ = sum / arraySize;
    }
};