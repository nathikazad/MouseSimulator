#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "maze_driver.h"

typedef struct cell cell;
#define up            0
#define right         1
#define down          2
#define left          3
#define clockwise     1
#define ct_clockwise -1
#define true          1
#define false         0
struct cell {
    int    x;
    int    y;
};


int **values;

int **visited_map;
int **vwalls;
int **hwalls;
int direction_map[4][2] = {
        { 0, -1},
        { 1,  0},
        { 0,  1},
        {-1 , 0}
};
int current_direction;
cell accessible_neighbors[4];
cell current_cell;
int number_of_accessible_neighbors=0;

cell* stack;
int stack_count = 0;

cell newly_blocked_cells[4];
int  newly_blocked_cells_count=0;

int chosen_x_index;
int chosen_y_index;

void set_visited(cell a)
{
    visited_map[a.y][a.x]=1;
}

int visited(cell a)
{
    return visited_map[a.y][a.x];
}
int value(cell a)
{
    return values[a.y][a.x];
}
int current_value()
{
    return value(current_cell);
}

void increment_cell_value(cell a)
{
    values[a.y][a.x]++;
}

void decrement_cell_value(cell a)
{
    values[a.y][a.x]--;
}
int in_bounds(cell a)
{
    return (a.x>=0 && a.x < get_num_of_cells() && a.y>=0 && a.y < get_num_of_cells());
}

int choose_wall_array(cell a, cell b)
{
    if(a.x < 0 || b.x < 0 || a.y < 0 || b.y < 0)
        return -1;

    if(a.x==b.x)
    {
        if(a.y>= get_num_of_cells() || b.y >= get_num_of_cells())
            return -1;
        int y;
        if(a.y<b.y)
            y=a.y;
        else
            y=b.y;
        chosen_x_index=y;
        chosen_y_index=a.x;
        return 1;//hwalls
    }
    else if(a.y==b.y)
    {
        if(a.x>= get_num_of_cells() || b.x >= get_num_of_cells())
            return -1;
        int x;
        if(a.x<b.x)
            x=a.x;
        else
            x=b.x;

        chosen_x_index=a.y;
        chosen_y_index=x;
        return 2;//vwalls
    }
    return -1;
}
int wall_between(cell a, cell b)
{
    int returned=choose_wall_array(a,b);
    if(returned == 1 )
        return hwalls[chosen_x_index][chosen_y_index];
    else if(returned == 2 )
        return vwalls[chosen_x_index][chosen_y_index];
    else
        return -1;
}

void set_wall_between(cell a, cell b)
{

    int returned=choose_wall_array(a,b);
    if( returned < 0)
        ;
    else if(returned == 1 )
        hwalls[chosen_x_index][chosen_y_index]=1;
    else if(returned == 2 )
        vwalls[chosen_x_index][chosen_y_index]=1;
}

void print_maze()
{
    int i,j;
    for(i=0;i< get_num_of_cells();i++)
    {
        for(j=0;j< get_num_of_cells();j++)
        {
            printf("%02d",values[i][j]);

                cell a={j,i};
                cell b={j+1,i};
                if(wall_between(a, b)==1)
                    printf("|");
                else
                    printf(" ");

        }
        printf("\n");
        if(i!=4) {
            for (j = 0; j < get_num_of_cells(); j++) {
                cell a = {j, i};
                cell b = {j, i + 1};
                if (wall_between(a, b) == 1)
                    printf("-- ");
                else
                    printf("   ");
            }
            printf("\n");
        }
    }
    printf("\n \n");
}

