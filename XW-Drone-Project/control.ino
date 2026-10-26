// Control de vuelo

#include "vector.h"
#include "quaternion.h"
#include "pid.h"
#include "lpf.h"
#include "util.h"

#if WEB_RC_ENABLED
#include "control.h"
#endif

// ============== Parámetros del lazo de velocidad angular (lazo interno) ==============
#define PITCHRATE_P 0.05     // Aumentar P mejora la velocidad de respuesta
#define PITCHRATE_I 0.2      // I medio para compensar diferencias entre motores
#define PITCHRATE_D 0.001    // D pequeño para suprimir oscilaciones
#define PITCHRATE_I_LIM 0.35 // Limitar la acumulación del integrador
#define ROLLRATE_P 0.06      // Alabeo y cabeceo usan los mismos parámetros
#define ROLLRATE_I PITCHRATE_I 
#define ROLLRATE_D 0.002 
#define ROLLRATE_I_LIM PITCHRATE_I_LIM
#define YAWRATE_P 0.3        // La guiñada necesita P más alto (menor inercia)
#define YAWRATE_I 0.01       // I medio para compensar
#define YAWRATE_D 0.01       // D pequeño
#define YAWRATE_I_LIM 0.3
// ============== Parámetros del lazo de ángulo (lazo externo) ==============
#define ROLL_P 7    // P alto para respuesta rápida
#define ROLL_I 0    // El lazo de ángulo normalmente no necesita I
#define ROLL_D 0    // El lazo de ángulo normalmente no necesita D
#define PITCH_P ROLL_P // Alabeo y cabeceo iguales
#define PITCH_I ROLL_I
#define PITCH_D ROLL_D
#define YAW_P 3     // La guiñada responde algo más lento
// ============== Límites ==============
#define PITCHRATE_MAX radians(360) // Límite de velocidad alta (1000°/s)
#define ROLLRATE_MAX radians(360)
#define YAWRATE_MAX radians(300)   // Velocidad de guiñada algo menor
#define TILT_MAX radians(30)       // Ángulo de inclinación máximo 30°
#define ALTHOLD_HOVER_THRUST 0.5f  // Empuje base de hover en modo altura (stub, se reemplazará con el barómetro)
#define ARM_THROTTLE_LIMIT   0.05f // Límite superior de gas para armar (normalizado 0~1), 5%, superarlo prohíbe armar
#define RATES_D_LPF_ALPHA 0.2      // Frecuencia de corte ~40 Hz

float motThrMin = 0.10f;  // Límite inferior de empuje (salida con stick al mínimo), ajustable con el parámetro MOT_THR_MIN
float motThrMax = 1.0f;   // Límite superior de empuje (salida con stick al máximo), ajustable con el parámetro MOT_THR_MAX

const int RAW = 0, ACRO = 1, STAB = 2, ALTHOLD = 3, AUTO = 4; // modos de vuelo
int mode = STAB;
bool armed = false;
int flightModes[] = {STAB, STAB, STAB}; // Modos de vuelo correspondientes a las 3 posiciones del interruptor RC, configurables con CTL_FLT_MODE_0/1/2

#if WEB_RC_ENABLED
extern uint16_t webRCButtons;
#endif

PID rollRatePID(ROLLRATE_P, ROLLRATE_I, ROLLRATE_D, ROLLRATE_I_LIM, RATES_D_LPF_ALPHA);
PID pitchRatePID(PITCHRATE_P, PITCHRATE_I, PITCHRATE_D, PITCHRATE_I_LIM, RATES_D_LPF_ALPHA);
PID yawRatePID(YAWRATE_P, YAWRATE_I, YAWRATE_D);
PID rollPID(ROLL_P, ROLL_I, ROLL_D);
PID pitchPID(PITCH_P, PITCH_I, PITCH_D);
PID yawPID(YAW_P, 0, 0);
Vector maxRate(ROLLRATE_MAX, PITCHRATE_MAX, YAWRATE_MAX);
float tiltMax = TILT_MAX;

Quaternion attitudeTarget;
Vector ratesTarget;
Vector ratesExtra; // velocidades de feedforward
Vector torqueTarget;
float thrustTarget;

