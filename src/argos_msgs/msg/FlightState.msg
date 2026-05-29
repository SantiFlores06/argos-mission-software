uint8 UNINITIALIZED  = 0
uint8 PREFLIGHT      = 1
uint8 ARMED          = 2
uint8 TAKEOFF        = 3
uint8 MISSION        = 4
uint8 HOLD           = 5
uint8 RETURN_TO_HOME = 6
uint8 LAND           = 7
uint8 DISARMED       = 8
uint8 FAULTED        = 9

std_msgs/Header header
uint8 state
string state_name
string last_event
builtin_interfaces/Time last_transition_time
