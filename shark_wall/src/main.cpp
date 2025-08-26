#include <Arduino.h>
#include <FastLED.h>

// ========= HUB

#define GH_INCLUDE_PORTAL
#define GH_NO_MQTT
#define GH_NO_STREAM

#include <GyverHub.h>

#define DEV_NAME "Snork"

GyverHub hub(DEV_NAME, DEV_NAME, "f0eb"); // network name, имя устройства, иконка

String ui_input_ssid;
String ui_input_password;
bool ui_manual_mode = true;

double ui_pot1_value = 0;
double ui_pot2_value = 0;
double ui_pot3_value = 0;
int ui_brightness_pot_value = 50;

// ========= STORAGE

#include <FileData.h>
#include <LittleFS.h>

struct FSData {
    int last_mode;
    bool manual_control = true;
    char ssid[MAX_SSID_LEN + 1];
    char password[MAX_PASSPHRASE_LEN + 1];
};
FSData fsdata;
FileData fsdata_handle(&LittleFS, "/data.dat", 'B', &fsdata, sizeof(fsdata));

// ========= GENERAL

bool wifi_connected = false;

// ========= HARDWARE

enum PowerState {
    OFF = 0,
    ON = 1,
    UNKNOWN = 2
};

const int brightness_pot_pin = A0;
const int pot1_pin = A1;
const int pot2_pin = A2;
const int pot3_pin = A3;
const int button_pin = 10;
const int power_switch_pin = 6;
const int transistor_pin = 7;
const int button_led_pin = 9;

#define POT1_MAX_ADC_VALUE 3820
#define POT2_MAX_ADC_VALUE 3820
#define POT3_MAX_ADC_VALUE 3820

#define BRIGHTNESS_POT_MAX_ADC_VALUE 4100

#define ADC_SMOOTHING 0.02

int ui_selected_color;

double pot1_in_value = 0;
double pot2_in_value = 0;
double pot3_in_value = 0;
double brightness_pot_value = 0;
int power_switch_value = 0;
int current_power_state = PowerState::UNKNOWN;

#include <EncButton.h>
Button btn(button_pin);

// ========= LED STRIP

#define NUM_LEDS 778
#define STRIP_PIN 8
#define MAX_BRIGHT 35
#define BRIGHTNESS_SMOOTHING 0.5

CRGB leds[NUM_LEDS];

int brightness_level = 0;

// 2D MAP

#define WIDTH 64
#define HEIGHT 64

