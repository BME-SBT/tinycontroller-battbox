#include <Arduino.h>
#include "tinycontroller.h"

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

enum class CtrlState
{
    NotSafeInit,
    SystemOff,
    SystemOn
};

enum class ChargeState
{
    ChargeDisabled,
    ChargeEnabled,
};

enum class ChargerSelector
{
    MpptCharger,
    ExternalCharger,
};

enum class DischargeState
{
    DischargeDisabled,
    DischargeEnabled,
};

CtrlState ctrlState = CtrlState::NotSafeInit;
ChargeState chargeState = ChargeState::ChargeDisabled;
ChargerSelector chargerSelector = ChargerSelector::MpptCharger;
DischargeState dischargeState = DischargeState::DischargeDisabled;
ChargerSelector lastUsedCharger = ChargerSelector::MpptCharger;

void setup()
{
    pinMode(PIN_IN_0, INPUT);
    pinMode(PIN_IN_1, INPUT);
    pinMode(PIN_IN_2, INPUT);
    pinMode(PIN_IN_3, INPUT);
    pinMode(PIN_IN_BUTTON, INPUT);

    digitalWrite(PIN_OUT_0, LOW);
    digitalWrite(PIN_OUT_1, LOW);
    digitalWrite(PIN_OUT_2, LOW);
    digitalWrite(PIN_OUT_3, LOW);
    digitalWrite(PIN_OUT_LED, LOW);

    pinMode(PIN_OUT_0, OUTPUT);
    pinMode(PIN_OUT_1, OUTPUT);
    pinMode(PIN_OUT_2, OUTPUT);
    pinMode(PIN_OUT_3, OUTPUT);
    pinMode(PIN_OUT_LED, OUTPUT);
}

void loop()
{
    // read inputs
    bool systemSwitch = digitalRead(PIN_IN_0);
    bool nDischargeEnabled = digitalRead(PIN_IN_1);
    bool nChargeEnabled = digitalRead(PIN_IN_2);
    bool useInternalCharger = digitalRead(PIN_IN_3);

    // update state machine
    switch (ctrlState)
    {
    // go to safe state after system is turned off
    case CtrlState::NotSafeInit:
        if (systemSwitch == LOW)
        {
            ctrlState = CtrlState::SystemOff;
        }
        break;

    // wait for system to be turned on
    case CtrlState::SystemOff:
        if (systemSwitch == HIGH)
        {
            ctrlState = CtrlState::SystemOn;
        }
        break;

    // wait for system off
    case CtrlState::SystemOn:
        if (systemSwitch == LOW)
        {
            ctrlState = CtrlState::SystemOff;
        }
        break;
    }

    // process inputs after on
    if (ctrlState == CtrlState::SystemOn)
    {
        // check discharge enabled
        if (nDischargeEnabled == LOW)
        {
            dischargeState = DischargeState::DischargeEnabled;
        }
        else
        {
            dischargeState = DischargeState::DischargeDisabled;
        }

        // check charge state
        if (nChargeEnabled == LOW)
        {
            chargeState = ChargeState::ChargeEnabled;
        }
        else
        {
            chargeState = ChargeState::ChargeDisabled;
        }

        // check charger selector
        if (useInternalCharger == HIGH)
        {
            chargerSelector = ChargerSelector::MpptCharger;
        }
        else
        {
            chargerSelector = ChargerSelector::ExternalCharger;
        }
    }
    else
    {
        chargeState = ChargeState::ChargeDisabled;
        chargerSelector = ChargerSelector::MpptCharger;
        dischargeState = DischargeState::DischargeDisabled;
    }

    // ------------------------------------------------------------------------
    if (ctrlState == CtrlState::SystemOn)
    {
        digitalWrite(PIN_OUT_0, HIGH);
    }
    else
    {
        digitalWrite(PIN_OUT_0, LOW);
    }

    if (dischargeState == DischargeState::DischargeEnabled)
    {
        digitalWrite(PIN_OUT_1, HIGH);
    }
    else
    {
        digitalWrite(PIN_OUT_1, LOW);
    }

    if (chargeState == ChargeState::ChargeEnabled)
    {
        if (chargerSelector == ChargerSelector::MpptCharger)
        {
            digitalWrite(PIN_OUT_3, LOW);
            if (lastUsedCharger == ChargerSelector::ExternalCharger)
            {
                delay(5); // wait for contactor to open
            }
            digitalWrite(PIN_OUT_2, HIGH);
        }
        else
        {
            digitalWrite(PIN_OUT_2, LOW);
            if (lastUsedCharger == ChargerSelector::MpptCharger)
            {
                delay(5); // wait for contactor to open
            }
            digitalWrite(PIN_OUT_3, HIGH);
        }
        lastUsedCharger = chargerSelector;
    }
    else
    {
        digitalWrite(PIN_OUT_2, LOW);
        digitalWrite(PIN_OUT_3, LOW);
    }
}