#include <AltSoftSerial.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <digitalWriteFast.h>

// #define DEBUG
// #define DEBUG_CONTROLLER

// #define CONTROLLER_TO_PANEL_UART_SWITCH_PIN 4
#define WAKEUP_PIN 10
#define CONTROLLER_TO_PANEL_UART_PIN 12
#define BTN_IN 6
#define BTN_OUT 5
#define BRIDGE_UART_RX_PIN 8
#define BRIDGE_UART_TX_PIN 9
#define BTN_OUT 5
#define PANEL_BYTES_PER_PACKET 5
#define CONTROLLER_BYTES_PER_PACKET 6

uint8_t idle_packet[] = {0xA5, 0x0, 0x0, 0x1, 0x1};
uint8_t m_button_packet[] = {0xA5, 0x0, 0x1, 0x1, 0x2};
uint8_t mem1_button_packet[] = {0xA5, 0x0, 0x2, 0x1, 0x3};
uint8_t mem2_button_packet[] = {0xA5, 0x0, 0x4, 0x1, 0x5};

enum DeskStatus {
    UNKNOWN,
    ACTIVE,
    IDLE,
    SLEEPING
};

AltSoftSerial frontPanelSerialBridge; // Serial from front panel to controller

void setup() {

    // Button pressed signal
    pinMode(BTN_IN, INPUT);
    pinMode(BTN_OUT, OUTPUT);

    // Panel to controller uart bridge
    pinMode(BRIDGE_UART_RX_PIN, INPUT);
    pinMode(BRIDGE_UART_TX_PIN, OUTPUT);

    // WAKE UP, WAKEUP, WAKEUP
    pinMode(WAKEUP_PIN, INPUT);

    // Unused?
    pinMode(7, INPUT);
    pinMode(11, INPUT);

    // Controller to panel UART
    pinMode(CONTROLLER_TO_PANEL_UART_PIN, INPUT);

    // pinMode(CONTROLLER_TO_PANEL_UART_SWITCH_PIN, OUTPUT); // Pin that controls a transistor switching pin 12 to TX (hardware uart)
    // digitalWrite(CONTROLLER_TO_PANEL_UART_SWITCH_PIN, LOW);

    frontPanelSerialBridge.begin(9600);
    Serial.begin(255200); // To PC

    Serial.println("DESK_CONTROLLER_v0.1");
}

const long IDLE_TIMEOUT = 60000; // Front panel goes into idle after 60 seconds

int target_button_state = 0;
DeskStatus desk_status = DeskStatus::UNKNOWN;

void set_target_button_state(int button_state) {
    Serial.print("BTN ");
    Serial.println(String(button_state));
    target_button_state = button_state;
}

void print_desk_status(DeskStatus status) {
    if (status == DeskStatus::ACTIVE) {
        Serial.print("A");
    } else if (status == DeskStatus::IDLE) {
        Serial.print("I");
    } else if (status == DeskStatus::SLEEPING) {
        Serial.print("S");
    } else if (status == DeskStatus::UNKNOWN) {
        Serial.print("U");
    }
}

bool change_desk_status(DeskStatus new_status) {
    if (desk_status != new_status) {
        Serial.print("ST: ");
        print_desk_status(desk_status);
        Serial.print("->");
        print_desk_status(new_status);
        Serial.println();
        desk_status = new_status;
        return true;
    }
    return false;
}

void dump_digital() {
    Serial.println("DMP: " + String(digitalRead(BTN_IN)) + " " + String(digitalRead(WAKEUP_PIN)) + " " + String(digitalRead(CONTROLLER_TO_PANEL_UART_PIN)));
}

bool is_blank_controller = true;
int blank_flag = 0;
String controller_packet_buffer = "";

void read_from_controller(int delay) {
    if (delay != 0) {
        delayMicroseconds(delay);
    }
    int current_bit = digitalReadFast(CONTROLLER_TO_PANEL_UART_PIN);
    if (!is_blank_controller) {
        controller_packet_buffer += String(current_bit);
    }
    if (current_bit == 1) {
        blank_flag++;
    } else {
        blank_flag = 0;
        is_blank_controller = false;
    }
    if (blank_flag > 9 && !is_blank_controller) {
        is_blank_controller = true;
        Serial.println(controller_packet_buffer);
        controller_packet_buffer = String();
    }
}

void send_button_event(int button_state) {

    if (desk_status != DeskStatus::ACTIVE) {
        set_target_button_state(-button_state); // Trigger M button before
    } else {
        set_target_button_state(button_state);
    }

    if (desk_status == DeskStatus::SLEEPING) {
        Serial.println("ERR04-DP_SLP");
    }
}

void send_packet_to_controller(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
    frontPanelSerialBridge.write(b1);
    delayMicroseconds(800);
    frontPanelSerialBridge.write(b2);
    delayMicroseconds(800);
    frontPanelSerialBridge.write(b3);
    delayMicroseconds(800);
    frontPanelSerialBridge.write(b4);
    delayMicroseconds(800);
    frontPanelSerialBridge.write(b5);
    Serial.println("A5:0:" + String(control_byte1, HEX) + ":1:" + String(control_byte2, HEX));
    for (int i = 0; i < 200; i++) {
        read_from_controller(42);
    }
}

long last_button_press_time = 0;

int current_panel_byte = 0;
bool rewrite_in_progress = false;
int panel_bytes_rewritten = 0;
long last_panel_packet_time = 0;
String panel_packet_buffer = "";

