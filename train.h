
#ifndef TRAINAUTOMATION_TRAIN_H
#define TRAINAUTOMATION_TRAIN_H

#include <pthread.h>
#include <stdbool.h>

typedef struct Station {
    pthread_mutex_t mutex;

    pthread_cond_t cond_passenger_boarded;
    pthread_cond_t cond_train_arrival;
    pthread_cond_t cond_station_platform;

    bool aTrainIsLoading;

    int currentTrainFreeSeats;
    int numWaitingPassengers;
    int numBoardedPassengers;

}Station;

void station_load_train(Station *station, int seatCount);

void station_wait_for_train(Station *station);

void station_on_board(Station *station);

void station_init(Station *station);

#endif //TRAINAUTOMATION_TRAIN_H