void load_accessible_neighbors(cell a)
{
    number_of_accessible_neighbors=0;
    if(a.x!=0) {
        cell left_cell = {a.x - 1, a.y};
        if(wall_between(a, left_cell)==0)
        {
            accessible_neighbors[number_of_accessible_neighbors]=left_cell;
            number_of_accessible_neighbors++;
        }
    }
    if(a.x!= get_num_of_cells()-1) {
        cell right_cell   = {a.x+1, a.y};
        if(wall_between(a, right_cell)==0)
        {
            accessible_neighbors[number_of_accessible_neighbors]=right_cell;
            number_of_accessible_neighbors++;
        }
    }
    if(a.y!=0) {
        cell top_cell     = {a.x  , a.y-1};
        if(wall_between(a, top_cell)==0)
        {
            accessible_neighbors[number_of_accessible_neighbors]=top_cell;
            number_of_accessible_neighbors++;
        }
    }
    if(a.y!= get_num_of_cells()-1) {
        cell bottom_cell  = {a.x  , a.y+1};
        if(wall_between(a, bottom_cell)==0)
        {
            accessible_neighbors[number_of_accessible_neighbors]=bottom_cell;
            number_of_accessible_neighbors++;
        }
    }

}
void sort_neighbors()
{
    int i;
    for(i=1;i<number_of_accessible_neighbors;i++) {
        int j = i;
        while(j>0 && value(accessible_neighbors[j-1])>value(accessible_neighbors[j]))
        {
            cell temp=accessible_neighbors[j-1];
            accessible_neighbors[j-1]=accessible_neighbors[j];
            accessible_neighbors[j]=temp;
            j--;
        }
    }
}
int value_of_minimum_neighbor(cell a)
{
    load_accessible_neighbors(a);
    sort_neighbors();
    return value(accessible_neighbors[0]);
}

cell location_of_minimum_neighbor(cell a)
{
    load_accessible_neighbors(a);
    sort_neighbors();
    return accessible_neighbors[0];
}

int equals(cell a, cell b)
{
    if(a.x==b.x && a.y==b.y)
        return true;
    return false;
}
cell in_front_after(int rotate_direction, int turns)
{
    int new_direction=(current_direction+ rotate_direction * turns+4)%4;
    cell temp_cell={current_cell.x+direction_map[new_direction][0],current_cell.y+direction_map[new_direction][1]};
    return temp_cell;

}
cell cell_in_front()
{
    return in_front_after(0, 0);
}
int wall(int direction)
{

    if(direction==up)
        return wall_between(current_cell, cell_in_front());
    if(direction==right)
        return wall_between(current_cell,in_front_after(1, clockwise));
    if(direction==left)
        return wall_between(current_cell,in_front_after(1, ct_clockwise));
    return -1;
}

void set_wall(int direction)
{
    if(direction==up)
        set_wall_between(current_cell, cell_in_front());
    if(direction==right)
        set_wall_between(current_cell, in_front_after(clockwise, 1));
    if(direction==left)
        set_wall_between(current_cell, in_front_after(ct_clockwise, 1));

}

void set_wall_and_push_to_newly_blocked_cells(cell temp_cell)
{
    if(in_bounds(temp_cell)) {
        set_wall_between(current_cell, temp_cell);
        newly_blocked_cells[newly_blocked_cells_count] = temp_cell;
        newly_blocked_cells_count++;
    }
}

void update_walls_and_save_newly_blocked_cells()
{
    newly_blocked_cells_count=0;
    cell temp_cell;
    if(check_front())
        set_wall_and_push_to_newly_blocked_cells(cell_in_front());
    if(check_right())
        set_wall_and_push_to_newly_blocked_cells(in_front_after(clockwise, 1));
    if(check_left())
        set_wall_and_push_to_newly_blocked_cells(in_front_after(ct_clockwise, 1));
}

void push(cell a)
{
    stack[stack_count]=a;
    stack_count++;
}

void push_accessible_neighbors(cell a)
{
    load_accessible_neighbors(a);
    int i;
    for(i=0;i<number_of_accessible_neighbors;i++)
    {
        push(accessible_neighbors[i]);
    }
}

cell pop()
{
    stack_count--;
    return stack[stack_count];
}