byte coordsX[NUM_LEDS] = {89, 90, 92, 93, 95, 96, 98, 100, 101, 103, 104, 106, 107, 109, 111, 112, 114, 115, 117, 119, 120, 122, 123, 125, 127, 128, 130, 131, 133, 135, 136, 138, 139, 141, 142, 144, 146, 147, 149, 150, 152, 154, 155, 157, 158, 159, 161, 162, 162, 163, 164, 165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 180, 181, 183, 184, 185, 185, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 185, 185, 185, 248, 249, 249, 250, 249, 248, 248, 246, 245, 244, 242, 241, 239, 238, 236, 235, 233, 232, 230, 228, 227, 225, 224, 222, 221, 219, 218, 216, 215, 213, 211, 210, 208, 207, 205, 204, 202, 201, 199, 198, 196, 195, 193, 192, 190, 188, 187, 185, 184, 182, 181, 179, 178, 176, 175, 173, 171, 170, 168, 167, 165, 164, 162, 161, 159, 158, 156, 155, 154, 152, 151, 149, 148, 146, 145, 143, 142, 140, 139, 137, 136, 134, 133, 131, 130, 128, 127, 125, 124, 122, 121, 119, 118, 116, 115, 113, 112, 110, 109, 108, 106, 105, 103, 102, 100, 99, 97, 96, 94, 93, 91, 90, 88, 86, 85, 83, 82, 80, 79, 77, 76, 74, 73, 71, 70, 68, 66, 65, 64, 62, 63, 63, 63, 65, 66, 68, 69, 71, 72, 74, 75, 77, 78, 80, 81, 83, 84, 86, 87, 89, 90, 92, 93, 95, 96, 98, 99, 101, 102, 103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 124, 126, 127, 128, 129, 129, 130, 128, 127, 126, 124, 123, 121, 120, 118, 117, 115, 114, 113, 111, 110, 108, 106, 105, 103, 102, 100, 99, 97, 96, 94, 93, 91, 90, 88, 87, 85, 84, 82, 81, 79, 78, 76, 74, 73, 71, 70, 68, 67, 65, 64, 62, 61, 59, 58, 56, 54, 53, 51, 50, 49, 47, 46, 46, 45, 46, 47, 49, 50, 52, 53, 55, 56, 58, 59, 61, 62, 64, 65, 67, 69, 70, 71, 73, 75, 76, 78, 79, 80, 82, 83, 84, 85, 86, 87, 88, 89, 90, 92, 93, 95, 96, 97, 98, 100, 100, 100, 46, 45, 44, 43, 42, 41, 39, 38, 37, 36, 35, 33, 32, 31, 30, 29, 28, 27, 25, 24, 23, 22, 20, 19, 17, 16, 15, 13, 12, 12, 12, 12, 13, 14, 14, 15, 16, 17, 18, 19, 19, 20, 21, 21, 22, 22, 22, 22, 21, 21, 20, 19, 18, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 1, 3, 4, 6, 7, 9, 10, 11, 12, 13, 15, 16, 17, 18, 20, 21, 22, 23, 24, 25, 27, 28, 29, 31, 32, 33, 34, 36, 37, 38, 39, 40, 42, 43, 44, 46, 47, 48, 50, 51, 53, 54, 56, 57, 58, 60, 61, 62, 64, 65, 67, 93, 95, 96, 98, 99, 101, 102, 103, 105, 106, 108, 109, 110, 112, 113, 114, 116, 117, 119, 120, 121, 122, 124, 124, 125, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 130, 130, 131, 131, 132, 132, 132, 133, 134, 134, 136, 137, 138, 140, 140, 141, 142, 142, 143, 144, 145, 146, 146, 147, 148, 149, 150, 150, 151, 152, 152, 153, 153, 215, 214, 214, 214, 213, 212, 211, 210, 208, 207, 207, 207, 206, 208, 209, 210, 132, 133, 135, 136, 138, 140, 141, 143, 144, 146, 147, 149, 150, 152, 153, 155, 157, 158, 160, 161, 163, 164, 166, 167, 169, 170, 172, 173, 175, 177, 178, 180, 181, 183, 184, 186, 187, 189, 190, 192, 194, 195, 197, 198, 200, 201, 203, 204, 206, 207, 209, 210, 212, 213, 215, 216, 218, 219, 221, 222, 224, 225, 227, 228, 230, 231, 233, 234, 235, 237, 238, 239, 241, 242, 243, 245, 246, 247, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255, 254, 253, 252, 251, 250, 248, 247, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 231, 229, 228, 227, 225, 224, 223, 221, 220, 219, 217, 216, 215, 213, 212, 210, 209, 207, 206, 205, 203, 202, 200, 198, 197, 195, 194, 193, 192, 192, 192, 194, 195, 197, 198, 200, 201, 203, 204, 206, 207, 209, 211, 212, 214, 215, 216, 218, 219, 220, 221, 222, 222, 221, 220, 219, 217, 216, 214, 213, 211, 210, 209, 207, 206, 204, 203, 201, 199, 198, 196, 195, 193, 192, 190, 189, 187, 186, 184, 182, 181, 179, 178, 176, 175, 173, 172, 170, 168, 167, 165, 164, 162};
byte coordsY[NUM_LEDS] = {170, 172, 173, 174, 175, 176, 177, 177, 178, 178, 179, 179, 180, 181, 181, 182, 182, 183, 183, 184, 184, 185, 185, 185, 186, 186, 186, 186, 187, 187, 187, 188, 188, 189, 189, 189, 190, 190, 190, 190, 191, 192, 193, 194, 195, 198, 200, 203, 206, 209, 212, 214, 217, 220, 223, 226, 229, 232, 235, 238, 240, 243, 245, 248, 251, 253, 254, 255, 255, 252, 250, 246, 243, 239, 236, 233, 229, 226, 222, 219, 215, 212, 208, 205, 201, 198, 194, 191, 139, 135, 132, 129, 126, 122, 119, 118, 116, 114, 114, 113, 113, 112, 111, 111, 111, 111, 111, 110, 110, 110, 110, 110, 111, 111, 111, 111, 111, 111, 111, 110, 110, 110, 110, 110, 110, 111, 111, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 113, 113, 114, 114, 115, 115, 116, 117, 118, 118, 119, 120, 121, 122, 122, 123, 124, 126, 127, 128, 129, 129, 130, 130, 130, 129, 129, 128, 127, 126, 124, 123, 122, 121, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 108, 107, 106, 105, 104, 103, 102, 101, 101, 100, 100, 99, 99, 98, 98, 97, 97, 96, 96, 95, 95, 95, 94, 94, 94, 94, 94, 94, 95, 96, 97, 99, 102, 105, 108, 109, 110, 111, 112, 113, 114, 114, 115, 116, 116, 117, 118, 119, 120, 121, 122, 122, 123, 124, 125, 126, 127, 128, 128, 129, 131, 132, 134, 135, 136, 137, 138, 139, 140, 141, 142, 142, 143, 144, 144, 145, 146, 147, 149, 152, 155, 158, 160, 162, 164, 164, 163, 163, 162, 162, 161, 159, 158, 156, 155, 155, 154, 154, 153, 153, 152, 151, 151, 150, 149, 148, 148, 147, 147, 146, 146, 146, 146, 145, 145, 145, 144, 144, 144, 143, 143, 142, 141, 141, 140, 140, 140, 140, 140, 140, 140, 141, 141, 141, 142, 144, 145, 147, 151, 154, 156, 158, 160, 161, 161, 161, 162, 162, 162, 163, 163, 163, 163, 163, 163, 163, 164, 165, 166, 167, 167, 168, 169, 171, 172, 175, 177, 180, 182, 185, 188, 190, 191, 192, 192, 192, 192, 191, 188, 186, 183, 179, 163, 166, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 190, 192, 194, 196, 199, 201, 203, 205, 207, 208, 210, 211, 212, 213, 212, 210, 209, 206, 203, 200, 196, 194, 191, 188, 185, 183, 180, 177, 174, 172, 169, 165, 162, 159, 156, 152, 149, 146, 143, 140, 137, 135, 132, 129, 127, 124, 122, 119, 117, 114, 111, 108, 106, 103, 100, 98, 95, 93, 90, 87, 84, 81, 77, 74, 71, 71, 70, 69, 71, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 91, 93, 96, 98, 100, 102, 104, 106, 107, 109, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 131, 133, 133, 132, 130, 129, 128, 127, 126, 124, 123, 121, 120, 119, 118, 117, 95, 94, 93, 92, 91, 90, 88, 87, 86, 85, 84, 83, 81, 80, 79, 77, 76, 75, 74, 73, 71, 69, 67, 65, 62, 59, 56, 53, 50, 47, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 12, 9, 6, 4, 1, 0, 1, 2, 4, 7, 10, 12, 15, 18, 21, 24, 26, 29, 32, 34, 37, 40, 42, 45, 48, 51, 54, 57, 79, 83, 86, 90, 93, 95, 98, 97, 96, 94, 91, 88, 84, 82, 80, 78, 66, 65, 65, 65, 64, 64, 63, 63, 63, 62, 62, 61, 60, 60, 61, 61, 61, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63, 63, 64, 64, 64, 64, 65, 65, 65, 65, 66, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 91, 92, 94, 96, 97, 99, 100, 102, 104, 106, 108, 111, 113, 116, 118, 121, 124, 127, 130, 134, 137, 140, 142, 144, 146, 147, 148, 150, 151, 153, 154, 155, 157, 158, 159, 159, 160, 159, 157, 156, 155, 153, 151, 149, 148, 146, 144, 142, 140, 139, 137, 136, 135, 134, 133, 132, 131, 130, 129, 129, 129, 129, 129, 130, 131, 135, 138, 141, 142, 143, 144, 145, 145, 146, 147, 147, 147, 148, 149, 150, 150, 151, 152, 153, 155, 156, 158, 161, 164, 167, 170, 173, 175, 176, 177, 178, 179, 180, 181, 181, 183, 184, 184, 185, 185, 185, 185, 185, 186, 186, 187, 187, 187, 187, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 188, 189, 189, 189};
byte angles[NUM_LEDS] = {222, 220, 219, 217, 216, 214, 212, 211, 209, 207, 206, 204, 202, 200, 198, 196, 195, 193, 191, 189, 187, 186, 184, 182, 181, 179, 177, 176, 175, 173, 172, 171, 169, 168, 167, 166, 165, 164, 163, 162, 161, 161, 160, 160, 159, 159, 159, 159, 159, 160, 160, 160, 160, 160, 160, 161, 161, 161, 161, 161, 161, 161, 161, 161, 161, 161, 161, 161, 160, 159, 159, 158, 158, 157, 156, 156, 155, 155, 154, 153, 153, 152, 152, 151, 150, 150, 149, 149, 132, 132, 131, 131, 130, 130, 129, 129, 129, 129, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 130, 130, 130, 130, 131, 131, 131, 132, 132, 132, 133, 133, 134, 135, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 145, 146, 146, 147, 147, 147, 148, 149, 150, 152, 153, 155, 159, 164, 171, 182, 195, 213, 227, 238, 245, 249, 252, 255, 1, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 1, 0, 254, 254, 253, 253, 253, 252, 252, 251, 251, 250, 250, 249, 249, 248, 247, 246, 245, 244, 243, 242, 241, 239, 237, 235, 233, 231, 228, 225, 222, 218, 215, 211, 207, 203, 199, 195, 192, 188, 185, 181, 178, 175, 173, 171, 170, 170, 171, 171, 174, 176, 178, 180, 182, 184, 187, 189, 191, 194, 196, 198, 201, 204, 206, 209, 211, 214, 216, 218, 220, 222, 224, 226, 228, 229, 230, 232, 233, 234, 235, 235, 236, 237, 238, 239, 239, 240, 241, 241, 242, 242, 243, 243, 244, 244, 244, 244, 245, 245, 245, 245, 245, 245, 245, 244, 244, 243, 242, 242, 241, 240, 240, 240, 239, 239, 238, 238, 237, 237, 236, 236, 235, 235, 234, 233, 232, 231, 230, 230, 228, 227, 226, 225, 223, 222, 221, 219, 218, 217, 216, 214, 213, 212, 211, 210, 210, 209, 209, 210, 241, 240, 240, 239, 239, 239, 239, 239, 238, 238, 238, 238, 238, 237, 237, 237, 237, 237, 237, 236, 236, 236, 236, 236, 237, 237, 237, 237, 238, 238, 239, 239, 240, 240, 240, 241, 241, 241, 242, 242, 242, 243, 243, 244, 244, 245, 245, 246, 247, 247, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 6, 6, 6, 5, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0, 255, 254, 254, 254, 253, 253, 252, 252, 251, 251, 250, 249, 249, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 250, 250, 250, 250, 251, 251, 9, 10, 12, 13, 15, 18, 20, 23, 26, 30, 34, 38, 43, 47, 52, 56, 61, 65, 69, 72, 75, 77, 79, 80, 80, 80, 80, 79, 79, 79, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 77, 77, 78, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 95, 96, 97, 99, 100, 101, 103, 104, 105, 122, 123, 124, 124, 125, 125, 126, 126, 125, 125, 124, 124, 123, 123, 122, 122, 91, 93, 95, 96, 98, 99, 100, 101, 102, 103, 104, 104, 105, 106, 107, 107, 108, 109, 109, 110, 110, 111, 111, 112, 112, 113, 113, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 117, 117, 118, 118, 118, 119, 119, 119, 119, 120, 120, 120, 120, 121, 121, 121, 122, 122, 122, 122, 123, 123, 123, 123, 124, 124, 124, 124, 124, 125, 125, 125, 125, 126, 126, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 129, 130, 130, 130, 131, 131, 132, 132, 132, 133, 133, 133, 134, 134, 134, 134, 135, 135, 135, 135, 136, 136, 136, 136, 136, 136, 136, 135, 135, 135, 135, 135, 134, 134, 134, 134, 134, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 134, 134, 135, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 137, 137, 138, 138, 139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 143, 143, 143, 144, 144, 144, 145, 145, 145, 146, 146, 147, 147, 147, 148, 148, 149, 149, 149, 150, 150, 151, 151, 152, 152, 153, 154, 154, 155, 156};
byte radii[NUM_LEDS] = {75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 66, 65, 65, 65, 65, 65, 65, 65, 65, 66, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77, 79, 81, 83, 85, 87, 88, 90, 92, 94, 97, 99, 102, 104, 107, 110, 113, 116, 119, 122, 124, 127, 130, 133, 136, 139, 142, 145, 148, 150, 153, 156, 159, 162, 165, 168, 170, 173, 175, 174, 174, 173, 171, 170, 168, 166, 165, 163, 161, 159, 158, 156, 154, 152, 150, 148, 146, 144, 243, 244, 244, 245, 244, 242, 241, 239, 236, 234, 231, 228, 225, 222, 220, 217, 214, 211, 208, 205, 203, 200, 197, 194, 191, 188, 186, 183, 180, 177, 174, 171, 169, 166, 163, 160, 157, 154, 152, 149, 146, 143, 140, 138, 135, 132, 129, 126, 123, 120, 118, 115, 112, 109, 106, 104, 101, 98, 96, 93, 90, 87, 85, 82, 80, 77, 74, 72, 69, 67, 65, 62, 60, 58, 55, 52, 50, 47, 44, 42, 39, 36, 33, 30, 28, 25, 22, 19, 17, 14, 12, 10, 8, 7, 7, 8, 10, 12, 14, 17, 20, 22, 25, 28, 31, 33, 36, 39, 42, 45, 48, 50, 53, 56, 59, 62, 65, 68, 70, 73, 76, 79, 82, 84, 87, 90, 93, 96, 98, 100, 100, 99, 98, 95, 93, 90, 88, 85, 82, 79, 77, 74, 71, 69, 66, 63, 61, 58, 56, 53, 51, 48, 46, 44, 42, 40, 37, 35, 34, 33, 32, 31, 31, 30, 29, 29, 29, 30, 30, 31, 32, 33, 34, 36, 38, 40, 42, 45, 48, 50, 50, 51, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 43, 43, 44, 45, 45, 46, 47, 49, 50, 51, 53, 55, 56, 58, 60, 62, 65, 67, 70, 72, 74, 77, 79, 82, 84, 86, 89, 91, 94, 96, 99, 102, 104, 107, 110, 113, 115, 118, 121, 124, 127, 129, 132, 134, 136, 137, 137, 135, 133, 131, 128, 126, 123, 121, 119, 116, 113, 111, 108, 106, 103, 101, 99, 97, 95, 93, 91, 89, 88, 86, 85, 85, 85, 85, 86, 87, 88, 87, 87, 86, 85, 83, 82, 80, 77, 74, 72, 69, 139, 142, 144, 146, 149, 152, 155, 157, 160, 163, 165, 168, 171, 173, 176, 179, 181, 184, 187, 189, 192, 195, 198, 201, 203, 206, 208, 210, 211, 211, 209, 208, 206, 204, 201, 199, 197, 195, 192, 190, 188, 186, 184, 183, 181, 180, 180, 179, 179, 180, 181, 182, 183, 184, 185, 187, 188, 190, 191, 193, 195, 196, 198, 199, 200, 202, 204, 205, 208, 210, 211, 212, 214, 215, 216, 216, 217, 214, 211, 209, 206, 203, 201, 198, 196, 193, 191, 189, 186, 184, 181, 179, 177, 175, 173, 170, 168, 166, 163, 161, 158, 156, 154, 152, 149, 147, 145, 143, 141, 139, 137, 135, 133, 130, 128, 125, 122, 119, 117, 114, 111, 108, 106, 103, 100, 98, 95, 92, 44, 41, 39, 37, 35, 33, 31, 29, 28, 26, 25, 24, 24, 24, 24, 25, 25, 26, 27, 28, 30, 33, 35, 37, 40, 43, 46, 48, 51, 54, 56, 59, 62, 65, 67, 70, 73, 75, 78, 81, 84, 86, 89, 92, 94, 96, 96, 96, 96, 94, 93, 91, 90, 89, 87, 86, 86, 85, 84, 83, 83, 82, 82, 82, 81, 80, 79, 79, 181, 180, 180, 179, 177, 175, 174, 171, 168, 166, 166, 166, 166, 168, 171, 173, 43, 46, 48, 50, 53, 55, 57, 60, 62, 65, 67, 70, 73, 75, 77, 80, 82, 85, 87, 90, 93, 95, 98, 100, 103, 105, 108, 111, 113, 116, 118, 121, 124, 127, 129, 132, 135, 137, 140, 143, 145, 148, 150, 153, 156, 158, 161, 164, 166, 169, 172, 174, 177, 179, 182, 184, 187, 190, 192, 195, 197, 200, 203, 205, 208, 211, 213, 216, 218, 221, 223, 226, 228, 231, 233, 235, 238, 240, 242, 244, 246, 248, 250, 252, 253, 254, 255, 254, 254, 253, 251, 249, 247, 244, 242, 239, 237, 235, 232, 230, 228, 225, 223, 220, 217, 215, 212, 209, 206, 203, 201, 198, 195, 193, 190, 188, 185, 182, 180, 177, 174, 171, 168, 166, 163, 160, 157, 154, 151, 149, 146, 143, 141, 141, 141, 142, 145, 148, 151, 154, 156, 159, 162, 165, 168, 171, 173, 176, 179, 182, 185, 188, 190, 193, 196, 197, 199, 200, 199, 198, 197, 194, 192, 190, 187, 185, 183, 180, 178, 176, 173, 171, 169, 166, 164, 161, 159, 156, 154, 151, 149, 147, 144, 142, 139, 137, 134, 132, 129, 127, 125, 122, 120, 118, 116, 114, 111, 109};

