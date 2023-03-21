#ifndef HEADER_H
#define HEADER_H

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
    int total_occupants = 0; //total number of guests who have stayed in the room since last cleaning
    int occupied_time = 0; //total time room occupied since last cleaning
} room;

#endif