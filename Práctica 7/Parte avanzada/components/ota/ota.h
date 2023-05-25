#ifndef OTA_H
#define OTA_H
void ota_init();
void ota_update();
void verify_image(bool (*diagnostic)());
#endif