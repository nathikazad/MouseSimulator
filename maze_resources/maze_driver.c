#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
typedef int bool;
#define true 1
#define false 0

char *socket_path = "\0hidden";
int detail_driver=0;
int fd;
int starting_corner_position;
int starting_direction;
int number_of_cells;

bool connect_server(char* sock)
{
    socket_path=sock;
    struct sockaddr_un addr;
    char read_buf[100];
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      perror("socket error");
      return false;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("connect error");
      return false;
    }
    read(fd, read_buf, sizeof(read_buf));
    if(read_buf[61]=='1')
        detail_driver=1;
    else
        detail_driver=0;

    starting_corner_position=read_buf[33]-(int)'0';
    starting_direction=read_buf[45]-(int)'0';

    number_of_cells=0;
    if(read_buf[23]!=' ')
        number_of_cells+=(read_buf[23]-(int)'0')*10;
    number_of_cells+=(read_buf[24]-(int)'0');
    printf("%d \n", detail_driver);
    return true;
}

void request(char* msg, int length, char response[])
{
    //writing to socket
    if (write(fd, msg, length) != length) {
      if (length > 0) fprintf(stderr,"partial write");
      else {
        perror("write error");
        exit(-1);
      }
    }
    //reading from socket
    int size=read(fd, response, 100);
    response[size]=0;
}
void move_forward_units(float units)
{
    char read_buf[100];
    char write_buf[100];
    if(units<0)
        units=units*-1;
    int left=(int)units;
    int right=(units-left)*100;
    sprintf(write_buf, "move forward %d.%d \n", left, right);
    printf("%s", write_buf);
    request(write_buf, 100, read_buf);
}
void move_forward()
{
    if(detail_driver==1)
        move_forward_units(1.0);
    else
    {
        char read_buf[100];
        char *write_buf="move forward\n";
        request(write_buf, 100, read_buf);
    }
}
void rotate_units(float units)
{
    char read_buf[100];
    char write_buf[100];
    int left=(int)units;
    int right=(units-left)*100;
    sprintf(write_buf, "rotate %d.%d \n", left, right);
    printf("%s", write_buf);
    request(write_buf, 100, read_buf);
}
void rotate_right()
{
    if(detail_driver==1)
        rotate_units(-90.0);
    else
    {
        char read_buf[100];
        char *write_buf="rotate right\n";
        request(write_buf, 100, read_buf);
    }
}

void rotate_left()
{
    if(detail_driver==1)
        rotate_units(90.0);
    else
    {
        char read_buf[100];
        char *write_buf="rotate left\n";
        request(write_buf, 100, read_buf);
    }
}

bool check_front()
{
    char read_buf[100];
    char *write_buf="wall in front?\n";
    request(write_buf, 100, read_buf);
    return (read_buf[0]=='1');
}

bool check_right()
{
    char read_buf[100];
    char *write_buf="wall to the right?\n";
    request(write_buf, 100, read_buf);
    return (read_buf[0]=='1');
}

bool check_left()
{
    char read_buf[100];
    char *write_buf="wall to the left?\n";
    request(write_buf, 100, read_buf);
    return (read_buf[0]=='1');
}

bool solved(int x, int y)
{
    if(number_of_cells%2==1)
    {
        return (x==number_of_cells/2 && y==number_of_cells/2);
    }
    return ((x==number_of_cells/2||x==(number_of_cells-1)/2 )&& (y==number_of_cells/2 || y==(number_of_cells-1)/2));;
}

int get_starting_position_x()
{
    //top-left 0
    //top-right 1
    //bottom-right 2
    //bottom-left 3
    if(starting_corner_position==0 || starting_corner_position==3)
        return 0;
    else
        return number_of_cells-1;
}
int get_starting_position_y()
{
    if(starting_corner_position==0 || starting_corner_position==1)
            return 0;
        else
            return number_of_cells-1;
}
int get_starting_direction()
{
    return starting_direction;
}
int get_num_of_cells()
{
    return number_of_cells;
}