const TProgmemRGBPalette16 MagmaColor_p FL_PROGMEM = {CRGB::Black, 0x240000, 0x480000, 0x660000, 0x9a1100, 0xc32500, 0xd12a00, 0xe12f17, 0xf0350f, 0xff3c00, 0xff6400, 0xff8300, 0xffa000, 0xffba00, 0xffd400, 0xffffff};
const TProgmemRGBPalette16 WoodFireColors_p FL_PROGMEM = {CRGB::Black, 0x330e00, 0x661c00, 0x992900, 0xcc3700, CRGB::OrangeRed, 0xff5800, 0xff6b00, 0xff7f00, 0xff9200, CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300, 0xffcd00, CRGB::Gold};
const TProgmemRGBPalette16 NormalFire_p FL_PROGMEM = {CRGB::Black, 0x330000, 0x660000, 0x990000, 0xcc0000, CRGB::Red, 0xff0c00, 0xff1800, 0xff2400, 0xff3000, 0xff3c00, 0xff4800, 0xff5400, 0xff6000, 0xff6c00, 0xff7800};
const TProgmemRGBPalette16 NormalFire2_p FL_PROGMEM = {CRGB::Black, 0x560000, 0x6b0000, 0x820000, 0x9a0011, CRGB::FireBrick, 0xc22520, 0xd12a1c, 0xe12f17, 0xf0350f, 0xff3c00, 0xff6400, 0xff8300, 0xffa000, 0xffba00, 0xffd400};
const TProgmemRGBPalette16 LithiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x240707, 0x470e0e, 0x6b1414, 0x8e1b1b, CRGB::FireBrick, 0xc14244, 0xd16166, 0xe08187, 0xf0a0a9, CRGB::Pink, 0xff9ec0, 0xff7bb5, 0xff59a9, 0xff369e, CRGB::DeepPink};
const TProgmemRGBPalette16 SodiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x332100, 0x664200, 0x996300, 0xcc8400, CRGB::Orange, 0xffaf00, 0xffb900, 0xffc300, 0xffcd00, CRGB::Gold, 0xf8cd06, 0xf0c30d, 0xe9b913, 0xe1af1a, CRGB::Goldenrod};
const TProgmemRGBPalette16 CopperFireColors_p FL_PROGMEM = {CRGB::Black, 0x001a00, 0x003300, 0x004d00, 0x006600, CRGB::Green, 0x239909, 0x45b313, 0x68cc1c, 0x8ae626, CRGB::GreenYellow, 0x94f530, 0x7ceb30, 0x63e131, 0x4bd731, CRGB::LimeGreen};
const TProgmemRGBPalette16 AlcoholFireColors_p FL_PROGMEM = {CRGB::Black, 0x000033, 0x000066, 0x000099, 0x0000cc, CRGB::Blue, 0x0026ff, 0x004cff, 0x0073ff, 0x0099ff, CRGB::DeepSkyBlue, 0x1bc2fe, 0x36c5fd, 0x51c8fc, 0x6ccbfb, CRGB::LightSkyBlue};
const TProgmemRGBPalette16 RubidiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x0f001a, 0x1e0034, 0x2d004e, 0x3c0068, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, CRGB::Indigo, 0x3c0084, 0x2d0086, 0x1e0087, 0x0f0089, CRGB::DarkBlue};
const TProgmemRGBPalette16 PotassiumFireColors_p FL_PROGMEM = {CRGB::Black, 0x0f001a, 0x1e0034, 0x2d004e, 0x3c0068, CRGB::Indigo, 0x591694, 0x682da6, 0x7643b7, 0x855ac9, CRGB::MediumPurple, 0xa95ecd, 0xbe4bbe, 0xd439b0, 0xe926a1, CRGB::DeepPink};
const TProgmemRGBPalette16 FlatRainbowColors_p = {
    0xFF0000,
    0xAB5500,
    0xABAB00,
    0x00FF00,
    0x00AB55,
    0x0000FF,
    0x5500AB,
    0xAB0055,
};

