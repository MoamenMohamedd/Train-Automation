
// #define _GNU_SOURCE is needed for the resolution of the following warnings
//warning: implicit declaration of function ‘pthread_setname_np’ [-Wimplicit-function-declaration]
//warning: implicit declaration of function ‘pthread_getname_np’ [-Wimplicit-function-declaration]

#define _GNU_SOURCE
#define THREAD_NAME_LENGTH 16

#include "train.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TrainData {
    Station *trainStation;
    int numberOfFreeSeats;
}TrainData;


void *passengerSubRoutine(void *args) {
    Station* trainStation = (Station*)args;

    //wait until a train comes and there is a free seat
    station_wait_for_train(trainStation);

    //the passenger robot moves the passenger on board the train and into a seat
    trainStation->numBoardedPassengers++;
    trainStation->numWaitingPassengers--;
    trainStation->currentTrainFreeSeats--;

    //let the train know that passenger is on board
    station_on_board(trainStation);

    //unlock mutex after the passenger is onboard
    pthread_mutex_unlock(&trainStation->mutex);

    pthread_exit(NULL);
}

void *trainSubRoutine(void *args) {
    TrainData *trainData = (TrainData *) args;
    Station* trainStation = trainData->trainStation;

    station_load_train(trainStation, trainData->numberOfFreeSeats);

    char trainName[THREAD_NAME_LENGTH];
    pthread_getname_np(pthread_self(), trainName, THREAD_NAME_LENGTH);
    printf("\n%s left the station with %d passengers onboard and %d waiting passengers\n", trainName,
           trainStation->numBoardedPassengers, trainStation->numWaitingPassengers);

    trainStation->numBoardedPassengers = 0;
    trainStation->aTrainIsLoading = false;

    pthread_cond_signal(&trainStation->cond_station_platform);

    pthread_mutex_unlock(&trainStation->mutex);
    pthread_exit(NULL);
}

int main() {

    int numPassengers = 0 ,numTrains = 0 ,num;
    char scenario[100] , command;

    pthread_t *trainIds = (pthread_t*)malloc(0) , *passengerIds = (pthread_t*)malloc(0);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    Station *kingsCrossStation = (Station *) malloc(sizeof(Station));
    station_init(kingsCrossStation);

//    printf("Enter a scenario : ");
    fgets(scenario, sizeof(scenario),stdin);
//    char test[100] = "10p 30t 20p";
//    char test[100] = "10p 30t 50p 30t 20t";
//    char test[100] = "10p 30t 50p 30t";
//    char test[100] = "10p 30t 50p 30t";

    char * pch;
    pch = strtok (scenario," ");
    while (pch != NULL)
    {

        sscanf(pch,"%d%c",&num,&command);

        switch (command){
            case 'p':
                passengerIds = (pthread_t*)realloc(passengerIds,sizeof(pthread_t) * (numPassengers + num));
                for (int i = numPassengers; i < numPassengers+num; i++) {

                    pthread_create(&passengerIds[i], &attr, passengerSubRoutine, (void *) kingsCrossStation);

                    char passengerName[THREAD_NAME_LENGTH];
                    sprintf(passengerName, "passenger-%d", (i + 1));
                    pthread_setname_np(passengerIds[i], passengerName);
                }

                numPassengers += num;
                break;
            case 't':
                trainIds = (pthread_t*)realloc(trainIds,sizeof(pthread_t) * (numTrains + 1));
                TrainData *trainData = (TrainData*)malloc(sizeof(TrainData));
                trainData->trainStation = kingsCrossStation;
                trainData->numberOfFreeSeats = num;

                pthread_create(&trainIds[numTrains], &attr, trainSubRoutine, (void *) trainData);

                char trainName[THREAD_NAME_LENGTH];
                sprintf(trainName, "train-%d", (numTrains + 1));
                pthread_setname_np(trainIds[numTrains], trainName);

                numTrains++;
                break;
            default:
                printf("Error");
                exit(0);
        }

        pch = strtok (NULL, " ");
    }



    pthread_attr_destroy(&attr);

    for (int i = 0; i < numPassengers; i++) {
        pthread_join(passengerIds[i], NULL);
    }

    for (int i = 0; i < numTrains; i++) {
        pthread_join(trainIds[i], NULL);
    }

    pthread_mutex_destroy(&kingsCrossStation->mutex);
    pthread_cond_destroy(&kingsCrossStation->cond_station_platform);
    pthread_cond_destroy(&kingsCrossStation->cond_train_arrival);
    pthread_cond_destroy(&kingsCrossStation->cond_passenger_boarded);

    pthread_exit(NULL);
}
