#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "semaphore.h"

#include "seats.h"

#define STANDBY_SIZE 10


seat_t* seat_header = NULL;


int standby_size;
standby_node* standby_list;
standby_node* standby_tail;
m_sem_t standby_sem;

char seat_state_to_char(seat_state_t);

//adds to standby list
void add_to_standby(char* buf,int bufsize,int seat_id,int customer_id);

//assigns from standby list if possible
void assign_standby(char* buf,int bufsize,int seat_id);

//dequeue standby list
standby_node* get_standby(char* buf,int bufsize,int seat_id);


void list_seats(char* buf, int bufsize)
{
    seat_t* curr = seat_header;
    int index = 0;
    while(curr != NULL && index < bufsize+ strlen("%d %c,"))
    {
        int length = snprintf(buf+index, bufsize-index, 
                "%d %c,", curr->id, seat_state_to_char(curr->state));
        if (length > 0)
            index = index + length;
        curr = curr->next;
    }
    if (index > 0)
        snprintf(buf+index-1, bufsize-index-1, "\n");
    else
        snprintf(buf, bufsize, "No seats not found\n\n");
}

void view_seat(char* buf, int bufsize,  int seat_id, int customer_id, int customer_priority)
{
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        if(curr->id == seat_id)
        {
            pthread_mutex_lock(&(curr->lock));
            if(curr->state == AVAILABLE || (curr->state == PENDING && curr->customer_id == customer_id))
            {
                snprintf(buf, bufsize, "Confirm seat: %d %c ?\n\n",
                curr->id, seat_state_to_char(curr->state));
                curr->state = PENDING;
                curr->customer_id = customer_id;
            }
            else
            {
                snprintf(buf, bufsize, "Seat unavailable\n\n");
                //
                // add to standby list
                sem_wait(&standby_sem);
                add_to_standby(buf,bufsize,seat_id,customer_id);
                sem_post(&standby_sem);
                //
                //
                
            }
            pthread_mutex_unlock(&(curr->lock));

            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Requested seat not found\n\n");
    return;
}

void confirm_seat(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        if(curr->id == seat_id)
        {
            pthread_mutex_lock(&(curr->lock));
            if(curr->state == PENDING && curr->customer_id == customer_id )
            {
                snprintf(buf, bufsize, "Seat confirmed: %d %c\n\n",
                        curr->id, seat_state_to_char(curr->state));
                curr->state = OCCUPIED;
            }
            else if(curr->customer_id != customer_id )
            {
                snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
            }
            else if(curr->state != PENDING)
            {
                snprintf(buf, bufsize, "No pending request\n\n");
            }
            
            pthread_mutex_unlock(&(curr->lock));
            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Requested seat not found\n\n");
    
    return;
}

void cancel(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        if(curr->id == seat_id)
        {
            pthread_mutex_lock(&(curr->lock));
            if(curr->state == PENDING && curr->customer_id == customer_id )
            {
                snprintf(buf, bufsize, "Seat request cancelled: %d %c\n\n",
                        curr->id, seat_state_to_char(curr->state));
                curr->state = AVAILABLE;
                curr->customer_id = -1;
                
                //
                //if possible, assign to someonefrom standby list
                assign_standby(buf,bufsize,seat_id);
                //
                //
                
            }
            else if(curr->customer_id != customer_id )
            {
                snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
            }
            else if(curr->state != PENDING)
            {
                snprintf(buf, bufsize, "No pending request\n\n");
            }
            
            pthread_mutex_unlock(&(curr->lock));

            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Seat not found\n\n");
    
    return;
}

void load_seats(int number_of_seats)
{
    seat_t* curr = NULL;
    int i;
    for(i = 0; i < number_of_seats; i++)
    {   
        seat_t* temp = (seat_t*) malloc(sizeof(seat_t));
        temp->id = i;
        temp->customer_id = -1;
        temp->state = AVAILABLE;
        temp->next = NULL;
        pthread_mutex_init(&(temp->lock),NULL);
        
        if (seat_header == NULL)
        {
            seat_header = temp;
        }
        else
        {
            curr-> next = temp;
        }
        curr = temp;
    }
}

void unload_seats()
{
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        seat_t* temp = curr;
        curr = curr->next;
        pthread_mutex_destroy(&(temp->lock));
        free(temp);
    }
}

char seat_state_to_char(seat_state_t state)
{
    switch(state)
    {
        case AVAILABLE:
            return 'A';
        case PENDING:
            return 'P';
        case OCCUPIED:
            return 'O';
    }
    return '?';
}


void add_to_standby(char* buf,int bufsize,int seat_id,int customer_id)
{
    printf("adding to standby \n");
    //if standby is too big, say no
    if (standby_size==STANDBY_SIZE)
    {
        printf("standby is full \n");
        //list is full, return nothing
        return;
    }
    
    
    //adds node to standby list
    //malloc new node
    standby_node* newStandby = (standby_node*)(malloc(sizeof(standby_node)));
    
    //put information in the node
    newStandby->customer_id = customer_id;
    newStandby->seat_id = seat_id;
    newStandby->next = NULL;
    
    //add node 
    standby_node* headNode;
    headNode = standby_list;
    
    if (headNode!=NULL)
    {
        headNode->previous = newStandby;
    }
    
    standby_list = newStandby;
    if (standby_size==0)
    {
        standby_tail = newStandby;
    }
    
    standby_size++;
    
    return;
    
}

//assigns from standby list if possible
standby_node* get_standby(char* buf,int bufsize,int seat_id)
{
   printf("Finding free from standby\n");
   if (standby_list==NULL)
   {
       printf("standby is empty\n");
       //no tasks
       return NULL;
   }
   
   //grab information from the tail
   standby_node* temp;
   temp = standby_tail;
   
   while ((temp!=NULL)&&(temp->seat_id!=seat_id))
   {
       //look for the seat
       printf("checking seat %d \n",temp->seat_id);
       temp = temp->previous;
   }
   
   if (temp==NULL)
   {
       return NULL;
   }
   
   //else, rearrange the list
   standby_size--;
   
   //if empty, set pointers to null and return node
   if (standby_size==0)
   {
       standby_list = NULL;
       standby_tail = NULL;
       return temp;
   }
   
    //if head, update head to point to next
    if (temp==standby_list)
    {
        standby_list = temp->next;
        standby_list->previous = NULL;
        return temp;
    }
    //if tail, update tail to point to previous
    if (temp==standby_tail)
    {
        standby_tail = temp->previous;
        standby_tail->next = NULL;
        return temp;
    }
    //otherwise, set previous to next and next to previous
    standby_node* tempNext = temp->next;
    standby_node* tempPrev = temp->previous;
    tempPrev->next = tempNext;
    tempNext->previous = tempPrev;

    //and return 
   return temp;
}

void assign_standby(char* buf,int bufsize,int seat_id)
{
    //lock the list 
    sem_wait(&standby_sem);
    standby_node* to_assign = get_standby(buf,bufsize,seat_id);
    
    //if null, nothing
    if (to_assign==NULL)
    {
        sem_post(&standby_sem);
        return;
    }
    int new_customer = to_assign->customer_id;
    free(to_assign);
    sem_post(&standby_sem);
    
    //if not null, assign the seat to standby
    seat_t* current;
    current = seat_header;
    
    while (current!=NULL)
    {
        //step through seats
        if (current->id==seat_id)
        {
            //not sure if need to lock here
            //pthread_mutex_lock(&(curr->lock));
            
            //change assignment of seat
            if(current->state == AVAILABLE)
            {
                //snprintf(buf,bufsize,"Standby List takes seat %d \n\n",seat_id);
                //set as pending since they haven't confirmed
                current->state = PENDING;
                //assign it
                current->customer_id = new_customer;
            }
            else
            {
                //snprintf(buf,bufsize,"SEAT unavailable\n\n");

            }
            
            //not sure if need to unlock here
            //pthread_mutex_unlock(&(curr->lock));
            
            //exit
            return;
            
        }
        current = current->next;
    }
    
}

void kill_standby()
{
    //destroys the standby list
    standby_node* curr = standby_list;
    while(curr != NULL)
    {
        standby_node* temp = curr;
        curr = curr->next;
        free(temp);
    }
    //now set head and tail to null
    standby_list = NULL;
    standby_tail = NULL;
    standby_size = 0;
    sem_kill(&standby_sem);
    
}


void initialize_standby()
{
    //creates the semaphore
    //sets pointers in list to NULL;
    standby_list = NULL;
    standby_tail = NULL;
    standby_size = 0;
    sem_initialize(&standby_sem,1);
    return;
} 