// ========= EFFECTS

enum Mode {
    Static = 0,
    Twinkle = 1,
    Rainbow = 2,
    Rainbow2D = 3,
    Forest2D = 4,
    Lava2D = 5,
    Ocean2D = 6,
    Potassium2D = 7,
    Party2D = 8,
    Police = 9
};

int current_mode = Mode::Static;

// ========= MAIN CODE

int current_status_led = 11;
void update_status_led(const CRGB &col) {
    leds[current_status_led] = col;
    FastLED.show();
    current_status_led += 2;
    current_status_led = current_status_led % NUM_LEDS;
}

void init_fs() {
    LittleFS.begin();

    update_status_led(CRGB::SandyBrown); // Filesystem initialised

    FDstat_t stat = fsdata_handle.read();

    switch (stat) {
    case FD_FS_ERR:
    case FD_FILE_ERR:
        update_status_led(CRGB::Red);
        delay(3000);
        break;
    case FD_WRITE:
        update_status_led(CRGB::Lime);
        break;
    case FD_ADD:
        update_status_led(CRGB::MediumAquamarine);
        break;
    case FD_READ:
        update_status_led(CRGB::MediumSeaGreen);
        break;
    default:
        update_status_led(CRGB::Red);
        delay(3000);
        break;
    }

    ui_manual_mode = fsdata.manual_control;
    ui_input_ssid = fsdata.ssid;
    ui_input_password = fsdata.password;
}