void loop() {
    long time = millis();
    // Synchronising to the front panel
    if (frontPanelSerialBridge.available() > 0) {
        int val = frontPanelSerialBridge.read();
        if (target_button_state != 0) {
            if (rewrite_in_progress) {
                // Target packet: 0 2 1 3 A5 (Memory preset 1)
                // Target packet: 0 4 1 5 A5 (Memory preset 2)
                if (target_button_state == 1 || target_button_state == 2) {
                    if (current_panel_byte == 0) {
                        val = 0;
                    } else if (current_panel_byte == 1) {
                        val = target_button_state == 1 ? 2 : 4;
                    } else if (current_panel_byte == 2) {
                        val = 1;
                    } else if (current_panel_byte == 3) {
                        val = target_button_state == 1 ? 3 : 5;
                    } else if (current_panel_byte == 4) {
                        val = 0xA5;
                    }
                    panel_bytes_rewritten++;
                    if (panel_bytes_rewritten >= PANEL_BYTES_PER_PACKET * 5) {
                        panel_bytes_rewritten = 0;
                        set_target_button_state(0);
                        rewrite_in_progress = false;
                        last_button_press_time = time;
                    }
                } else if (target_button_state < 0) {
                    // Target packet: 0 1 1 2 A5 (M button)
                    if (current_panel_byte == 0) {
                        val = 0;
                    } else if (current_panel_byte == 1) {
                        val = 1;
                    } else if (current_panel_byte == 2) {
                        val = 1;
                    } else if (current_panel_byte == 3) {
                        val = 2;
                    } else if (current_panel_byte == 4) {
                        val = 0xA5;
                    }
                    panel_bytes_rewritten++;
                    if (panel_bytes_rewritten >= PANEL_BYTES_PER_PACKET * 330) {
                        panel_bytes_rewritten = 0;
                        set_target_button_state(-target_button_state);
                        last_button_press_time = time;
                    }
                }
            } else if (current_panel_byte == 0) {
                // Start at the beginning of the packet (don't start from the middle)
                rewrite_in_progress = true;
            }
        }
        frontPanelSerialBridge.write(val);

        current_panel_byte++;
#ifdef DEBUG
        panel_packet_buffer += String(":") + String(val, HEX);
#endif
        if (val == 0xA5) { // End of packet
            current_panel_byte = 0;
            last_panel_packet_time = time;
#ifdef DEBUG
            Serial.println(panel_packet_buffer);
            panel_packet_buffer = String();
#endif
        }
    } else if (desk_status == DeskStatus::SLEEPING) {
        if (target_button_state != 0) {
            // Synchronising to the controller to panel packets

            // Send some idle packets
            for (int i = 0; i < 350; i++) {
                send_packet_to_controller(0x0, 0x1); // Idle
            }

            // Try pressing the button right away (I don't know how to detect what mode we are in)
            digitalWriteFast(BTN_OUT, HIGH);
            for (int i = 0; i < 5;) {
                // A5 0 2 1 3 (Memory preset 1)
                // A5 0 4 1 5 (Memory preset 2)
                send_packet_to_controller(target_button_state == -1 ? 0x2 : 0x4, target_button_state == -1 ? 0x3 : 0x5);
            }
            digitalWriteFast(BTN_OUT, LOW);

            // Send some idle packets between button presses
            for (int i = 0; i < 30; i++) {
                send_packet_to_controller(0x0, 0x1); // Idle
            }

            digitalWriteFast(BTN_OUT, HIGH);
            // Send some M button packets
            for (int i = 0; i < 330; i++) {
                send_packet_to_controller(0x1, 0x2); // M button
            }
            digitalWriteFast(BTN_OUT, LOW);

            // Send some idle packets between button presses
            for (int i = 0; i < 30; i++) {
                send_packet_to_controller(0x0, 0x1); // Idle
            }

            digitalWriteFast(BTN_OUT, HIGH);
            for (int i = 0; i < 5;) {
                // A5 0 2 1 3 (Memory preset 1)
                // A5 0 4 1 5 (Memory preset 2)
                send_packet_to_controller(target_button_state == -1 ? 0x2 : 0x4, target_button_state == -1 ? 0x3 : 0x5);
            }
            digitalWriteFast(BTN_OUT, LOW);

            // Send some more idle packets and pray it works
            for (int i = 0; i < 2000; i++) {
                send_packet_to_controller(0x0, 0x1); // Idle
            }

            set_target_button_state(0);
        }
    }

#ifdef DEBUG_CONTROLLER
    read_from_controller(0);
#endif

    if (time - last_panel_packet_time > 5000) { // No packet for 5 seconds
        // Flush current buffer
        if (change_desk_status(DeskStatus::SLEEPING) && panel_packet_buffer.length() > 0) {
            Serial.println(panel_packet_buffer);
            panel_packet_buffer = String();
        }
    } else if (time - last_button_press_time >= IDLE_TIMEOUT || last_button_press_time == 0) {
        change_desk_status(DeskStatus::IDLE);
    } else {
        change_desk_status(DeskStatus::ACTIVE);
    }

    if (target_button_state != 0) {
        digitalWriteFast(BTN_OUT, HIGH); // Simulate button press
    } else {
        digitalWriteFast(BTN_OUT, digitalReadFast(BTN_IN)); // Forward pin 6 to pin 5 (button press)
    }

    if (Serial.available() > 0) {
        int val = Serial.read();
        if (val == 49) { // 1 On the keyboard
            send_button_event(1);
        } else if (val == 50) { // 2 On the keyboard
            send_button_event(2);
        } else if (val == 100 || val == 68) { // D or d
            dump_digital();
        } else if (val == 100 || val == 68) { // D or d
            dump_digital();
        } else if (val != 0) {
            Serial.print("UNKNW ");
            Serial.println(String(val));
        }
    }
}