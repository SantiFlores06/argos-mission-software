uint8 SEVERITY_WARNING  = 1
uint8 SEVERITY_ERROR    = 2
uint8 SEVERITY_CRITICAL = 3

uint8 SUBSYSTEM_GPS        = 1
uint8 SUBSYSTEM_IMU        = 2
uint8 SUBSYSTEM_BATTERY    = 3
uint8 SUBSYSTEM_GCS_LINK   = 4
uint8 SUBSYSTEM_EKF        = 5
uint8 SUBSYSTEM_NAVIGATION = 6

std_msgs/Header header
uint32 fault_id
uint8 subsystem
uint8 severity
string description
float64 metric_value
float64 threshold_value
