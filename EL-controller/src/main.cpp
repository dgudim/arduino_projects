#include <Arduino.h>
#include <GyverButton.h>
#include <SoftPWM.h>
#include <math.h>

#define LED_A0 A0
#define LED_A1 A1
#define LED_A2 A2
#define BUTTON_A 3

#define LED_B0 A3
#define LED_B1 A4
#define LED_B2 A5
#define BUTTON_B 2

#define HALF_BRIGHTNESS percentToDuty(50)
#define BREATH_MIN percentToDuty(20)
#define BREATH_MAX percentToDuty(100)

#define FADE_PERIOD_MS 3000
#define FADE_SPREAD 1.6f

#define BREATH_PERIOD_MS 4000
#define HEARTBEAT_PERIOD_MS 1200
#define HEARTBEAT_MIN percentToDuty(8)
#define HEARTBEAT_MAX percentToDuty(100)

#define BUTTON_LOCKOUT_MS 400
#define STAGGERED_TURN_ON_LED_MS 400

#define PWM_SLEW_UP_STEP 2
#define PWM_SLEW_DOWN_STEP 8

#define PWM_FREQ_HZ 120

SOFTPWM_DEFINE_CHANNEL(0, DDRC, PORTC, PORTC0); // A0
SOFTPWM_DEFINE_CHANNEL(1, DDRC, PORTC, PORTC1); // A1
SOFTPWM_DEFINE_CHANNEL(2, DDRC, PORTC, PORTC2); // A2
SOFTPWM_DEFINE_CHANNEL(3, DDRC, PORTC, PORTC3); // A3
SOFTPWM_DEFINE_CHANNEL(4, DDRC, PORTC, PORTC4); // A4
SOFTPWM_DEFINE_CHANNEL(5, DDRC, PORTC, PORTC5); // A5
SOFTPWM_DEFINE_OBJECT(6);

enum Effect : uint8_t {
    EFFECT_OFF = 0,
    EFFECT_FULL,
    EFFECT_HALF,
    EFFECT_FADE,
    EFFECT_BREATH,
    EFFECT_HEARTBEAT,
    EFFECT_COUNT
};

struct Bank {
    uint8_t pwmBaseOffset;
    Effect activeEffect;
    uint32_t effectStartMs;
    uint32_t lastClickMs;
    uint8_t pwmTarget[3];
    bool staggerTurnOnActive;
};

static GButton buttons[2] = {GButton(BUTTON_A), GButton(BUTTON_B)};
static Bank banks[2];
static uint8_t pwmDuty[6];

constexpr uint8_t percentToDuty(uint8_t percent) {
    return percent >= 100 ? 255 : (uint16_t)percent * 255 / 100;
}

static float smoothstep(float x) {
    return x * x * (3.0f - 2.0f * x);
}

static float cyclePhase(uint32_t elapsedMs, uint32_t periodMs) {
    return (elapsedMs % periodMs) / (float)periodMs;
}

static void setPwm(uint8_t pwmChannel, uint8_t duty) {
    pwmDuty[pwmChannel] = duty;
    Palatis::SoftPWM.set(pwmChannel, duty);
}

static void setAllLeds(Bank &bank, uint8_t duty) {
    bank.pwmTarget[0] = duty;
    bank.pwmTarget[1] = duty;
    bank.pwmTarget[2] = duty;
}

static void effectFull(Bank &bank) {
    if (!bank.staggerTurnOnActive) {
        setAllLeds(bank, 255);
        return;
    }

    const uint32_t elapsed = millis() - bank.effectStartMs;
    for (uint8_t i = 0; i < 3; i++) {
        const int32_t t = (int32_t)elapsed - (int32_t)i * STAGGERED_TURN_ON_LED_MS;
        float level = 0.0f;
        if (t >= (int32_t)STAGGERED_TURN_ON_LED_MS) {
            level = 1.0f;
        } else if (t > 0) {
            level = smoothstep(t / (float)STAGGERED_TURN_ON_LED_MS);
        }
        bank.pwmTarget[i] = (uint8_t)(level * 255.0f + 0.5f);
    }

    if (elapsed >= 3 * STAGGERED_TURN_ON_LED_MS) {
        bank.staggerTurnOnActive = false;
    }
}

static void effectFade(Bank &bank, uint32_t elapsedMs) {
    const float phase = cyclePhase(elapsedMs, FADE_PERIOD_MS);
    const float position = sinf(phase * 2.0f * (float)M_PI - (float)M_PI / 2.0f) + 1.0f;

    for (uint8_t i = 0; i < 3; i++) {
        const float dist = fabsf(position - (float)i) / FADE_SPREAD;
        float level = 0.0f;
        if (dist < 1.0f) {
            level = 0.5f * (1.0f + cosf(dist * (float)M_PI));
        }
        bank.pwmTarget[i] = (uint8_t)(level * level * 255.0f + 0.5f);
    }
}

