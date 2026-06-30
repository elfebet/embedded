#pragma once
#include <Arduino.h>

class LaserDetector {
private:
    const uint8_t _ldrPin;
    const int _threshold;
    bool _isMaskingMode = false;
    bool _isLaserDetected = false;
public:
    const bool& isLaserDetected = _isLaserDetected;
    const bool& isMaskingMode = _isMaskingMode;
    
    LaserDetector(uint8_t ldrPin, int threshold = 3000);
    void detectLaser();
    void toggleMaskingMode();
    void resetDetection();
};