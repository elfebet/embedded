#include "LaserDetector.h"

LaserDetector::LaserDetector(uint8_t ldrPin, int threshold) : _ldrPin(ldrPin), _threshold(threshold) {
}

void LaserDetector::detectLaser() {
    int ldrValue = analogRead(_ldrPin); // 0..4095
    if (ldrValue > _threshold && !_isLaserDetected && !_isMaskingMode) {
        _isLaserDetected = true;
        Serial.printf("Laser detected!!! ADC: %d\n", ldrValue);
    }
}

void LaserDetector::toggleMaskingMode() {
    _isMaskingMode = !_isMaskingMode;
    Serial.printf("Masking mode: %s\n", _isMaskingMode ? "ON" : "OFF");
    if (_isMaskingMode) {
        _isLaserDetected = false;
    }
}

void LaserDetector::resetDetection() {
    _isLaserDetected = false;
    Serial.println("Did reset detection.");
}
