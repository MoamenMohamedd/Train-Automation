// #define _GNU_SOURCE is needed for the resolution of the following warnings
//warning: implicit declaration of function ‘pthread_setname_np’ [-Wimplicit-function-declaration]
//warning: implicit declaration of function ‘pthread_getname_np’ [-Wimplicit-function-declaration]

#define _GNU_SOURCE
#define THREAD_NAME_LENGTH 16

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

struct station {
    pthread_mutex_t mutex;

    pthread_cond_t trainFull; //trains wait for this condition while passengers are boarding
    pthread_cond_t trainArrival; //passengers wait for this condition until a train arrives
    pthread_cond_t stationAvailable; //trains wait for other trains until they leave

    bool stationAvailableForTrainEntry;

    int numWaitingPassengers;
    int numBoardedPassengers;
    int currentTrainSeatCount;
};

struct trainData {
    struct station *trainStation;
    int numberOfFreeSeats;
};


void station_load_train(struct station *station, int seatCount);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);

void station_init(struct station *station);

void *passengerSubRoutine(void *args);

void *trainSubRoutine(void *args);


int main() {

//    (rand() % 5)+1;

    printf("dfdfd");

    int i;

    pthread_t trainIds[3];
    pthread_t passengerIds[70];

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    struct station *kingsCrossStation = (struct station *) malloc(sizeof(struct station));

    station_init(kingsCrossStation);


    //generate scenario 10 passengers then 1 train 30 seats then 60 passengers then 2 trains each 30 seats
    for (i = 0; i < 10; i++) {
        pthread_create(&passengerIds[i], &attr, passengerSubRoutine, (void *) kingsCrossStation);

        char passengerName[THREAD_NAME_LENGTH];

        sprintf(passengerName, "passenger-%d", (i + 1));

        pthread_setname_np(passengerIds[i], passengerName);
    }

    struct trainData *trainData1 = (struct trainData *) malloc(sizeof(struct trainData));
    trainData1->trainStation = kingsCrossStation;
    trainData1->numberOfFreeSeats = 30;

    pthread_create(&trainIds[0], &attr, trainSubRoutine, (void *) trainData1);

    pthread_setname_np(trainIds[0], "train-1");

    for (; i < 60; i++) {
        pthread_create(&passengerIds[i], &attr, passengerSubRoutine, (void *) kingsCrossStation);

        char passengerName[THREAD_NAME_LENGTH];

        sprintf(passengerName, "passenger-%d", (i + 1));

        pthread_setname_np(passengerIds[i], passengerName);
    }

    struct trainData *trainData2 = (struct trainData *) malloc(sizeof(struct trainData));
    trainData2->trainStation = kingsCrossStation;
    trainData2->numberOfFreeSeats = 30;

    pthread_create(&trainIds[1], &attr, trainSubRoutine, (void *) trainData2);

    pthread_setname_np(trainIds[1], "train-2");

    struct trainData *trainData3 = (struct trainData *) malloc(sizeof(struct trainData));
    trainData3->trainStation = kingsCrossStation;
    trainData3->numberOfFreeSeats = 30;

    pthread_create(&trainIds[2], &attr, trainSubRoutine, (void *) trainData3);

    pthread_setname_np(trainIds[2], "train-3");


    pthread_attr_destroy(&attr);

    for (i = 0; i < 60; i++) {
        pthread_join(passengerIds[i], NULL);
    }
    pthread_join(trainIds[0], NULL);
    pthread_join(trainIds[1], NULL);
    pthread_join(trainIds[2], NULL);


    pthread_mutex_destroy(&kingsCrossStation->mutex);
    pthread_cond_destroy(&kingsCrossStation->trainFull);
    pthread_cond_destroy(&kingsCrossStation->trainArrival);
    pthread_cond_destroy(&kingsCrossStation->stationAvailable);

    pthread_exit(NULL);

}

void station_init(struct station *station) {
    pthread_mutex_init(&station->mutex, NULL);

    pthread_cond_init(&station->trainFull, NULL);
    pthread_cond_init(&station->trainArrival, NULL);
    pthread_cond_init(&station->stationAvailable, NULL);

    station->numWaitingPassengers = 0;
    station->numBoardedPassengers = 0;
    station->currentTrainSeatCount = 0;
    station->stationAvailableForTrainEntry = true;
}

void station_load_train(struct station *station, int seatCount) {

    pthread_mutex_lock(&station->mutex);

//    //check if there is no train currently loading in the station
//    while (station->stationAvailableForTrainEntry == false) {
//        //if station is occupied then wait until it is empty
//        pthread_cond_wait(&station->stationAvailable, &station->mutex);
//    }


    //if it is available then start loading passengers
    char trainName[THREAD_NAME_LENGTH];
    pthread_getname_np(pthread_self(), trainName, THREAD_NAME_LENGTH);

    printf("\n%s is loading", trainName);

    //update available free seats
    station->currentTrainSeatCount = seatCount;

    //signal to passengers that a train has come
    pthread_cond_broadcast(&station->trainArrival);
    //prevent trains from entering the station
    station->stationAvailableForTrainEntry = false;
    //wait till the train can leave
    while (station->currentTrainSeatCount != station->numBoardedPassengers && station->numWaitingPassengers > 0) {
        pthread_cond_wait(&station->trainFull, &station->mutex);
    }


    printf("\n%s left the station with %d passengers onboard and %d waiting passengers", trainName,
           station->numBoardedPassengers, station->numWaitingPassengers);

    station->numBoardedPassengers = 0;
    station->stationAvailableForTrainEntry = true;
    pthread_cond_signal(&station->stationAvailable);

    pthread_mutex_unlock(&station->mutex);
    pthread_exit(NULL);
}

void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&station->mutex);

//    station->numWaitingPassengers++;
//    pthread_cond_wait(&station->trainArrival, &station->mutex);
//
//    if (station->numBoardedPassengers < station->currentTrainSeatCount){
//        station->numBoardedPassengers++;
//        station->numWaitingPassengers--;
//
//        station_on_board(station);
//    } else{
//        pthread_cond_signal(&station->trainFull);
//    }

    //wait for train
    station->numWaitingPassengers++;
    while (station->stationAvailableForTrainEntry){
        pthread_cond_wait(&station->trainArrival, &station->mutex);
        //train arrived first check if there is an available seat
        if (station->numBoardedPassengers < station->currentTrainSeatCount){
            station->numBoardedPassengers++;
            station->numWaitingPassengers--;

            //if there is one then board the train
            station_on_board(station);

            //if this was the last waiting passenger
            if (station->numWaitingPassengers == 0)
                //signal the train to move
                pthread_cond_signal(&station->trainFull);

            break;
        }else{
            //train is full signal the train to leave and loop wait for the train to arrive
            pthread_cond_signal(&station->trainFull);
        }
    }

    pthread_mutex_unlock(&station->mutex);
    pthread_exit(NULL);
}

//passenger boards the train
void station_on_board(struct station *station) {

    char passengerName[THREAD_NAME_LENGTH];

    pthread_getname_np(pthread_self(), passengerName, THREAD_NAME_LENGTH);

    printf("\n%s has boarded the train, %d passengers onboard ", passengerName, station->numBoardedPassengers);

}

void *passengerSubRoutine(void *args) {
    station_wait_for_train((struct station*)args);
}

void *trainSubRoutine(void *args) {
    struct trainData *data = (struct trainData *) args;

    station_load_train(data->trainStation, data->numberOfFreeSeats);
}