extern const int MOTOR_REAR_LEFT, MOTOR_REAR_RIGHT, MOTOR_FRONT_RIGHT, MOTOR_FRONT_LEFT;
extern float motors[4];
extern float controlRoll, controlPitch, controlThrottle, controlYaw, controlMode;
extern float batteryVoltage;  // battery.ino

void control() {
	interpretControls();
#if WEB_RC_ENABLED
	interpretWebRC();
#endif
	failsafe();
	controlAttitude();
	controlRates();
	controlTorque();
}

void interpretControls() {
	if (controlMode < 0.25) mode = flightModes[0];
	else if (controlMode <= 0.75) mode = flightModes[1];
	else if (controlMode > 0.75) mode = flightModes[2];

	if (mode == AUTO) return; // el piloto no tiene efecto en modo AUTO

#if WEB_RC_ENABLED
	if (!isUsingWebRC()) { // El gesto de armado por SBUS solo se aplica cuando Web RC no está activo
#endif
	static bool armWarnNotified = false;  // Anti-spam: aviso de armar bloqueado por batería baja
	if (controlThrottle < 0.05 && controlYaw > 0.95) { // gesto de armado
		if (batteryVoltage > VBAT_ABSENT_THRESHOLD && batteryVoltage < VBAT_WARN_THRESHOLD) {
			// L1 o inferior: armar prohibido, avisar cuando cambie el estado
			if (!armWarnNotified) {
				print("Batería baja (%.2fV), armado bloqueado\n", batteryVoltage);
#if WEB_RC_ENABLED
				char warnBuf[64];
				snprintf(warnBuf, sizeof(warnBuf), "Batería baja (%.2fV) armado bloqueado", batteryVoltage);
				setWebRCWarn(warnBuf);
#endif
				armWarnNotified = true;
			}
		} else {
			armed = true;
			armWarnNotified = false; // Reiniciar al cambiar de batería
		}
	}
	if (controlThrottle < 0.05 && controlYaw < -0.95) armed = false; // gesto de desarmado
#if WEB_RC_ENABLED
	}
#endif

	if (abs(controlYaw) < 0.1) controlYaw = 0; // zona muerta de guiñada

	if (controlThrottle < 0.05f) {
		thrustTarget = 0.0f;   // Zona muerta inferior → ralentí, PID no se ejecuta
	} else {
		thrustTarget = mapf(controlThrottle, 0.05f, 1.0f, motThrMin, motThrMax);
	}

	if (mode == STAB) {
		float yawTarget = attitudeTarget.getYaw();
		if (!armed || invalid(yawTarget) || controlYaw != 0) yawTarget = attitude.getYaw(); // reiniciar objetivo de guiñada
		attitudeTarget = Quaternion::fromEuler(Vector(controlRoll * tiltMax, controlPitch * tiltMax, yawTarget));
		ratesExtra = Vector(0, 0, -controlYaw * maxRate.z); // stick de guiñada positivo = rotación horaria en FLU
	}

	if (mode == ACRO) {
		attitudeTarget.invalidate(); // omitir control de actitud
		ratesTarget.x = controlRoll * maxRate.x;
		ratesTarget.y = controlPitch * maxRate.y;
		ratesTarget.z = -controlYaw * maxRate.z; // stick de guiñada positivo = rotación horaria en FLU
	}

	if (mode == RAW) { // control directo de torque
		attitudeTarget.invalidate(); // omitir control de actitud
		ratesTarget.invalidate(); // omitir control de velocidad angular
		torqueTarget = Vector(controlRoll, controlPitch, -controlYaw) * 0.1;
	}
}

void controlAttitude() {
	if (!armed || attitudeTarget.invalid() || thrustTarget < motThrMin) return; // omitir control de actitud

	const Vector up(0, 0, 1);
	Vector upActual = Quaternion::rotateVector(up, attitude);
	Vector upTarget = Quaternion::rotateVector(up, attitudeTarget);

	Vector error = Vector::rotationVectorBetween(upTarget, upActual);

	ratesTarget.x = rollPID.update(error.x) + ratesExtra.x;
	ratesTarget.y = pitchPID.update(error.y) + ratesExtra.y;

	float yawError = wrapAngle(attitudeTarget.getYaw() - attitude.getYaw());
	ratesTarget.z = yawPID.update(yawError) + ratesExtra.z;
}


