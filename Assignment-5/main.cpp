// todo: sleep times are currently divided by 10 for fast testing


#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <vector>
#include <algorithm>

using namespace std;

typedef struct room_t
{
    int occupied = 0;
    int curr_guest = 0;
    int curr_stay_time = 0;
    time_t curr_stay_start = 0;
    int total_occupants = 0;
    int occupied_time = 0;
} room;

int x, y, n;
room *rooms;
int *guest_priority;
int *guest_removed_status;
int total_guests_curr = 0;

int rooms_with_2_occupants = 0;
int allotment_possible=1;

sem_t *room_access;
sem_t *guest_removed_status_access;
sem_t total_guests_curr_access;
sem_t rooms_with_2_occupants_access;
sem_t cleaning_mode;
sem_t allotment_possible_access;
sem_t print_access;

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

        //if guest is removed from the hotel, it will try to complete previous stay time
        sem_wait(&guest_removed_status_access[id]);
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
                    sem_post(&room_access[i]);
                    if (guest_priority[rooms[i].curr_guest] < min_priority)
                    {
                        min_priority = guest_priority[rooms[i].curr_guest];
                        min_priority_room = i;
                    }
                }
            }
            if (min_priority_room != -1)
            {
                if (guest_priority[id] > min_priority)
                {
                    time_t curr_time = time(NULL);
                    sem_wait(&room_access[min_priority_room]);
                    sem_wait(&guest_removed_status_access[rooms[min_priority_room].curr_guest]);
                    guest_removed_status[rooms[min_priority_room].curr_guest] = rooms[min_priority_room].curr_stay_time - (curr_time - rooms[min_priority_room].curr_stay_start);
                    sem_post(&guest_removed_status_access[rooms[min_priority_room].curr_guest]);

                    rooms[min_priority_room].curr_guest = id;
                    rooms[min_priority_room].curr_stay_time = stay_time;
                    rooms[min_priority_room].curr_stay_start = time(NULL);
                    sem_wait(&total_guests_curr_access);
                    total_guests_curr++;
                    sem_post(&total_guests_curr_access);
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
            sleep(stay_time);
            sem_wait(&print_access);
            cout << "Guest " << id << " has completed stay in room " << room_allocated<<"." << endl;
            sem_post(&print_access);
            sem_wait(&room_access[room_allocated]);
            if(rooms[room_allocated].total_occupants==2)
            {
                sem_wait(&rooms_with_2_occupants_access);
                rooms_with_2_occupants++;
                if(rooms_with_2_occupants>=n)
                {
                    for(int i=0;i<n;i++)
                    {
                        sem_post(&cleaning_mode);                        
                    }
                    // sem_post(&cleaning_mode);
                    rooms_with_2_occupants=0;
                    sem_wait(&allotment_possible_access);
                    allotment_possible=-1*n + 1;
                    sem_post(&allotment_possible_access);
                }
                sem_post(&rooms_with_2_occupants_access);
            }
            rooms[room_allocated].occupied = 0;
            sem_post(&room_access[room_allocated]);
            sem_wait(&total_guests_curr_access);
            total_guests_curr--;
            sem_post(&total_guests_curr_access);
        }
    }
}

void* cleaning_staff_thread_fn(void *arg)
{
    while(1)
    {
        sem_wait(&cleaning_mode);
        int* temp= (int*)arg;
        int id = *temp;
        
        for(int i=0;i<n;i++)
        {
            sem_wait(&room_access[i]);
            if(rooms[i].total_occupants>=2)
            {
                sem_wait(&print_access);
                cout<<"Cleaning staff "<<id<<" is cleaning room "<<i<<"."<<endl;
                sem_wait(&print_access);
                sleep(rooms[i].occupied_time);
                rooms[i].total_occupants=0;
                rooms[i].occupied_time=0;
                sem_wait(&print_access);
                cout<<"Cleaning staff "<<id<<" has cleaned room "<<i<<"."<<endl;
                sem_post(&print_access);
                sem_post(&room_access[i]);
                break;
            }
            sem_post(&room_access[i]);
        }

        sem_wait(&allotment_possible_access);
        allotment_possible++;
        sem_post(&allotment_possible_access);

    }
}

int main()
{
    cout << "Enter the number of rooms: ";
    cin >> n;
    cout << "Enter the number of guests: ";
    cin >> y;
    cout << "Enter the number of cleaning staff: ";
    cin >> x;

    rooms = new room[n];
    guest_priority = new int[y];
    guest_removed_status = new int[y];
    for(int i = 0; i < y; i++)
    {
        guest_removed_status[i] = 0;
    }

    vector<pair<int, int>> temp_priority;

    for (int i = 0; i < y; i++)
    {
        temp_priority.push_back(make_pair(rand(), i));
    }
    sort(temp_priority.begin(), temp_priority.end(), greater<pair<int, int>>());
    for (int i = 0; i < y; i++)
    {
        guest_priority[temp_priority[i].second] = i + 1;
    }

    room_access = new sem_t[n];
    for(int i=0;i<n;i++)
    {
        sem_init(&room_access[i],0,1);
    }

    guest_removed_status_access = new sem_t[y];
    for(int i=0;i<y;i++)
    {
        sem_init(&guest_removed_status_access[i],0,1);
    }

    sem_init(&total_guests_curr_access,0,1);
    sem_init(&rooms_with_2_occupants_access,0,1);
    sem_init(&cleaning_mode,0,0);
    sem_init(&allotment_possible_access,0,1);
    sem_init(&print_access,0,1);

    pthread_t guests[y];

    int guest_id[y];
    for (int i = 0; i < y; i++)
    {
        guest_id[i] = i;
        pthread_create(&guests[i], NULL,guest_thread_fn, (void *)&guest_id[i]);
    }

    pthread_t cleaning_staff[x];

    int cleaning_staff_id[x];
    for (int i = 0; i < x; i++)
    {
        cleaning_staff_id[i] = i;
        pthread_create(&cleaning_staff[i], NULL,cleaning_staff_thread_fn, (void *)&cleaning_staff_id[i]);
    }

    for (int i = 0; i < y; i++)
    {
        pthread_join(guests[i], NULL);
    }

    for (int i = 0; i < x; i++)
    {
        pthread_join(cleaning_staff[i], NULL);
    }

    return 0;

}