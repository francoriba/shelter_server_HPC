#include "alertInfection.h"

void update_sensor_values(Sensor sensors[], int num_sensors)
{
    for (int i = 0; i < num_sensors; i++)
    {
        sensors[i].temperature = get_temperature();
    }
}

void check_temperature_threshold(Sensor sensors[], int num_sensors)
{
    for (int i = 0; i < num_sensors; i++)
    {
        if (sensors[i].temperature > THRESHOLD_TEMP)
        {
            // printf("%s", sensors[i].name);
            send_alert(sensors[i].temperature, sensors[i].name);
        }
    }
}

void send_alert(float temperature, const char* sensor_name)
{
    int fd;
    // Open FIFO for writing
    fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1)
    {
        perror("Error opening FIFO");
        return;
    }
    // Build alert message
    char alert_message[BUFFER_SIZE];
    memset(alert_message, 0, sizeof(alert_message));
    sprintf(alert_message, "%s, ALERT, %.1f°C ", sensor_name, temperature);

    ssize_t bytes_written = write(fd, alert_message, strlen(alert_message));
    if (bytes_written == -1)
    {
        perror("Error writing to FIFO");
    }
    else if (bytes_written != (ssize_t)strlen(alert_message))
    {
        fprintf(stderr, "Not able to write all data to FIFO\n");
    }
    else
    {
        // printf("Sent alert to FIFO: %s\n", alert_message);
    }
    close(fd);
}

float get_temperature()
{
    // Generate a random float value between 0 and 1
    float random_value = ((float)rand()) / (float)RAND_MAX;
    // Probability of generating a high temperature (1 out of 1000 times)
    float high_temperature_probability = 0.00000000001f;
    // If the random value is less than the high temperature probability, generate a high temperature
    if (random_value < high_temperature_probability)
    {
        // Generate a high temperature between 38 and 43 degrees Celsius
        // printf("random tempeture is: %f\n", ((float)(rand() % 51 + 380) / 10.0f));
        return ((float)(rand() % 51 + 380) / 10.0f); // Between 38.0 and 43.0
    }
    else
    {
        // Generate a normal temperature between 35 and 37.9 degrees Celsius
        // printf("random tempeture is: %f\n", ((float)(rand() % 40 + 350) / 10.0f));
        return ((float)(rand() % 40 + 350) / 10.0f); // Between 35.0 and 37.9
    }
}
