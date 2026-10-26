// Trabajar con el receptor RC
// Work with the RC receiver

#include <SBUS.h>
#include "util.h"
#include "board_config.h"

SBUS rc(BOARD_RC_SERIAL); // Serial2 en ESP32, Serial1 en ESP32-C3
int rcRxPin = BOARD_RC_RX_PIN; // Pin RX de SBUS, -1 = RC deshabilitado, configurable con RC_RX_PIN
uint16_t channels[16]; // canales RC crudos
float controlTime; // momento de la última actualización de controles
float channelZero[16]; // valores cero de calibración
float channelMax[16]; // valores máximos de calibración

// Mapeo de canales (float para almacenar en parámetros):
float rollChannel = NAN, pitchChannel = NAN, throttleChannel = NAN, yawChannel = NAN, modeChannel = NAN;

void setupRC() {
	if (rcRxPin < 0) return; // Pin no configurado, omitir inicialización de RC
	print("Configurando RC\n");
	rc.begin(rcRxPin);
}

bool readRC() {
	if (rcRxPin < 0) return false; // RC deshabilitado
	if (rc.read()) {
		SBUSData data = rc.data();
		for (int i = 0; i < 16; i++) channels[i] = data.ch[i]; // copiar los canales
		normalizeRC();
		controlTime = t;
		return true;
	}
	return false;
}

void normalizeRC() {
	float controls[16];
	for (int i = 0; i < 16; i++) {
		controls[i] = mapf(channels[i], channelZero[i], channelMax[i], 0, 1);
	}
	// Actualizar los valores de control
	controlRoll = rollChannel >= 0 ? controls[(int)rollChannel] : NAN;
	controlPitch = pitchChannel >= 0 ? controls[(int)pitchChannel] : NAN;
	controlYaw = yawChannel >= 0 ? controls[(int)yawChannel] : NAN;
	controlThrottle = throttleChannel >= 0 ? controls[(int)throttleChannel] : NAN;
	controlMode = modeChannel >= 0 ? controls[(int)modeChannel] : NAN;
}

void calibrateRC() {
	uint16_t zero[16];
	uint16_t center[16];
	uint16_t max[16];
	print("1/8 Calibrando RC, centrar todos los sticks [3 segundos]\n");
	pause(8);
	calibrateRCChannel(NULL, zero, zero, "2/8 Stick izquierdo: abajo, stick derecho: centrado [3 segundos]\n...     ...\n...     .o.\n.o.     ...\n");
	calibrateRCChannel(NULL, center, center, "3/8 Stick izquierdo: centrado, stick derecho: centrado [3 segundos]\n...     ...\n.o.     .o.\n...     ...\n");
	calibrateRCChannel(&throttleChannel, zero, max, "4/8 Identificación del canal de gas, stick izquierdo: hasta arriba (gas máximo), stick derecho: centrado [3 segundos]\n.o.     ...\n...     .o.\n...     ...\n");
	calibrateRCChannel(&yawChannel, center, max, "5/8 Identificación del canal de guiñada, stick izquierdo: hasta la derecha (guiñada horaria), stick derecho: centrado [3 segundos]\n...     ...\n..o     .o.\n...     ...\n");
	calibrateRCChannel(&pitchChannel, zero, max, "6/8 Identificación del canal de cabeceo, stick izquierdo: hasta abajo, stick derecho: hasta arriba (cabeceo adelante) [3 segundos]\n...     .o.\n...     ...\n.o.     ...\n");
	calibrateRCChannel(&rollChannel, zero, max, "7/8 Identificación del canal de alabeo, stick izquierdo: hasta abajo, stick derecho: hasta la derecha (alabeo derecha) [3 segundos]\n...     ...\n...     ..o\n.o.     ...\n");
	calibrateRCChannel(&modeChannel, zero, max, "8/8 Identificación del canal de modo, primero mover el interruptor de armado a la posición bloqueada, luego mover el interruptor de modo a la posición más alta (p. ej. modo manual) [3 segundos]\n");
	printRCCalibration();
}

