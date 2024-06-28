#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/alerts_fifo2"
#define BUFFER_SIZE 1024
#define THRESHOLD_TEMP 38.0
#define NUM_SENSORS 4

// Struct for sensor data
typedef struct
{
    char name[20];
    float temperature;
} Sensor;

/**
 * @brief Sends an alert message via FIFO.
 *
 * This function sends an alert message containing information about the high temperature detected at a particular
 * sensor.
 *
 * @param temperature The temperature value triggering the alert.
 * @param sensor_name The name of the sensor where the high temperature was detected.
 */
void send_alert(float temperature, const char* sensor_name);

/**
 * @brief Generates a random temperature value.
 *
 * This function generates a random temperature value, simulating temperature readings from a sensor.
 *
 * @return A random float value representing the temperature.
 */
float get_temperature();

/**
 * @brief Checks temperature thresholds and sends alerts if necessary.
 *
 * This function checks the temperature of each sensor against a predefined threshold. If the temperature exceeds the
 * threshold, an alert is sent.
 *
 * @param sensors An array of Sensor structures representing the sensors.
 * @param num_sensors The number of sensors in the array.
 */
void check_temperature_threshold(Sensor sensors[], int num_sensors);

/**
 * @brief Updates sensor values with random temperatures.
 *
 * This function updates the temperature values of the sensors with random values.
 *
 * @param sensors An array of Sensor structures representing the sensors.
 * @param num_sensors The number of sensors in the array.
 */
void update_sensor_values(Sensor sensors[], int num_sensors);
