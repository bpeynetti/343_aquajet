#ifndef _SEAT_OPERATIONS_H_
#define _SEAT_OPERATIONS_H_

typedef enum 
{
    AVAILABLE, 
    PENDING, 
    OCCUPIED
} seat_state_t;

typedef struct seat_struct
{
    pthread_mutex_t lock;
    int id;
    int customer_id;
    seat_state_t state;
    struct seat_struct* next;
} seat_t;

typedef struct standby_node_struct
{
    int customer_id;
    int seat_id;
    struct standby_node_struct* next;
    struct standby_node_struct* previous;
} standby_node;


void load_seats(int);
void unload_seats();

void list_seats(char* buf, int bufsize);
void view_seat(char* buf, int bufsize, int seat_num, int customer_num, int customer_priority);
void confirm_seat(char* buf, int bufsize, int seat_num, int customer_num, int customer_priority);
void cancel(char* buf, int bufsize, int seat_num, int customer_num, int customer_priority);

void initialize_standby();
void kill_standby();



#endif