// Paso 1: posición inicial, acción:
// ✓ Centrar todos los sticks
// ✓ Todos los interruptores en posición por defecto (normalmente bloqueado/central)
// ✓ Mantener 3 segundos sin mover
// Paso 2: movimiento básico del stick, acción:
// ✓ Stick izquierdo: abajo
// ✓ Stick derecho: centrado
// ✓ Mantener 3 segundos
// Paso 3: sticks centrados, acción:
// ✓ Stick izquierdo: centrado
// ✓ Stick derecho: centrado
// ✓ Mantener 3 segundos
// Paso 4: identificación del canal de gas, acción:
// ✓ Stick izquierdo: hasta arriba (gas máximo)
// ✓ Stick derecho: centrado
// ✓ Mantener 3 segundos
// → El sistema identifica automáticamente el canal de gas
// Paso 5: identificación del canal de guiñada, acción:
// ✓ Stick izquierdo: hasta la derecha (guiñada horaria)
// ✓ Stick derecho: centrado
// ✓ Mantener 3 segundos
// → El sistema identifica automáticamente el canal de guiñada
// Paso 6: identificación del canal de cabeceo, acción:
// ✓ Stick izquierdo: hasta abajo
// ✓ Stick derecho: hasta arriba (cabeceo adelante)
// ✓ Mantener 3 segundos
// → El sistema identifica automáticamente el canal de cabeceo
// Paso 7: identificación del canal de alabeo, acción:
// ✓ Stick izquierdo: hasta abajo
// ✓ Stick derecho: hasta la derecha (alabeo derecha)
// ✓ Mantener 3 segundos
// → El sistema identifica automáticamente el canal de alabeo
// Paso 8: identificación del canal de modo, acción:
// ✓ Primero mover el interruptor de armado a la posición bloqueada
// ✓ Luego mover el interruptor de modo a la posición más alta (p. ej. modo manual)
// ✓ Mantener 3 segundos
// → El sistema identifica automáticamente el canal de modo

void calibrateRCChannel(float *channel, uint16_t in[16], uint16_t out[16], const char *str) {
	print("%s", str);
	pause(8);
	for (int i = 0; i < 30; i++) readRC(); // intentar actualizar hasta 30 veces
	memcpy(out, channels, sizeof(channels));

	if (channel == NULL) return; // no hay canal que calibrar

	// Encontrar el canal que más cambió entre in y out
	int ch = -1, diff = 0;
	for (int i = 0; i < 16; i++) {
		if (abs(out[i] - in[i]) > diff) {
			ch = i;
			diff = abs(out[i] - in[i]);
		}
	}
	if (ch >= 0 && diff > 10) { // umbral de diferencia 10
		*channel = ch;
		channelZero[ch] = in[ch];
		channelMax[ch] = out[ch];
	} else {
		*channel = NAN;
	}
}

void printRCCalibration() {
	print("Control   Ch     Cero   Máx\n");
	print("Alabeo    %-7g%-7g%-7g\n", rollChannel, rollChannel >= 0 ? channelZero[(int)rollChannel] : NAN, rollChannel >= 0 ? channelMax[(int)rollChannel] : NAN);
	print("Cabeceo   %-7g%-7g%-7g\n", pitchChannel, pitchChannel >= 0 ? channelZero[(int)pitchChannel] : NAN, pitchChannel >= 0 ? channelMax[(int)pitchChannel] : NAN);
	print("Guiñada   %-7g%-7g%-7g\n", yawChannel, yawChannel >= 0 ? channelZero[(int)yawChannel] : NAN, yawChannel >= 0 ? channelMax[(int)yawChannel] : NAN);
	print("Gas       %-7g%-7g%-7g\n", throttleChannel, throttleChannel >= 0 ? channelZero[(int)throttleChannel] : NAN, throttleChannel >= 0 ? channelMax[(int)throttleChannel] : NAN);
	print("Modo      %-7g%-7g%-7g\n", modeChannel, modeChannel >= 0 ? channelZero[(int)modeChannel] : NAN, modeChannel >= 0 ? channelMax[(int)modeChannel] : NAN);
}