static void effectBreath(Bank &bank, uint32_t elapsedMs) {
    const float phase = cyclePhase(elapsedMs, BREATH_PERIOD_MS);
    const float wave = (expf(sinf(phase * 2.0f * (float)M_PI)) - 0.36787944f) * 0.42545906412f;
    const uint8_t duty = BREATH_MIN + (uint8_t)(wave * (BREATH_MAX - BREATH_MIN) + 0.5f);
    setAllLeds(bank, duty);
}

static float gaussianPulse(float t, float center, float width) {
    const float x = (t - center) / width;
    return expf(-x * x);
}

static void effectHeartbeat(Bank &bank, uint32_t elapsedMs) {
    const float phase = cyclePhase(elapsedMs, HEARTBEAT_PERIOD_MS);
    float wave = gaussianPulse(phase, 0.14f, 0.05f) + 0.55f * gaussianPulse(phase, 0.32f, 0.045f);
    if (wave > 1.0f) {
        wave = 1.0f;
    }

    const uint8_t duty =
        HEARTBEAT_MIN + (uint8_t)(wave * (HEARTBEAT_MAX - HEARTBEAT_MIN) + 0.5f);
    setAllLeds(bank, duty);
}

static void applyEffect(Bank &bank) {
    const uint32_t elapsed = millis() - bank.effectStartMs;

    switch (bank.activeEffect) {
    case EFFECT_OFF:
        setAllLeds(bank, 0);
        break;
    case EFFECT_FULL:
        effectFull(bank);
        break;
    case EFFECT_HALF:
        setAllLeds(bank, HALF_BRIGHTNESS);
        break;
    case EFFECT_FADE:
        effectFade(bank, elapsed);
        break;
    case EFFECT_BREATH:
        effectBreath(bank, elapsed);
        break;
    case EFFECT_HEARTBEAT:
        effectHeartbeat(bank, elapsed);
        break;
    default:
        break;
    }
}

static bool usesDirectPwm(const Bank &bank) {
    return bank.staggerTurnOnActive || bank.activeEffect == EFFECT_FADE ||
           bank.activeEffect == EFFECT_BREATH || bank.activeEffect == EFFECT_HEARTBEAT;
}

static void nextEffect(Bank &bank) {
    const bool prevEffectIsOff = (bank.activeEffect == EFFECT_OFF);
    bank.activeEffect = (Effect)((bank.activeEffect + 1) % EFFECT_COUNT);
    bank.effectStartMs = millis();
    bank.staggerTurnOnActive = prevEffectIsOff;
    applyEffect(bank);
}

static void initButton(GButton &button) {
    button.setDebounce(80);
    button.setTimeout(400);
    button.setClickTimeout(200);
    button.setType(HIGH_PULL);
    button.setDirection(NORM_OPEN);
}

static void initBank(uint8_t index) {
    initButton(buttons[index]);

    banks[index].pwmBaseOffset = index * 3;
    banks[index].activeEffect = EFFECT_OFF;
    banks[index].effectStartMs = millis();
    banks[index].lastClickMs = 0;
    banks[index].staggerTurnOnActive = false;
    banks[index].pwmTarget[0] = 0;
    banks[index].pwmTarget[1] = 0;
    banks[index].pwmTarget[2] = 0;
    applyEffect(banks[index]);
}

void setup() {
    Palatis::SoftPWM.begin(PWM_FREQ_HZ);
    Palatis::SoftPWM.allOff();

    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);

    initBank(0);
    initBank(1);
}

void loop() {
    const uint32_t now = millis();

    for (uint8_t b = 0; b < 2; b++) {
        buttons[b].tick();
        if ((now - banks[b].lastClickMs) >= BUTTON_LOCKOUT_MS && buttons[b].isClick()) {
            banks[b].lastClickMs = now;
            nextEffect(banks[b]);
        }
    }

    for (uint8_t b = 0; b < 2; b++) {
        Bank &bank = banks[b];
        applyEffect(bank);
        if (usesDirectPwm(bank)) {
            setPwm(bank.pwmBaseOffset + 0, bank.pwmTarget[0]);
            setPwm(bank.pwmBaseOffset + 1, bank.pwmTarget[1]);
            setPwm(bank.pwmBaseOffset + 2, bank.pwmTarget[2]);
        } else {
            for (uint8_t i = 0; i < 3; i++) {
                const uint8_t pwmChannel = bank.pwmBaseOffset + i;
                const uint8_t currentDuty = pwmDuty[pwmChannel];
                const uint8_t targetDuty = bank.pwmTarget[i];

                if (currentDuty < targetDuty) {
                    const uint8_t next = currentDuty + PWM_SLEW_UP_STEP;
                    setPwm(pwmChannel, (next > targetDuty) ? targetDuty : next);
                } else if (currentDuty > targetDuty) {
                    const uint8_t delta = currentDuty - targetDuty;
                    // Bypass unsigned int undeflow
                    setPwm(pwmChannel, (delta < PWM_SLEW_DOWN_STEP) ? targetDuty : (uint8_t)(currentDuty - PWM_SLEW_DOWN_STEP));
                }
            }
        }
    }
}
