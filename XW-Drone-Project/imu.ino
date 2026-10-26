// Configuración del giroscopio y acelerómetro
// Work with the IMU sensor

#include <SPI.h>
#include <FlixPeriph.h>
#include "vector.h"
#include "lpf.h"
#include "util.h"
#include "board_config.h"

MPU9250 imu(SPI, BOARD_SPI_CS);

// Orientación de montaje del IMU (ángulos de Euler, rad). Valor por defecto (0, 0, -PI/2) corresponde al montaje de esta PCB:
// Chip con la cara hacia arriba, serigrafía X→lado derecho del dron, serigrafía Y→parte frontal → fórmula de conversión Vector(data.y, -data.x, data.z)
// Para otras orientaciones, modificar este valor y ejecutar `preset` para reiniciar el almacenamiento de parámetros.
Vector imuRotation(0, 0, -PI / 2);

Vector accBias;
Vector accScale(1, 1, 1);
Vector gyroBias;
LowPassFilter<Vector> gyroBiasFilter(0.001); // Filtro paso bajo para estimación del sesgo del giroscopio

void setupIMU() {
	print("Configurando IMU\n");
	SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, BOARD_SPI_CS);
	imu.begin();
	configureIMU();
}

void configureIMU() {
	imu.setAccelRange(imu.ACCEL_RANGE_4G);
	imu.setGyroRange(imu.GYRO_RANGE_2000DPS);
	imu.setDLPF(imu.DLPF_MAX);
	imu.setRate(imu.RATE_1KHZ_APPROX);
	imu.setupInterrupt();
}

void readIMU() {
	imu.waitForData();
	imu.getGyro(gyro.x, gyro.y, gyro.z);
	imu.getAccel(acc.x, acc.y, acc.z);
	calibrateGyroOnce();
	// aplicar escala y sesgo
	acc = (acc - accBias) / accScale;
	gyro = gyro - gyroBias;
	// rotar al marco del cuerpo usando los ángulos de Euler de imuRotation
	Quaternion rotation = Quaternion::fromEuler(imuRotation);
	acc  = Quaternion::rotateVector(acc,  rotation.inversed());
	gyro = Quaternion::rotateVector(gyro, rotation.inversed());
}

void calibrateGyroOnce() {
	static Delay landedDelay(2);
	if (!landedDelay.update(landed)) return; // calibrar solo si está definitivamente parado

	gyroBias = gyroBiasFilter.update(gyro);
}

void calibrateAccel() {
	print("Calibrando giroscopio y acelerómetro\n");
	imu.setAccelRange(imu.ACCEL_RANGE_2G); // modo más sensible

	print("1/6 Colocación horizontal: morro hacia delante (dirección normal de vuelo), parte inferior hacia abajo sobre superficie plana, totalmente horizontal sin inclinación. Mantener quieto, la calibración comienza en 8 segundos;\n");
	pause(8);
	calibrateAccelOnce();
	print("Calibración horizontal completada. Continuar.\n");
	pause(1);
	print("2/6 Morro hacia arriba: mantener el morro hacia delante, apuntando al cielo, cola hacia abajo tocando la superficie, perpendicular al suelo. Mantener quieto, la calibración comienza en 8 segundos;\n");
	pause(8);
	calibrateAccelOnce();
	print("Calibración morro arriba completada. Continuar.\n");
	pause(1);
	print("3/6 Morro hacia abajo: mantener el morro hacia delante, apuntando al suelo y tocando la superficie, cola hacia arriba, perpendicular al suelo. Mantener quieto, la calibración comienza en 8 segundos;\n");
	pause(8);
	calibrateAccelOnce();
	print("Calibración morro abajo completada. Continuar.\n");
	pause(1);
	print("4/6 Lado derecho hacia abajo: mantener el morro hacia delante, lado derecho del dron hacia abajo tocando la superficie, brazo izquierdo hacia arriba, perpendicular al suelo. Mantener quieto, la calibración comienza en 8 segundos;\n");
	pause(8);
	calibrateAccelOnce();
	print("Calibración lado derecho abajo completada. Continuar.\n");
	pause(1);
	print("5/6 Lado izquierdo hacia abajo: mantener el morro hacia delante, lado izquierdo del dron hacia abajo tocando la superficie, brazo derecho hacia arriba, perpendicular al suelo. Mantener quieto, la calibración comienza en 8 segundos;\n");
	pause(8);
	calibrateAccelOnce();
	print("Calibración lado izquierdo abajo completada. Continuar.\n");
	pause(1);
	print("6/6 Invertido: mantener el morro hacia delante, girar completamente el dron, parte superior hacia abajo, inferior hacia arriba, totalmente horizontal invertido. Mantener quieto, la calibración comienza en 8 segundos;\n");
	pause(8);
	calibrateAccelOnce();

	printIMUCalibration();
	print("✓ ¡Calibración completa! Colocar el dron en posición normal y ejecutar ps para ver el resultado.\n");
	configureIMU();
}

void calibrateAccelOnce() {
	const int samples = 1000;
	static Vector accMax(-INFINITY, -INFINITY, -INFINITY);
	static Vector accMin(INFINITY, INFINITY, INFINITY);

	// Promedio de las lecturas del acelerómetro
	acc = Vector(0, 0, 0);
	for (int i = 0; i < samples; i++) {
		imu.waitForData();
		Vector sample;
		imu.getAccel(sample.x, sample.y, sample.z);
		acc = acc + sample;
	}
	acc = acc / samples;

	// Actualizar los valores máximo y mínimo
	if (acc.x > accMax.x) accMax.x = acc.x;
	if (acc.y > accMax.y) accMax.y = acc.y;
	if (acc.z > accMax.z) accMax.z = acc.z;
	if (acc.x < accMin.x) accMin.x = acc.x;
	if (acc.y < accMin.y) accMin.y = acc.y;
	if (acc.z < accMin.z) accMin.z = acc.z;
	// Calcular escala y sesgo
	accScale = (accMax - accMin) / 2 / ONE_G;
	accBias = (accMax + accMin) / 2;
}

void printIMUCalibration() {
	print("sesgo giro: %f %f %f\n", gyroBias.x, gyroBias.y, gyroBias.z);
	print("sesgo acelerómetro: %f %f %f\n", accBias.x, accBias.y, accBias.z);
	print("escala acelerómetro: %f %f %f\n", accScale.x, accScale.y, accScale.z);
}

void printIMUInfo() {
	imu.status() ? print("estado: ERROR %d\n", imu.status()) : print("estado: OK\n");
	print("modelo: %s\n", imu.getModel());
	print("who am I: 0x%02X\n", imu.whoAmI());
	print("frecuencia: %.0f\n", loopRate);
	print("giroscopio: %f %f %f\n", gyro.x, gyro.y, gyro.z);
	print("acelerómetro: %f %f %f\n", acc.x, acc.y, acc.z);
	imu.waitForData();
	Vector rawGyro, rawAcc;
	imu.getGyro(rawGyro.x, rawGyro.y, rawGyro.z);
	imu.getAccel(rawAcc.x, rawAcc.y, rawAcc.z);
	print("giroscopio crudo: %f %f %f\n", rawGyro.x, rawGyro.y, rawGyro.z);
	print("acelerómetro crudo: %f %f %f\n", rawAcc.x, rawAcc.y, rawAcc.z);
}