void save_to_fs() {
    fsdata_handle.updateNow();
}

void try_connect_to_wifi() {
    if (strlen(fsdata.ssid) > 1) {
        update_status_led(CRGB::MediumPurple);

        WiFi.mode(WIFI_STA);
        WiFi.begin(fsdata.ssid, fsdata.password);

        int iteration = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            update_status_led(CRGB::Coral);
            iteration++;

            if (iteration > 60) {
                break;
            }
        }

        switch (WiFi.status()) {
        case WL_CONNECTED:
            update_status_led(CRGB::Green);
            wifi_connected = true;
            break;
        case WL_CONNECT_FAILED:
        case WL_CONNECTION_LOST:
        case WL_DISCONNECTED:
            update_status_led(CRGB::Aquamarine);
            update_status_led(CRGB::FireBrick);
            delay(3000);
            break;
        case WL_NO_SSID_AVAIL:
            update_status_led(CRGB::Purple);
            update_status_led(CRGB::FireBrick);
            delay(3000);
            break;
        case WL_SCAN_COMPLETED:
            update_status_led(CRGB::OrangeRed);
            update_status_led(CRGB::FireBrick);
            break;
        case WL_IDLE_STATUS:
            update_status_led(CRGB::Chocolate);
            update_status_led(CRGB::FireBrick);
            break;
        default:
            break;
        }
    }

    if (!wifi_connected) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(DEV_NAME);

        update_status_led(CRGB::Aquamarine);
        update_status_led(CRGB::Coral);
        update_status_led(CRGB::Aquamarine);
    }
}

