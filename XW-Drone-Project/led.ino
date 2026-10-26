// Control del LED integrado en la placa
// Board's LED control

#include "board_config.h"

#if BOARD_LED_ENABLED

#define BLINK_PERIOD      500000  // Parpadeo lento: medio período 500 ms → 1 Hz
#define BLINK_FAST_PERIOD  62500  // Parpadeo rápido: medio período 62.5 ms → 8 Hz

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // para ESP32 Dev Module
#endif

// ---- Declaraciones de dependencias externas ----
extern bool armed;
extern float t;
extern float controlTime;
extern float rcLossTimeout;
extern float thrustTarget;    // control.ino
extern float batteryVoltage;  // battery.ino
extern bool isInverted; // safety.ino
#if WEB_RC_ENABLED
extern bool webRCEnabled;
extern bool useWebRC;
bool isUsingWebRC();
#endif

void setupLED() {
	pinMode(LED_BUILTIN, OUTPUT);
}

void setLED(bool on) {
	static bool state = false;
	if (on == state) {
		return; // no llamar a digitalWrite si el estado no cambia
	}
	digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
	state = on;
}

void blinkLED() {
	setLED(micros() / BLINK_PERIOD % 2);
}

// Aviso de batería: elige el umbral según el estado de vuelo
// En vuelo (thrustTarget >= 0.15) → L2 (2.8V), L1 no aplica en vuelo
// Sin armar / armado en ralentí → L1 (3.4V)
bool batteryAlertActive() {
	if (batteryVoltage <= VBAT_ABSENT_THRESHOLD) return false;
	bool flying = armed && thrustTarget >= 0.15f;
	if (flying) return batteryVoltage < VBAT_LOW_THRESHOLD;   // L2: en vuelo
	else        return batteryVoltage < VBAT_WARN_THRESHOLD;   // L1: sin armar/ralentí
}

// Detectar cualquier aviso activo (batería baja / pérdida de RC / invertido)
bool ledAlertActive() {
	// Detección de inversión
	if (isInverted) return true;

	// Detección de pérdida de RC SBUS (solo tras armar)
	if (controlTime != 0 && armed && (t - controlTime > rcLossTimeout)) return true;

#if WEB_RC_ENABLED
	// Detección de pérdida del Web RC: activo pero con timeout
	if (webRCEnabled && useWebRC && !isUsingWebRC()) return true;
#endif

	if (batteryAlertActive()) return true;

	return false;
}

// Llamado desde el bucle principal: gestiona el LED según el estado de vuelo
void updateLED() {
	if (!armed) {
		if (batteryAlertActive()) {
			setLED(micros() / BLINK_FAST_PERIOD % 2); // Batería baja antes de armar: parpadeo rápido
		} else {
			setLED(false); // En espera normal: apagado fijo
		}
		return;
	}
	if (ledAlertActive()) {
		setLED(micros() / BLINK_FAST_PERIOD % 2); // Cualquier aviso: parpadeo rápido 8 Hz
	} else {
		setLED(micros() / BLINK_PERIOD % 2); // Vuelo normal: parpadeo lento 1 Hz
	}
}

#else // BOARD_LED_ENABLED == 0 (ESP32-C3 sin LED integrado)

void setupLED() {}
void setLED(bool on) {}
void updateLED() {}

#endif // BOARD_LED_ENABLED