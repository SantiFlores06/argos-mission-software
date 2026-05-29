uint8 IDLE    = 0
uint8 LOADING = 1
uint8 RUNNING = 2
uint8 PAUSED  = 3
uint8 COMPLETE = 4
uint8 ABORTED = 5
uint8 FAILED  = 6

std_msgs/Header header
uint8 status
uint32 current_waypoint_index
uint32 total_waypoints
float32 mission_progress_pct
float64 distance_to_next_wp_m
string active_bt_node