void build_ui(gh::Builder &b) {

    if (!wifi_connected) {

        b.Title("WIFI SETUP");

        b.Input(&ui_input_ssid).maxLen(MAX_SSID_LEN).label("SSID");
        b.Input(&ui_input_password).maxLen(MAX_PASSPHRASE_LEN).label("PASSWORD");

        if (b.Button().label("Save").text("Save").icon("f1eb").click()) {

            strcpy(fsdata.ssid, ui_input_ssid.c_str());
            fsdata.ssid[strlen(ui_input_ssid.c_str())] = '\0';

            strcpy(fsdata.password, ui_input_password.c_str());
            fsdata.password[strlen(ui_input_password.c_str())] = '\0';

            save_to_fs();
            try_connect_to_wifi();
        }

    } else {
        b.Title("SNORK CONTROLLER 9000").fontSize(25);

        if (b.Select(&current_mode).text("Static;Twinkle;Rainbow;Rainbow2D;Forest2D;Lava2D;Ocean2D;Potassium2D;Party2D;Police").label("Mode").click()) {
            b.refresh();
        }

        if (b.Switch(&ui_manual_mode).label("Manual control").click()) {
            fsdata.manual_control = ui_manual_mode;
            b.refresh();
        }

        char *label1 = "P1";
        char *label2 = "P2";
        char *label3 = "P3";

        bool color_selector_enabled = false;

        switch (current_mode) {
        case Mode::Static:
        case Mode::Twinkle:
            label1 = "Red";
            label2 = "Green";
            label3 = "Blue";
            color_selector_enabled = true;
            break;

        case Mode::Forest2D:
        case Mode::Lava2D:
        case Mode::Ocean2D:
        case Mode::Potassium2D:
        case Mode::Party2D:
        case Mode::Rainbow2D:
            label1 = "Color speed";
            label2 = "Brightness speed";
            label3 = "Saturation";
            break;

        case Mode::Rainbow:
            label1 = "Speed";
            label2 = "Step";
            label3 = "Saturation";
            break;

        default:
            break;
        }

        if (ui_manual_mode) {
            {
                gh::Row r(b);
                b.Gauge(&pot1_in_value).range(0, 1, 0.01).unit("").color(gh::Colors::Red).label(label1);
                b.Gauge(&pot2_in_value).range(0, 1, 0.01).unit("").color(gh::Colors::Green).label(label2);
                b.Gauge(&pot3_in_value).range(0, 1, 0.01).unit("").color(gh::Colors::Blue).label(label3);
            }
        } else {

            if (color_selector_enabled) {
                if (b.Color(&ui_selected_color).click()) {
                    ui_pot1_value = ((ui_selected_color >> 16) & 0xFF) / 255.0;
                    ui_pot2_value = ((ui_selected_color >> 8) & 0xFF) / 255.0;
                    ui_pot3_value = (ui_selected_color & 0xFF) / 255.0;
                    b.refresh();
                }
            }

            b.Slider(&ui_pot1_value).range(0, 1, 0.01).unit("").color(gh::Colors::Red).label(label1);
            b.Slider(&ui_pot2_value).range(0, 1, 0.01).unit("").color(gh::Colors::Green).label(label2);
            b.Slider(&ui_pot3_value).range(0, 1, 0.01).unit("").color(gh::Colors::Blue).label(label3);
        }

        if (ui_manual_mode) {
            int bright_percent = (1.0 - brightness_pot_value) * 100;
            b.Gauge(&bright_percent).range(0, 100, 1).unit("%").color(gh::Colors::Yellow).label("Brightness");
        } else {
            b.Slider(&ui_brightness_pot_value).range(0, 100, 1).unit("%").color(gh::Colors::Yellow).label("Brightness");
        }
    }
}

