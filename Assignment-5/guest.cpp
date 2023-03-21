#include "header.h"

extern int x;
extern int y;
extern int n;
extern room *rooms;
extern int *guest_priority;
extern int *guest_removed_status;
extern int total_guests_curr ;

extern int rooms_with_2_occupants ; //used to trigger cleaning mode
extern int allotment_possible; //not in cleaning mode

extern sem_t *room_access; //ability to access room's data
extern sem_t *room_busy; //room is being stayed in or being cleaned, hence busy
extern sem_t *guest_removed_status_access;
extern sem_t total_guests_curr_access;
extern sem_t rooms_with_2_occupants_access;
extern sem_t cleaning_mode;
extern sem_t allotment_possible_access;
extern sem_t print_access;

void* guest_thread_fn(void *arg)
{
    while(1)
    {
        int rand_time = rand() % 10 + 10;
        sleep(rand_time);

        int* temp= (int*)arg;
        int id = *temp;
        int stay_time = rand() % 20 + 10;
        int room_allocated = -1;

        //if guest is removed from the hotel, it will try to complete previous stay time first in the next iterations
        sem_wait(&guest_removed_status_access[id]);
        if(guest_removed_status[id] < 0)
        {
            guest_removed_status[id]=0;
        }
        if(guest_removed_status[id] != 0)
        {
            stay_time = guest_removed_status[id];
            guest_removed_status[id]=0;
        }
        sem_post(&guest_removed_status_access[id]);

        sem_wait(&print_access);
        cout << "Guest " << id << " is requesting a stay for " << stay_time << " seconds." << endl;
        sem_post(&print_access);
        sem_wait(&allotment_possible_access);
        if(allotment_possible!=1)
        {
            sem_post(&allotment_possible_access);
            sem_wait(&guest_removed_status_access[id]);
            guest_removed_status[id] = stay_time;
            sem_post(&guest_removed_status_access[id]);
            continue;
        }
        sem_post(&allotment_possible_access);

        sem_wait(&total_guests_curr_access);
        if (total_guests_curr < n)
        {
            sem_post(&total_guests_curr_access);
            for (int i = 0; i < n; i++)
            {
                sem_wait(&room_access[i]);
                if (rooms[i].occupied == 0)
                {
                    rooms[i].occupied = 1;
                    rooms[i].curr_guest = id;
                    rooms[i].curr_stay_time = stay_time;
                    rooms[i].curr_stay_start = time(NULL);
                    rooms[i].total_occupants++;
                    rooms[i].occupied_time += stay_time;
                    sem_wait(&total_guests_curr_access);
                    total_guests_curr++;
                    sem_post(&total_guests_curr_access);
                    sem_wait(&print_access);
                    cout << "Guest " << id << " is staying in room " << i << " for " << stay_time << " seconds." << endl;
                    sem_post(&print_access);
                    room_allocated = i;
                    sem_post(&room_access[i]);
                    break;
                }
                sem_post(&room_access[i]);
            }
        }
        else
        {
            sem_post(&total_guests_curr_access);
            int min_priority = 1000000;
            int min_priority_room = -1;
            for (int i = 0; i < n; i++)
            {
                sem_wait(&room_access[i]);
                if (rooms[i].occupied == 1)
                {
                    if (guest_priority[rooms[i].curr_guest] < min_priority)
                    {
                        min_priority = guest_priority[rooms[i].curr_guest];
                        min_priority_room = i;
                    }
                    
                }
                sem_post(&room_access[i]);
            }
            if (min_priority_room != -1)
            {
                if (guest_priority[id] > min_priority)
                {
                    time_t curr_time = time(NULL);
                    sem_wait(&room_access[min_priority_room]);
                    sem_wait(&guest_removed_status_access[rooms[min_priority_room].curr_guest]);
                    guest_removed_status[rooms[min_priority_room].curr_guest] = rooms[min_priority_room].curr_stay_time - (curr_time - rooms[min_priority_room].curr_stay_start);
                    sem_wait(&print_access);
                    cout << "Guest " << rooms[min_priority_room].curr_guest << " is removed from room " << min_priority_room << " after stay for " << (curr_time - rooms[min_priority_room].curr_stay_start) << " seconds." << endl;
                    sem_post(&print_access);
                    sem_post(&guest_removed_status_access[rooms[min_priority_room].curr_guest]);

                    rooms[min_priority_room].curr_guest = id;
                    rooms[min_priority_room].curr_stay_time = stay_time;
                    rooms[min_priority_room].curr_stay_start = time(NULL);

                    rooms[min_priority_room].total_occupants++;
                    sem_wait(&print_access);
                    cout << "Guest " << id << " is staying in room " << min_priority_room << " for " << stay_time << " seconds." << endl;
                    sem_post(&print_access);
                    sem_wait(&guest_removed_status_access[rooms[min_priority_room].curr_guest]);
                    rooms[min_priority_room].occupied_time += stay_time - guest_removed_status[rooms[min_priority_room].curr_guest];
                    sem_post(&guest_removed_status_access[rooms[min_priority_room].curr_guest]);
                    sem_post(&room_access[min_priority_room]);
                    room_allocated = min_priority_room;
                }
                else
                {
                    sem_wait(&guest_removed_status_access[id]);
                    guest_removed_status[id] = stay_time;
                    sem_post(&guest_removed_status_access[id]);
                }
            }
        }
        if(room_allocated!=-1)
        {
            sem_wait(&room_busy[room_allocated]);
            sleep(stay_time);
            
            sem_wait(&guest_removed_status_access[id]);
            if(guest_removed_status[id]!=0)
            {
                sem_post(&guest_removed_status_access[id]);
                sem_post(&room_busy[room_allocated]);
                continue;
            }
            sem_post(&guest_removed_status_access[id]);
            sem_wait(&print_access);
            cout << "Guest " << id << " has completed stay in room " << room_allocated<<"." << endl;
            sem_post(&print_access);
            sem_post(&room_busy[room_allocated]);
            sem_wait(&room_access[room_allocated]);
            if(rooms[room_allocated].total_occupants>=2)
            {
                sem_post(&room_access[room_allocated]);
                sem_wait(&rooms_with_2_occupants_access);
                rooms_with_2_occupants++;
                if(rooms_with_2_occupants>=n)
                {
                    for(int i=0;i<n;i++)
                    {
                        sem_post(&cleaning_mode);                        
                    }

                    rooms_with_2_occupants=0;
                    sem_wait(&allotment_possible_access);
                    allotment_possible=-1*n + 1;
                    sem_post(&allotment_possible_access);
                    sem_wait(&total_guests_curr_access);
                    total_guests_curr=0;
                    sem_post(&total_guests_curr_access);
                }
                sem_post(&rooms_with_2_occupants_access);
            }
            else
            {
                sem_post(&room_access[room_allocated]);
            }
            sem_wait(&room_access[room_allocated]);
            rooms[room_allocated].occupied = 0;
            sem_post(&room_access[room_allocated]);

            sem_wait(&total_guests_curr_access);
            total_guests_curr--;
            sem_post(&total_guests_curr_access);
        }
    }
}