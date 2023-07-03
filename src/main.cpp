#include <Arduino.h>
#include "tinycontroller.h"
#include "blinky.h"

/**
 * Safety controller for BatterBox switching
 *
 * Wiring:
 * Input0: SystemSwitch (0 off, VBAT on)
 * Input1: DischargeEnable + PULLUP (0 on, VBAT off)
 * Input2: ChargeEnable + PULLUP (0 on, VBAT off)
 * Input3: ChargerSelector + PULLUP (0 External, VBAT MPPT)
 *
 * Output0: BMS ReadyPower+
 * Output1: DischargeContactor+
 * Output2: MPPT Charge Minitactor+
 * Output3: External Charge Minitactor+
 *
 */

#define PIN_IN_SYSTEM PIN_IN_0
#define PIN_IN_N_DISCHARGE_EN PIN_IN_1
#define PIN_IN_N_CHARGE_EN PIN_IN_2
#define PIN_IN_USE_MPPT PIN_IN_3

#define PIN_OUT_BMS_RPWR PIN_OUT_0
#define PIN_OUT_DISCHARGE_CONT PIN_OUT_1
#define PIN_OUT_MPPT_CONT PIN_OUT_2
#define PIN_OUT_EXT_CONT PIN_OUT_3

#define CONTACTOR_OPEN_DELAY 10


enum class CtrlState {
    NotSafeInit,
    SystemOff,
    SystemOn
};

enum class ChargeState {
    ChargeDisabled,
    ChargeEnabled,
};

enum class ChargerSelector {
    MpptCharger,
    ExternalCharger,
};

enum class DischargeState {
    DischargeDisabled,
    DischargeEnabled,
};

CtrlState ctrlState = CtrlState::NotSafeInit;
ChargeState chargeState = ChargeState::ChargeDisabled;
ChargerSelector chargerSelector = ChargerSelector::MpptCharger;
DischargeState dischargeState = DischargeState::DischargeDisabled;
ChargerSelector lastUsedCharger = ChargerSelector::MpptCharger;


void setup() {
    pinMode(PIN_IN_SYSTEM, INPUT);
    pinMode(PIN_IN_N_DISCHARGE_EN, INPUT);
    pinMode(PIN_IN_N_CHARGE_EN, INPUT);
    pinMode(PIN_IN_USE_MPPT, INPUT);
    pinMode(PIN_IN_BUTTON, INPUT);

    digitalWrite(PIN_OUT_BMS_RPWR, LOW);
    digitalWrite(PIN_OUT_DISCHARGE_CONT, LOW);
    digitalWrite(PIN_OUT_MPPT_CONT, LOW);
    digitalWrite(PIN_OUT_EXT_CONT, LOW);
    digitalWrite(PIN_OUT_LED, LOW);

    pinMode(PIN_OUT_BMS_RPWR, OUTPUT);
    pinMode(PIN_OUT_DISCHARGE_CONT, OUTPUT);
    pinMode(PIN_OUT_MPPT_CONT, OUTPUT);
    pinMode(PIN_OUT_EXT_CONT, OUTPUT);
    pinMode(PIN_OUT_LED, OUTPUT);

    delay(1000);
}



void loop() {
    // read inputs
    bool systemSwitch = digitalRead(PIN_IN_SYSTEM);
    bool nDischargeEnabled = digitalRead(PIN_IN_N_DISCHARGE_EN);
    bool nChargeEnabled = digitalRead(PIN_IN_N_CHARGE_EN);
    bool useInternalCharger = digitalRead(PIN_IN_USE_MPPT);

    // update state machine
    switch (ctrlState) {
        // go to safe state after system is turned off
        case CtrlState::NotSafeInit:
            if (systemSwitch == LOW) {
                ctrlState = CtrlState::SystemOff;
            }
            break;

            // wait for system to be turned on
        case CtrlState::SystemOff:
            if (systemSwitch == HIGH) {
                ctrlState = CtrlState::SystemOn;
            }
            break;

            // wait for system off
        case CtrlState::SystemOn:
            if (systemSwitch == LOW) {
                ctrlState = CtrlState::SystemOff;
            }
            break;
    }

    // process inputs after on
    if (ctrlState == CtrlState::SystemOn) {
        // check discharge enabled
        if (nDischargeEnabled == LOW) {
            dischargeState = DischargeState::DischargeEnabled;
        } else {
            dischargeState = DischargeState::DischargeDisabled;
        }

        // check charge state
        if (nChargeEnabled == LOW) {
            chargeState = ChargeState::ChargeEnabled;
        } else {
            chargeState = ChargeState::ChargeDisabled;
        }

        // check charger selector
        if (useInternalCharger == HIGH) {
            chargerSelector = ChargerSelector::MpptCharger;
        } else {
            chargerSelector = ChargerSelector::ExternalCharger;
        }

    } else {
        chargeState = ChargeState::ChargeDisabled;
        chargerSelector = ChargerSelector::MpptCharger;
        dischargeState = DischargeState::DischargeDisabled;
    }

    // ------------------------------------------------------------------------
    if (ctrlState == CtrlState::SystemOn) {
        digitalWrite(PIN_OUT_BMS_RPWR, HIGH);
    } else {
        digitalWrite(PIN_OUT_BMS_RPWR, LOW);
    }

    if (dischargeState == DischargeState::DischargeEnabled) {
        digitalWrite(PIN_OUT_DISCHARGE_CONT, HIGH);
    } else {
        digitalWrite(PIN_OUT_DISCHARGE_CONT, LOW);
    }

    if (chargeState == ChargeState::ChargeEnabled) {
        if (chargerSelector == ChargerSelector::MpptCharger) {
            digitalWrite(PIN_OUT_EXT_CONT, LOW);
            if (lastUsedCharger == ChargerSelector::ExternalCharger) {
                delay(CONTACTOR_OPEN_DELAY); // wait for contactor to open
            }
            digitalWrite(PIN_OUT_MPPT_CONT, HIGH);
        } else {
            digitalWrite(PIN_OUT_MPPT_CONT, LOW);
            if (lastUsedCharger == ChargerSelector::MpptCharger) {
                delay(CONTACTOR_OPEN_DELAY); // wait for contactor to open
            }
            digitalWrite(PIN_OUT_EXT_CONT, HIGH);
        }
        lastUsedCharger = chargerSelector;
    } else {
        digitalWrite(PIN_OUT_MPPT_CONT, LOW);
        digitalWrite(PIN_OUT_EXT_CONT, LOW);
    }

    uint64_t time = millis();
    if(ctrlState == CtrlState::NotSafeInit) {
        if(lastLedSwitch + LED_BLINK_NOTSAFE < time){
            lastLedSwitch = time;
            digitalWrite(PIN_OUT_LED, CHANGE);
        }
    }else {
        if(lastLedSwitch + LED_BLINK_OK < time) {
            lastLedSwitch = time;
            digitalWrite(PIN_OUT_LED, CHANGE);
        }
    }

}