void controlRates() {
	if (!armed || ratesTarget.invalid() || thrustTarget < motThrMin) return; // omitir control de velocidad angular

	Vector error = ratesTarget - rates;

	// Calcular el torque deseado, donde 0 = sin torque, 1 = torque máximo posible
	torqueTarget.x = rollRatePID.update(error.x);
	torqueTarget.y = pitchRatePID.update(error.y);
	torqueTarget.z = yawRatePID.update(error.z);
}

void controlTorque() {
	if (!torqueTarget.valid()) return; // omitir control de torque

	if (!armed) {
		memset(motors, 0, sizeof(motors)); // detener motores si está desarmado
		return;
	}

	if (thrustTarget < motThrMin) {
		for (int i = 0; i < 4; i++) motors[i] = motThrMin; // empuje de ralentí
		return;
	}

	motors[MOTOR_FRONT_LEFT] = thrustTarget + torqueTarget.x - torqueTarget.y + torqueTarget.z;
	motors[MOTOR_FRONT_RIGHT] = thrustTarget - torqueTarget.x - torqueTarget.y - torqueTarget.z;
	motors[MOTOR_REAR_LEFT] = thrustTarget + torqueTarget.x + torqueTarget.y - torqueTarget.z;
	motors[MOTOR_REAR_RIGHT] = thrustTarget - torqueTarget.x + torqueTarget.y + torqueTarget.z;

	motors[0] = constrain(motors[0], 0, 1);
	motors[1] = constrain(motors[1], 0, 1);
	motors[2] = constrain(motors[2], 0, 1);
	motors[3] = constrain(motors[3], 0, 1);
}

const char* getModeName() {
	switch (mode) {
		case RAW:     return "DIRECTO";
		case ACRO:    return "ACRO";
		case STAB:    return "AUTOESTABLE";
		case ALTHOLD: return "ALTURA";
		case AUTO:    return "AUTO";
		default:      return "DESCONOCIDO";
	}
}

#if WEB_RC_ENABLED
// Manejo de botones del control web (solo armado/desarmado y cambio de modo)
void interpretWebRC() {
	if (!isUsingWebRC()) return;

	// Manejar botones de armar/desarmar (detección de flanco ascendente, evita repetición cada ciclo)
	static uint16_t lastWebRCButtons = 0;
	uint16_t risingEdge = webRCButtons & ~lastWebRCButtons;
	lastWebRCButtons = webRCButtons;

	// Registrar cambios de estado armado/desarmado
	static bool lastArmedState = false;
	if (armed != lastArmedState) {
		print(armed ? "Web RC: Armado\n" : "Web RC: Desarmado\n");
		lastArmedState = armed;
	}

	// Botón 0: armar (flanco ascendente)
	if (risingEdge & 0x0001) {
		if (controlThrottle < ARM_THROTTLE_LIMIT) {
			armed = true;
		} else {
			setWebRCWarn("Gas muy alto, no se puede armar");
		}
	}

	// Botón 1: desarmar (flanco ascendente)
	if (risingEdge & 0x0002) {
		armed = false;
	}

	// Botón 2: parada de emergencia (flanco ascendente)
	if (risingEdge & 0x0004) {
		armed = false;
		thrustTarget = 0.0f;
	}

	// Botón 6: modo AUTOESTABLE (flanco ascendente)
	if (risingEdge & 0x0040) {
		mode = STAB;
	}

	// Botón 7: modo ACRO (flanco ascendente)
	if (risingEdge & 0x0080) {
		mode = ACRO;
	}

	// Botón 8: modo ALTURA (flanco ascendente) - no implementado, saltar a AUTOESTABLE para evitar que el frontend se bloquee
	if (risingEdge & 0x0100) {
		mode = STAB;
		setWebRCWarn("Modo altura no soportado, cambiado a autoestable");
	}

	// Registro de cambio de modo
	static int lastMode = STAB;
	if (mode != lastMode) {
		print("Web RC: modo cambiado a %s\n", getModeName());
		lastMode = mode;
	}
}
#endif