void init_webui() {
    hub.setVersion("0.1.0-nice");
    hub.onBuild(build_ui);
    hub.begin();

    update_status_led(CRGB::YellowGreen);
}

void setup() {

    pinMode(brightness_pot_pin, INPUT);
    pinMode(pot1_pin, INPUT);
    pinMode(pot2_pin, INPUT);
    pinMode(pot3_pin, INPUT);
    pinMode(button_pin, INPUT_PULLDOWN);
    pinMode(power_switch_pin, INPUT_PULLDOWN);
    pinMode(button_led_pin, OUTPUT);
    pinMode(transistor_pin, OUTPUT);

    FastLED.addLeds<WS2815, STRIP_PIN>(leds, NUM_LEDS); // GRB ordering is assumed
    // FastLED.setDither(0); // Disable brightness dithering
    FastLED.setBrightness(MAX_BRIGHT * 0.5);
    FastLED.setCorrection(LEDColorCorrection::TypicalPixelString);

    digitalWrite(transistor_pin, HIGH);

    update_status_led(CRGB::Orange); // Initial setup done

    update_status_led(CRGB::Black);
    init_fs();
    update_status_led(CRGB::Black);
    try_connect_to_wifi();
    update_status_led(CRGB::Black);
    init_webui();
    update_status_led(CRGB::Black);
}

// https://pinout.uno/articles/analog-signal-noise-filtering-on-arduino/
int read_median(int pin, int samples) {
    // storage array
    int raw[samples];
    // read the input and put the value in the array cells
    for (int i = 0; i < samples; i++) {
        raw[i] = analogRead(pin);
    }
    // sort the array in ascending order of cell values
    int temp = 0; // temporary variable

    for (int i = 0; i < samples; i++) {
        for (int j = 0; j < samples - 1; j++) {
            if (raw[j] > raw[j + 1]) {
                temp = raw[j];
                raw[j] = raw[j + 1];
                raw[j + 1] = temp;
            }
        }
    }
    // return the value of the middle cell of the array
    return raw[samples / 2];
}

void read_pot(int pin, double *value, int max_value) {
    *value = min((*value * (1 - ADC_SMOOTHING) + (read_median(pin, 15) / (double)max_value) * ADC_SMOOTHING), 1.0);
}

// EFFECT FUNCS

#pragma region helper_funcs

double fract(double x) { return x - int(x); }

double mix(double a, double b, double t) { return a + (b - a) * t; }

double step(double e, double x) { return x < e ? 0.0 : 1.0; }

