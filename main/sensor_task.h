#ifndef __SENSOR_TASK_H
#define __SENSOR_TASK_H

typedef void (*sensor_reporter_t)(const char* dev_id, const char* data);

extern int mqtt_num_error;

void sht3x_task(const char* device_id, sensor_reporter_t reporter);
void sps30_task(const char* device_id, sensor_reporter_t reporter);
void mqtt_task_status_print(void);
void mqtt_task_publish(const char* device_id, const char* data);
void mqtt_task(const char* broker_uri, const char* client_id, const char* username, const char* password);


#endif // __SENSOR_TASK_H

