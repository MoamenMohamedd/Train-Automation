// #define _GNU_SOURCE is needed for the resolution of the following warnings
//warning: implicit declaration of function ‘pthread_setname_np’ [-Wimplicit-function-declaration]
//warning: implicit declaration of function ‘pthread_getname_np’ [-Wimplicit-function-declaration]

#define _GNU_SOURCE
#define THREAD_NAME_LENGTH 12

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <memory.h>

struct station {
    pthread_mutex_t mutex;

    pthread_cond_t trainFull;
    pthread_cond_t trainArrival;

    int numWaitingPassengers;
    int numBoardedPassengers;
    int currentTrainSeatCount;
};

struct trainData{
    struct station* trainStation;
    int numberOfFreeSeats;
};


void station_load_train(struct station *station, int seatCount);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);

void station_init(struct station *station);

void *passengerSubRoutine(void *args);

void *trainSubRoutine(void *args);


int numDepartedTrains = 0;

int main() {

//    (rand() % 5)+1;

    int i;
    pthread_t trainIds[2];
    pthread_t passengerIds[70];

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    struct station *kingsCrossStation = (struct station *) malloc(sizeof(struct station));

    station_init(kingsCrossStation);

    for (i = 0; i < 10; i++) {
        pthread_create(&passengerIds[i], &attr, passengerSubRoutine, (void *) kingsCrossStation);
    }

    struct trainData* trainData1 = (struct trainData*)malloc(sizeof(struct trainData));
    trainData1->trainStation = kingsCrossStation;
    trainData1->numberOfFreeSeats = 30;

    pthread_create(&trainIds[0], &attr, trainSubRoutine, (void *) trainData1);

    for (i = 0; i < 60; i++) {
        pthread_create(&passengerIds[i], &attr, passengerSubRoutine, (void *) kingsCrossStation);
    }

    struct trainData* trainData2 = (struct trainData*)malloc(sizeof(struct trainData));
    trainData1->trainStation = kingsCrossStation;
    trainData2->numberOfFreeSeats = 30;

    pthread_create(&trainIds[1], &attr, trainSubRoutine, (void *) trainData2);




    pthread_attr_destroy(&attr);

    for (i = 0; i < 60; i++) {
        pthread_join(passengerIds[i], NULL);
    }
    pthread_join(trainIds[0], NULL);
    pthread_join(trainIds[1], NULL);


    pthread_mutex_destroy(&kingsCrossStation->mutex);
    pthread_cond_destroy(&kingsCrossStation->trainFull);
    pthread_cond_destroy(&kingsCrossStation->trainArrival);

    pthread_exit(NULL);

}

void station_init(struct station *station) {
    pthread_mutex_init(&station->mutex, NULL);

    pthread_cond_init(&station->trainFull, NULL);
    pthread_cond_init(&station->trainArrival, NULL);

    station->numWaitingPassengers = 0;
    station->numBoardedPassengers = 0;
}

void station_load_train(struct station *station, int seatCount) {
    pthread_mutex_lock(&station->mutex);

    pthread_cond_broadcast(&station->trainArrival);

    station->currentTrainSeatCount = seatCount;
    pthread_cond_wait(&station->trainFull, &station->mutex);

    char trainName[THREAD_NAME_LENGTH];

    pthread_getname_np(pthread_self(), trainName, THREAD_NAME_LENGTH);

    printf("train %s left the station with %d passengers onboard", trainName , station->numBoardedPassengers);

    station->numBoardedPassengers = 0;

    pthread_mutex_unlock(&station->mutex);
    pthread_exit(NULL);

}

void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&station->mutex);

    station->numWaitingPassengers++;

    pthread_cond_wait(&station->trainArrival, &station->mutex);

    station_on_board(station);

    pthread_mutex_unlock(&station->mutex);
    pthread_exit(NULL);
}

void station_on_board(struct station *station) {
    station->numBoardedPassengers++;
    station->numWaitingPassengers--;

    char passengerName[THREAD_NAME_LENGTH];

    pthread_getname_np(pthread_self(), passengerName, THREAD_NAME_LENGTH);

    printf("passenger %s has boarded the train, %d passengers onboard ", passengerName, station->numBoardedPassengers);

    if (station->numBoardedPassengers == station->currentTrainSeatCount || station->numWaitingPassengers == 0)
        pthread_cond_signal(&station->trainFull);

}

void *passengerSubRoutine(void *args) {
    station_wait_for_train(args);
}

void *trainSubRoutine(void *args) {
    struct trainData* data = (struct trainData*)args;

    station_load_train(data->trainStation , data->numberOfFreeSeats);
}
