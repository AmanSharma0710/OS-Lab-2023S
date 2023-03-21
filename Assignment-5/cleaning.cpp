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

void* cleaning_staff_thread_fn(void *arg)
{
    while(1)
    {
        sem_wait(&cleaning_mode);
        int* temp= (int*)arg;
        int id = *temp;
        // flag is used to check if any room was cleaned or not
        int flag=0;

        for(int i=0;i<n;i++)
        {
            sem_wait(&room_access[i]);
            if(rooms[i].total_occupants>=2)
            {
                sem_wait(&room_busy[i]);
                sem_wait(&print_access);
                cout<<"Cleaning staff "<<id<<" is cleaning room "<<i<<"."<<endl;
                sem_post(&print_access);
                rooms[i].occupied=0;
                rooms[i].curr_guest=0;
                rooms[i].curr_stay_start=0;
                rooms[i].curr_stay_time=0;

                rooms[i].total_occupants=0;
                auto wait = rooms[i].occupied_time;
                rooms[i].occupied_time=0;
                sem_post(&room_access[i]);

                sem_wait(&rooms_with_2_occupants_access);
                rooms_with_2_occupants=0;
                sem_post(&rooms_with_2_occupants_access);

                sem_wait(&total_guests_curr_access);
                total_guests_curr=0;
                sem_post(&total_guests_curr_access);

                sleep(wait);
                sem_post(&room_busy[i]);
                sem_wait(&print_access);
                cout<<"Cleaning staff "<<id<<" has cleaned room "<<i<<"."<<endl;
                flag=1;
                sem_post(&print_access);
                
                break;
            }
            sem_post(&room_access[i]);
        }
        if(flag==0)
        {
            sem_post(&cleaning_mode);
        }

        sem_wait(&allotment_possible_access);
        allotment_possible++;
        sem_post(&allotment_possible_access);

    }
}