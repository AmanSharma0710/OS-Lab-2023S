#include "header.h"

int x, y, n;
room *rooms;
int *guest_priority;
int *guest_removed_status;
int total_guests_curr = 0;

int rooms_with_2_occupants = 0; //used to trigger cleaning mode
int allotment_possible=1; //not in cleaning mode

sem_t *room_access; //ability to access room's data
sem_t *room_busy; //room is being stayed in or being cleaned, hence busy
sem_t *guest_removed_status_access;
sem_t total_guests_curr_access;
sem_t rooms_with_2_occupants_access;
sem_t cleaning_mode;
sem_t allotment_possible_access;
sem_t print_access;

void* guest_thread_fn(void *arg);

void* cleaning_staff_thread_fn(void *arg);

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

    // print the priority of the guests
    cout << "Guest priority: ";
    for (int i = 0; i < y; i++)
    {
        cout << guest_priority[i] << " ";
    }
    cout << endl;

    room_access = new sem_t[n];
    for(int i=0;i<n;i++)
    {
        sem_init(&room_access[i],0,1);
    }

    room_busy = new sem_t[n];
    for(int i=0;i<n;i++)
    {
        sem_init(&room_busy[i],0,1);
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
