// #define _GNU_SOURCE is needed for the resolution of the following warnings
//warning: implicit declaration of function ‘pthread_setname_np’ [-Wimplicit-function-declaration]
//warning: implicit declaration of function ‘pthread_getname_np’ [-Wimplicit-function-declaration]

#define _GNU_SOURCE
#define THREAD_NAME_LENGTH 16

#include "train.h"
#include <stdio.h>
#include <stdlib.h>

void station_init(Station *station) {
    pthread_mutex_init(&station->mutex, NULL);

    pthread_cond_init(&station->cond_station_platform, NULL);
    pthread_cond_init(&station->cond_train_arrival, NULL);
    pthread_cond_init(&station->cond_passenger_boarded, NULL);

    station->aTrainIsLoading = false;
    station->numWaitingPassengers = 0;
    station->numBoardedPassengers = 0;
    station->currentTrainFreeSeats = 0;
}

void station_load_train(Station *station, int seatCount) {
    pthread_mutex_lock(&station->mutex);

    while (station->aTrainIsLoading)
        pthread_cond_wait(&station->cond_station_platform, &station->mutex);

    char trainName[THREAD_NAME_LENGTH];
    pthread_getname_np(pthread_self(), trainName, THREAD_NAME_LENGTH);
    printf("\n%s is loading", trainName);
    printf("\n-----------------------------------");

    station->aTrainIsLoading = true;

    //update available free seats
    station->currentTrainFreeSeats = seatCount;

    pthread_cond_broadcast(&station->cond_train_arrival);

    //wait until all passengers are in their seats, and
    //either the train is full or all waiting passengers have boarded
    while (station->currentTrainFreeSeats > 0 && station->numWaitingPassengers != 0)
        pthread_cond_wait(&station->cond_passenger_boarded, &station->mutex);

}

void station_wait_for_train(Station *station) {
    pthread_mutex_lock(&station->mutex);

    //wait until a train is loading and the passenger can be seated
    station->numWaitingPassengers++;
    while (!station->aTrainIsLoading || station->currentTrainFreeSeats == 0)
        pthread_cond_wait(&station->cond_train_arrival, &station->mutex);

}

void station_on_board(Station *station) {

    char passengerName[THREAD_NAME_LENGTH];
    pthread_getname_np(pthread_self(), passengerName, THREAD_NAME_LENGTH);
    printf("\n%s is seated in the train, %d passengers onboard ", passengerName, station->numBoardedPassengers);

    //notify train that a passenger has boarded (is seated)
    pthread_cond_signal(&station->cond_passenger_boarded);

}

