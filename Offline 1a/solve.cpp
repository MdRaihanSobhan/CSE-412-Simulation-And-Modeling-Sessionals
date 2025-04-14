#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <iomanip>
#include <queue>

using namespace std;

const int MAX_QUEUE_SIZE = 100;
const int SERVER_BUSY = 1;
const int SERVER_IDLE = 0;

int nextEventType, totalCustomersDelayed, requiredDelays;
int totalEvents, queueSize, serverState;
float cumulativeQueueLength, cumulativeServerBusyTime, averageInterarrivalTime;
float averageServiceTime, currentTime;
float queueEntryTimes[MAX_QUEUE_SIZE + 1]; 
float lastEventTime, nextEventTimes[3]; 
float totalDelayTime;
ifstream inputFile;
ofstream outputFile, eventLogFile;
int currentCustomerID = 0;
queue<int> customerQueue;

void handleCustomerArrival();
void handleCustomerDeparture();

float generateRandomNumber(int streamID) {
    const long multiplier = 16807;
    const long modulus = 2147483647;
    static long seed[6] = {1973272912, 281629770, 20006270, 1280689831, 2096730329, 1933576050};
    long quotient = modulus / multiplier;
    long remainder = modulus % multiplier;
    long term;

    term = multiplier * (seed[streamID] % quotient) - remainder * (seed[streamID] / quotient);
    if (term > 0) 
        seed[streamID] = term;
    else 
        seed[streamID] = term + modulus;
    return ((float) seed[streamID] / modulus);
}

float generateExponentialVariate(float mean) {
    return -mean * log(generateRandomNumber(1));
}

void initializeSimulation() {
    currentTime = 0.0;

    serverState = SERVER_IDLE;
    queueSize = 0;
    lastEventTime = 0.0;

    totalCustomersDelayed = 0;
    totalDelayTime = 0.0;
    cumulativeQueueLength = 0.0;
    cumulativeServerBusyTime = 0.0;

    nextEventTimes[1] = currentTime + generateExponentialVariate(averageInterarrivalTime);
    nextEventTimes[2] = numeric_limits<float>::max();
}



void updateAverageStats() {
    float timeSinceLastEvent;

    timeSinceLastEvent = currentTime - lastEventTime;
    lastEventTime = currentTime;

    cumulativeQueueLength += queueSize * timeSinceLastEvent;

    cumulativeServerBusyTime += serverState * timeSinceLastEvent;
}


void logSimulationEvent(int customerID, const string& eventType) {
    static int eventCount = 0;

    if (eventType == "Arrival") {
        eventCount++;
        eventLogFile << setw(1) << eventCount << ". Next event: Customer " 
                << customerID << " " << eventType << endl;
        eventLogFile << "-No. of customers delayed: " << totalCustomersDelayed << "-" << endl;
    } else {
        eventCount++;
        eventLogFile << setw(1) << eventCount << ". Next event: Customer " 
                << customerID << " " << eventType << endl;
    }
}



void handleCustomerArrival() {
    float delay;

    currentCustomerID++;

    nextEventTimes[1] = currentTime + generateExponentialVariate(averageInterarrivalTime);

    if (serverState == SERVER_BUSY) {
        ++queueSize;

        if (queueSize > MAX_QUEUE_SIZE) {
            outputFile << "\nOverflow of the array time_arrival at";
            outputFile << " time " << currentTime;
            exit(2);
        }

        queueEntryTimes[queueSize] = currentTime;
        customerQueue.push(currentCustomerID);
    }
    else {
        delay = 0.0;
        totalDelayTime += delay;

        serverState = SERVER_BUSY;

        nextEventTimes[2] = currentTime + generateExponentialVariate(averageServiceTime);
        customerQueue.push(currentCustomerID);
    }

    ++totalCustomersDelayed;

    logSimulationEvent(currentCustomerID, "Arrival");
}

void handleCustomerDeparture() {
    int i;
    float delay;

    if (!customerQueue.empty()) {
        int departingCustomer = customerQueue.front();
        customerQueue.pop();
        logSimulationEvent(departingCustomer, "Departure");

        if (customerQueue.empty()) {
            serverState = SERVER_IDLE;
            nextEventTimes[2] = numeric_limits<float>::max();
        }
        else {
            nextEventTimes[2] = currentTime + generateExponentialVariate(averageServiceTime);
        }

        if (queueSize > 0) {
            --queueSize;

            delay = currentTime - queueEntryTimes[1];
            totalDelayTime += delay;

            for (i = 1; i <= queueSize; ++i)
                queueEntryTimes[i] = queueEntryTimes[i + 1];
        }
    }
}

void processNextEvent() {
    int i;
    float minNextEventTime = numeric_limits<float>::max();

    nextEventType = 0;

    for (i = 1; i <= totalEvents; ++i) {
        if (nextEventTimes[i] < minNextEventTime) {
            minNextEventTime = nextEventTimes[i];
            nextEventType = i;
        }
    }

    if (serverState == SERVER_BUSY && nextEventType == 1 && nextEventTimes[2] < numeric_limits<float>::max()) {
        nextEventType = 2;
        minNextEventTime = nextEventTimes[2];
    }

    if (nextEventType == 0) {
        outputFile << "\nEvent list empty at time " << currentTime;
        exit(1);
    }

    currentTime = minNextEventTime;
}


void generateSimulationReport() {
    outputFile << "Average delay in queue " << totalDelayTime / totalCustomersDelayed << " minutes\n\n";
    outputFile << "Average number in queue " << cumulativeQueueLength / currentTime << endl << endl;
    outputFile << "Server utilization " << cumulativeServerBusyTime / currentTime << endl << endl;
    outputFile << "Time simulation ended " << currentTime << " minutes";
}


int main() {
    inputFile.open("mm1.in");
    outputFile.open("mm1.out");
    eventLogFile.open("mm1_log.out");

    totalEvents = 2;

    inputFile >> averageInterarrivalTime >> averageServiceTime >> requiredDelays;

    outputFile << "Single-server queueing system\n\n";
    outputFile << "Mean interarrival time " << averageInterarrivalTime << " minutes\n\n";
    outputFile << "Mean service time " << averageServiceTime << " minutes\n\n";
    outputFile << "Number of customers " << requiredDelays << endl << endl;

    initializeSimulation();

    while (totalCustomersDelayed < requiredDelays) {
        processNextEvent();

        updateAverageStats();

        switch (nextEventType) {
            case 1:
                handleCustomerArrival();
                break;
            case 2:
                handleCustomerDeparture();
                break;
        }
    }

    generateSimulationReport();

    inputFile.close();
    outputFile.close();
    eventLogFile.close();

    return 0;
}



