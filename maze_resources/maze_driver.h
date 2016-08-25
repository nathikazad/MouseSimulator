typedef int bool;
#define true 1
#define false 0

bool connect_server(char* sock);
void move_forward();
void move_forward_units(float units);
void rotate_units(float units);
bool check_right();
bool check_front();
bool check_left();
bool solved(int x, int y);
void rotate_right();
void rotate_left();
void request(char* msg, int length, char response[]);
int get_starting_position_x();
int get_starting_position_y();
int get_starting_direction();
int get_num_of_cells();