void move_forward_and_update()
{
    move_forward();
    current_cell= cell_in_front();
}
void rotate_right_and_update()
{
    rotate_right();
    current_direction=(current_direction+1)%4;
}
void rotate_left_and_update()
{
    rotate_left();
    current_direction=(current_direction-1+4)%4;
}
void rotate_towards(cell b)
{
    int turns=0;
    int rotate_direction=clockwise;
    while(!equals(in_front_after(clockwise, turns), b))
        turns++;
    if (turns == 3)
    {
        turns = 1;
        rotate_direction=ct_clockwise;
    }
    int i;
    for(i=0; i<turns;i++)
    {
        if(rotate_direction==clockwise)
            rotate_right_and_update();
        else
            rotate_left_and_update();
    }
}
void generate_values()
{
    int i,j;
    int vertical;
    if(get_num_of_cells()%2==0)
        vertical = get_num_of_cells()-1;
    else
        vertical = get_num_of_cells();
    int vflip=-1;
    for(i=0;i< get_num_of_cells();i++)
    {
        vertical=vertical+vflip;
        int horizontal=vertical;
        int hflip=-1;
        for(j=0;j< get_num_of_cells();j++)
        {

            if(get_num_of_cells()%2!=0 && j==get_num_of_cells()/2)
                hflip=1;
            if(get_num_of_cells()%2==0 && j==get_num_of_cells()/2)
            {
                hflip=1;
                horizontal++;
            }
            values[i][j]=horizontal;
            horizontal=horizontal+hflip;
        }
        if(get_num_of_cells()%2!=0 && i==get_num_of_cells()/2)
            vflip=1;
        if(get_num_of_cells()%2==0 && i==get_num_of_cells()/2)
        {
            vflip=1;
            vertical++;
        }

    }
}
void push_newly_blocked_cells_and_clear()
{
    int i=0;
    for(;i<newly_blocked_cells_count;i++)
        push(newly_blocked_cells[i]);
    newly_blocked_cells_count=0;
}
int center(cell a)
{
    if(get_num_of_cells()%2==0)
        return ((a.x==get_num_of_cells()/2 || a.x==get_num_of_cells()/2-1) && (a.y==get_num_of_cells()/2 || a.y==get_num_of_cells()/2-1));
    else
        return (a.x==get_num_of_cells()/2 && a.y==get_num_of_cells()/2);
}

void init_memory()
{
    int i,j;
    values = malloc(get_num_of_cells() * sizeof(int *));
    for (i = 0; i < get_num_of_cells(); i++)
      values[i] = malloc(get_num_of_cells() * sizeof(int));

    visited_map = malloc(get_num_of_cells() * sizeof(int *));
    for (i = 0; i < get_num_of_cells(); i++)
    {
      visited_map[i] = malloc(get_num_of_cells() * sizeof(int));
      for (j = 0; j < get_num_of_cells(); j++)
        visited_map[i][j]=0;
    }


        vwalls = malloc(get_num_of_cells() * sizeof(int *));
        for (i = 0; i < get_num_of_cells(); i++)
        {
          vwalls[i] = malloc((get_num_of_cells()-1) * sizeof(int));
          for (j = 0; j < (get_num_of_cells()-1); j++)
            vwalls[i][j]=0;
        }

        hwalls = malloc((get_num_of_cells()-1) * sizeof(int *));
        for (i = 0; i < (get_num_of_cells()-1); i++)
        {
          hwalls[i] = malloc(get_num_of_cells() * sizeof(int));
          for (j = 0; j < get_num_of_cells(); j++)
            hwalls[i][j]=0;
        }

        stack = malloc(get_num_of_cells() * get_num_of_cells() * sizeof(cell *));

}
void free_memory()
{
    int i;
        for (i = 0; i < get_num_of_cells(); i++)
          free(values[i]);
    free(values);

}
int main(int argc, char *argv[]){
    if(connect_server(argv[1])==false)
        exit(-1);
    init_memory();
    generate_values();
    print_maze();
    current_cell.x=get_starting_position_x();
    current_cell.y=get_starting_position_y();
    current_direction=get_starting_direction();

    while(!solved(current_cell.x, current_cell.y)) {
        update_walls_and_save_newly_blocked_cells();
        set_visited(current_cell);
        push(current_cell);
        push_newly_blocked_cells_and_clear();
        while (stack_count != 0) {
            cell popped = pop();
            if (!center(popped) && value(popped) != value_of_minimum_neighbor(popped)+1 ) {
                increment_cell_value(popped);
                push(popped);
                push_accessible_neighbors(popped);
            }
        }
        if (wall(up) || visited(cell_in_front())) {
            load_accessible_neighbors(current_cell);
            sort_neighbors();
            int i=0;
            while(visited(accessible_neighbors[i]) && i<number_of_accessible_neighbors)
                i++;
            if(i==number_of_accessible_neighbors)
                rotate_towards(accessible_neighbors[0]);
            else
                rotate_towards(accessible_neighbors[i]);
        }
        move_forward_and_update();
        print_maze();
    }
    //go back to start
    //now kill it
    print_maze();
   free_memory();
   return 0;
}