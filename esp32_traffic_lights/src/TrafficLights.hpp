#include <cstdint>
#include <Arduino.h>

class TrafficLights {
public:
    enum LightMode {
        ALL_OFF,
        ALL_ON,
        TRAFFIC_LIGHTS
    };

private:
    enum LightSpeed {
        SLOW = 1000,
        MEDIUM = 500,
        FAST = 200
    };

    const uint8_t* _pins;
    uint8_t _numPins;
    LightSpeed _speed = MEDIUM;
    LightMode _mode = TRAFFIC_LIGHTS;
    unsigned long _previousMillis = 0;
    uint8_t _currentPin = 0;

    void setPinsState(uint8_t val) {
        for (int i = 0; i < _numPins; i++) {
            digitalWrite(_pins[i], val);
        }
    }

public:
    TrafficLights(const uint8_t pins[], uint8_t numPins) {
        _pins = pins;
        _numPins = numPins;
    }

    void setup() {
        Serial.printf("Setup with %d pins\n", _numPins);
        for (int i = 0; i < _numPins; i++) {
            pinMode(_pins[i], OUTPUT);
        }
        for (int i = 0; i < _numPins; i++) {
            digitalWrite(_pins[i], (_currentPin == i) ? HIGH : LOW);
        }
    }

    void changeSpeed() {
        switch (_speed) {
            case SLOW:
                _speed = MEDIUM;
                break;
            case MEDIUM:
                _speed = FAST;
                break;
            case FAST:
                _speed = SLOW;
                break;
        }
        // force setup traffic lights mode
        setMode(TRAFFIC_LIGHTS);
    }

    LightMode getMode() {
        return _mode;
    }

    void setMode(LightMode mode) {
        if (_mode == mode) return;

        switch (mode) {
            case ALL_OFF:
                setPinsState(LOW);
                break;
            case ALL_ON:
                setPinsState(HIGH);
                break;
            case TRAFFIC_LIGHTS:
                _previousMillis = 0; // reset the timer
                break;
        }
        _mode = mode;
    }

    void update() {
        if (_mode != TRAFFIC_LIGHTS) return;
        unsigned long currentMillis = millis();

        if (currentMillis - _previousMillis >= _speed) {
            _previousMillis = currentMillis;
            _currentPin = (_currentPin + 1) % _numPins;

            Serial.printf("Activated %d pin\n", _currentPin);
            for (int i = 0; i < _numPins; i++) {
                digitalWrite(_pins[i], (_currentPin == i) ? HIGH : LOW);
            }
        }
    }
};