// Control de salida de motores mediante MOSFET
// Si se usan ESCs, cambiar pwmStop, pwmMin y pwmMax a los valores apropiados en μs, reducir pwmFrequency a 400
// Motors output control using MOSFETs
// In case of using ESCs, change pwmStop, pwmMin and pwmMax to appropriate values in μs, decrease pwmFrequency (to 400)

#include "util.h"
#include "board_config.h"

float motors[4]; // empujes normalizados de los motores en rango [0..1]

// Pines de motor (corresponden a MOTOR_REAR_LEFT=0, MOTOR_REAR_RIGHT=1, MOTOR_FRONT_RIGHT=2, MOTOR_FRONT_LEFT=3)
int motorPins[4] = BOARD_MOTOR_PINS; // RL, RR, FR, FL

int pwmFrequency = 78000;
int pwmResolution = 10;
int pwmStop = 0;
int pwmMin = 0;
int pwmMax = -1; // -1 = modo ciclo de trabajo puro (MOSFET directo); con ESC, poner el valor PWM máximo real (μs)

// Índices del array de motores:
const int MOTOR_REAR_LEFT = 0;
const int MOTOR_REAR_RIGHT = 1;
const int MOTOR_FRONT_RIGHT = 2;
const int MOTOR_FRONT_LEFT = 3;

void setupMotors() {
	print("Configurando motores\n");

	// configurar pines
	for (int i = 0; i < 4; i++) {
		ledcAttach(motorPins[i], pwmFrequency, pwmResolution);
		pwmFrequency = ledcChangeFrequency(motorPins[i], pwmFrequency, pwmResolution); // al reconfigurar
	}

	sendMotors();
	print("Motores inicializados\n");
}

int getDutyCycle(float value) {
	value = constrain(value, 0, 1);
	if (pwmMax >= 0) { // modo tiempo PWM (con ESC)
		float pwm = mapf(value, 0, 1, pwmMin, pwmMax);
		if (value == 0) pwm = pwmStop;
		float duty = mapf(pwm, 0, 1000000 / pwmFrequency, 0, (1 << pwmResolution) - 1);
		return round(duty);
	} else { // modo ciclo de trabajo puro (MOSFET directo)
		return round(value * ((1 << pwmResolution) - 1));
	}
}

void sendMotors() {
	for (int i = 0; i < 4; i++) {
		ledcWrite(motorPins[i], getDutyCycle(motors[i]));
	}
}

bool motorsActive() {
	return motors[0] != 0 || motors[1] != 0 || motors[2] != 0 || motors[3] != 0;
}

void testMotor(int n) {
	print("Probando motor %d\n", n);
	motors[n] = 1;
	delay(50); // ESP32 puede necesitar esperar al final del ciclo actual para cambiar el duty https://github.com/espressif/arduino-esp32/issues/5306
	sendMotors();
	pause(3);
	motors[n] = 0;
	sendMotors();
	print("Hecho\n");
}