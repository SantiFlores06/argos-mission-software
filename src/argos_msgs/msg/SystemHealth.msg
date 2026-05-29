uint8 HEALTH_OK      = 0
uint8 HEALTH_DEGRADED = 1
uint8 HEALTH_FAULTED  = 2

std_msgs/Header header
uint8 overall_health
uint8 gps_health
uint8 imu_health
uint8 battery_health
uint8 gcs_link_health
uint8 ekf_health
float32 battery_voltage
float32 battery_pct
uint8 gps_satellites
float32 gps_hdop
float32 ekf_covariance_norm
bool ekf_dead_reckoning_active