CRGB hsv_offset(CRGB col, double h_offset, double s_offset, double v_offset) {

    double r = col.r / 255.0;
    double g = col.g / 255.0;
    double b = col.b / 255.0;

    double s = step(b, g);
    double px = mix(b, g, s);
    double py = mix(g, b, s);
    double pz = mix(-1.0, 0.0, s);
    double pw = mix(0.6666666, -0.3333333, s);
    s = step(px, r);
    double qx = mix(px, r, s);
    double qz = mix(pw, pz, s);
    double qw = mix(r, px, s);
    double d = qx - min(qw, py);

    double h_ = abs(qz + (qw - py) / (6.0 * d + 1e-10)) + h_offset;
    double s_ = d / (qx + 1e-10) + s_offset;
    double v_ = qx + v_offset;

    col.r = constrain(v_ * mix(1.0, constrain(abs(fract(h_ + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s_) * 255, 0, 255);
    col.g = constrain(v_ * mix(1.0, constrain(abs(fract(h_ + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s_) * 255, 0, 255);
    col.b = constrain(v_ * mix(1.0, constrain(abs(fract(h_ + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s_) * 255, 0, 255);

    return col;
}

void fill_color(const CRGB &color) {
    for (int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = color;
    }
}

void rainbow_wave(double speed, double delta_hue, double saturation_offset) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = hsv_offset(ColorFromPalette(FlatRainbowColors_p, (i + beat8(40 * speed)) * delta_hue, 255), 0, saturation_offset - 1, 0);
    }
}

void display_2d_palette(const TProgmemRGBPalette16 &pal, double color_speed, double brightness_speed, double saturation_offset) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = hsv_offset(ColorFromPalette(pal, angles[i] + beatsin8(50 * color_speed, 35, 255, 0, coordsX[i] - coordsY[i]), beatsin8(80 * brightness_speed, 35, 255, 0, radii[i] + coordsY[i]), TBlendType::LINEARBLEND), 0, saturation_offset - 1, 0);
    }
}

#pragma endregion

#pragma region sparkle

int sparkle_pos = 0;
CRGB target_sparkle_frame[NUM_LEDS];
CRGB sparkle_current_frame[NUM_LEDS];

void single_sparkle(fl::u8 red, fl::u8 green, fl::u8 blue, int pixel_index) {
    CRGB col = CRGB(red, green, blue);
    if (pixel_index > 0) {
        target_sparkle_frame[pixel_index - 1] = col;
    }
    target_sparkle_frame[pixel_index] = col;
    if (pixel_index < NUM_LEDS - 1) {
        target_sparkle_frame[pixel_index + 1] = col;
    }
}

void sparkle(fl::u8 red, fl::u8 green, fl::u8 blue) {
    if (sparkle_pos == -25) {
        for (int i = 0; i < NUM_LEDS; i++) {
            target_sparkle_frame[i] = CRGB::Black;
        }
        for (int i = 0; i < NUM_LEDS / 10; i++) {
            int pixel_index = random(NUM_LEDS);
            single_sparkle(red, green, blue, pixel_index);
        }
    }

    if (sparkle_pos <= 0) {
        double fade_fract = 0.05;

        for (int i = 0; i < NUM_LEDS; i++) {
            sparkle_current_frame[i].r = sparkle_current_frame[i].r * (1 - fade_fract) + target_sparkle_frame[i].r * fade_fract;
            sparkle_current_frame[i].g = sparkle_current_frame[i].g * (1 - fade_fract) + target_sparkle_frame[i].g * fade_fract;
            sparkle_current_frame[i].b = sparkle_current_frame[i].b * (1 - fade_fract) + target_sparkle_frame[i].b * fade_fract;
        }
    } else {

        double speed_mod = 1 + sparkle_pos / 100.0;

        for (int i = 0; i < NUM_LEDS; i++) {
            sparkle_current_frame[i].r /= 1.03 * speed_mod; // Exponential (double exponential?) fall-off, 1/4 faster at the end
            sparkle_current_frame[i].g /= 1.03 * speed_mod;
            sparkle_current_frame[i].b /= 1.03 * speed_mod;
        }
    }

    sparkle_pos += 1;

    if (sparkle_pos > 15) {
        sparkle_pos = -25;
        for (int i = 0; i < NUM_LEDS; ++i) {
            sparkle_current_frame[i] = CRGB::Black;
        }
    }

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = constrain(sparkle_current_frame[i].r + 4, 0, 255);
        leds[i].g = constrain(sparkle_current_frame[i].g + 4, 0, 255);
        leds[i].b = constrain(sparkle_current_frame[i].b + 20, 0, 255);
    }

    FastLED.show();
}

#pragma endregion

#pragma region update_funcs

void update_brightness() {
    double brightness_source = ui_manual_mode ? (1.0 - brightness_pot_value) : ui_brightness_pot_value / 100.0;

    brightness_level = round(brightness_level * (1 - BRIGHTNESS_SMOOTHING) + brightness_source * MAX_BRIGHT * BRIGHTNESS_SMOOTHING);

    if (brightness_level == 0 || power_switch_value == LOW) {
        if (current_power_state) {
        }
        digitalWrite(transistor_pin, LOW);
        pinMode(STRIP_PIN, INPUT);
    } else {
        pinMode(STRIP_PIN, OUTPUT);
        digitalWrite(transistor_pin, HIGH);
        FastLED.setBrightness(brightness_level);
    }
}

void update_effects() {

    double parameter1 = ui_pot1_value;
    double parameter2 = ui_pot2_value;
    double parameter3 = ui_pot3_value;

    if (fsdata.manual_control) {
        parameter1 = pot1_in_value;
        parameter2 = pot2_in_value;
        parameter3 = pot3_in_value;
    }

    switch (current_mode) {
    case Mode::Static:
        fill_color(CRGB(parameter1 * 255, parameter2 * 255, parameter3 * 255));
        break;
    case Mode::Rainbow:
        rainbow_wave(parameter1, parameter2, parameter3);
        break;
    case Mode::Twinkle:
        sparkle(parameter1 * 255, parameter2 * 255, parameter3 * 255);
        break;
    case Mode::Rainbow2D:
        display_2d_palette(RainbowColors_p, parameter1, parameter2, parameter3);
        break;
    case Mode::Lava2D:
        display_2d_palette(LavaColors_p, parameter1, parameter2, parameter3);
        break;
    case Mode::Forest2D:
        display_2d_palette(ForestColors_p, parameter1, parameter2, parameter3);
        break;
    case Mode::Ocean2D:
        display_2d_palette(OceanColors_p, parameter1, parameter2, parameter3);
        break;
    case Mode::Potassium2D:
        display_2d_palette(PotassiumFireColors_p, parameter1, parameter2, parameter3);
        break;
    case Mode::Party2D:
        display_2d_palette(PartyColors_p, parameter1, parameter2, parameter3);
        break;
    case Mode::Police:
        draw_magma();
        break;
    default:
        break;
    }

    FastLED.show();
}

#pragma endregion

void loop() {

    hub.tick();
    btn.tick();

    read_pot(pot1_pin, &pot1_in_value, POT1_MAX_ADC_VALUE);
    read_pot(pot2_pin, &pot2_in_value, POT2_MAX_ADC_VALUE);
    read_pot(pot3_pin, &pot3_in_value, POT3_MAX_ADC_VALUE);
    read_pot(brightness_pot_pin, &brightness_pot_value, BRIGHTNESS_POT_MAX_ADC_VALUE);
    power_switch_value = digitalRead(power_switch_pin);

    update_brightness();
    update_effects();

    // if (time_passed > 2000) {
    //     if (ui_manual_mode) {
    //         hub.update("P1;P2;P3");
    //         time_passed = 0;
    //     }
    // }

    if (btn.press()) {
        current_mode++;
        if (current_mode > Mode::Police) {
            current_mode = Mode::Static;
        }
    }

    digitalWrite(button_led_pin, digitalRead(button_pin));
}