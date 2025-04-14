#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>

#define MODLUS 2147483647
#define MULT1 24112
#define MULT2 26143
std::ifstream infile;
std::ofstream outfile;


/* Default seeds for all 100 streams */
static long zrng[] = {
    1,
    1973272912, 281629770, 20006270, 1280689831, 2096730329, 1933576050,
    913566091, 246780520, 1363774876, 604901985, 1511192140, 1259851944,
    824064364, 150493284, 242708531, 75253171, 1964472944, 1202299975,
    233217322, 1911216000, 726370533, 403498145, 993232223, 1103205531,
    762430696, 1922803170, 1385516923, 76271663, 413682397, 726466604,
    336157058, 1432650381, 1120463904, 595778810, 877722890, 1046574445,
    68911991, 2088367019, 748545416, 622401386, 2122378830, 640690903,
    1774806513, 2132545692, 2079249579, 78130110, 852776735, 1187867272,
    1351423507, 1645973084, 1997049139, 922510944, 2045512870, 898585771,
    243649545, 1004818771, 773686062, 403188473, 372279877, 1901633463,
    498067494, 2087759558, 493157915, 597104727, 1530940798, 1814496276,
    536444882, 1663153658, 855503735, 67784357, 1432404475, 619691088,
    119025595, 880802310, 176192644, 1116780070, 277854671, 1366580350,
    1142483975, 2026948561, 1053920743, 786262391, 1792203830, 1494667770,
    1923011392, 1433700034, 1244184613, 1147297105, 539712780, 1545929719,
    190641742, 1645390429, 264907697, 620389253, 1502074852, 927711160,
    364849192, 2049576050, 638580085, 547070247
};

/* Generate the next random number */
float lcgrand(int stream) {
    long zi, lowprd, hi31;

    zi = zrng[stream];
    lowprd = (zi & 65535) * MULT1;
    hi31 = (zi >> 16) * MULT1 + (lowprd >> 16);
    zi = ((lowprd & 65535) - MODLUS) +
         ((hi31 & 32767) << 16) + (hi31 >> 15);
    if (zi < 0) zi += MODLUS;

    lowprd = (zi & 65535) * MULT2;
    hi31 = (zi >> 16) * MULT2 + (lowprd >> 16);
    zi = ((lowprd & 65535) - MODLUS) +
         ((hi31 & 32767) << 16) + (hi31 >> 15);
    if (zi < 0) zi += MODLUS;

    zrng[stream] = zi;
    return (zi >> 7 | 1) / 16777216.0;
}

/* Set the current zrng for stream "stream" to zset */
void lcgrandst(long zset, int stream) {
    zrng[stream] = zset;
}

/* Return the current zrng for stream "stream" */
long lcgrandgt(int stream) {
    return zrng[stream];
}


int amount, bigs, smalls, initial_inv_level, inv_level;


float area_holding, area_shortage, holding_cost, incremental_cost, maxlag, minlag;
float mean_interdemand, prob_distrib_demand[26], setup_cost,
      shortage_cost;
float sim_time, time_last_event, time_next_event[5],
      total_ordering_cost;

int next_event_type;
int num_events, num_months, num_values_demand;


float expon(float mean)  {
    return -mean * log(lcgrand(1));
}

int random_integer(float prob_distrib[]) {
    int i;
    float u = lcgrand(1);
    for (i = 1; u >= prob_distrib[i]; ++i);
    return i;
}

float uniform(float a, float b) {
    return a + lcgrand(1) * (b - a);
}

void initialize() {
    sim_time = 0.0;
    inv_level = initial_inv_level;
    time_last_event = 0.0;
    total_ordering_cost = 0.0;
    area_holding = 0.0;
    area_shortage = 0.0;
    time_next_event[1] = 1.0e+30;
    time_next_event[2] = sim_time + expon(mean_interdemand);
    time_next_event[3] = num_months;
    time_next_event[4] = 0.0;
}

void processNextEvent()  
{
    int i;
    float min_time_next_event = 1.0e+29;
    next_event_type = 0;

    for (i = 1; i <= num_events; ++i) {
        if (time_next_event[i] < min_time_next_event) {
            min_time_next_event = time_next_event[i];
            next_event_type = i;
        }
    }

    if (next_event_type == 0) {
        outfile << "\nEvent list empty at time " << sim_time << "\n";
        exit(1);
    }

    sim_time = min_time_next_event;
}

void order_arrival() {
    inv_level += amount;
    time_next_event[1] = 1.0e+30;
}

void demand() {
    inv_level -= random_integer(prob_distrib_demand);
    time_next_event[2] = sim_time + expon(mean_interdemand);
}

void evaluateInventory() {
    if (inv_level < smalls) {
        amount = bigs - inv_level;
        total_ordering_cost += setup_cost + incremental_cost * amount;
        time_next_event[1] = sim_time + uniform(minlag, maxlag);
    }
    time_next_event[4] = sim_time + 1.0;
}

void generateReport() {
    float avg_holding_cost = holding_cost * area_holding / num_months;
    float avg_ordering_cost = total_ordering_cost / num_months;
    float avg_shortage_cost = shortage_cost * area_shortage / num_months;
    float avg_total_cost = avg_ordering_cost + avg_holding_cost + avg_shortage_cost;
    
    outfile << "(" << std::setw(2) << smalls << "," << std::setw(3) << bigs << ")" 
            << std::setw(15) << std::fixed << std::setprecision(2) << avg_total_cost
            << std::setw(20) << avg_ordering_cost 
            << std::setw(20) << avg_holding_cost 
            << std::setw(20) << avg_shortage_cost << "\n";
}

void update_time_avg_stats() {
    float time_since_last_event = sim_time - time_last_event;
    time_last_event = sim_time;
    if (inv_level < 0)
        area_shortage -= inv_level * time_since_last_event;
    else if (inv_level > 0)
        area_holding += inv_level * time_since_last_event;
}



int main() {
    int num_policies;

    infile.open("in.txt");
    outfile.open("out.txt");

    num_events = 4;

    infile >> initial_inv_level >> num_months >> num_policies >> num_values_demand
           >> mean_interdemand >> setup_cost >> incremental_cost >> holding_cost
           >> shortage_cost >> minlag >> maxlag;
    for (int i = 1; i <= num_values_demand; ++i)
        infile >> prob_distrib_demand[i];

    outfile << "------Single-Product Inventory System------\n\n";
    outfile << "Initial inventory level: " << initial_inv_level << " items\n\n";
    outfile << "Number of demand sizes: " << num_values_demand << "\n\n";
    outfile << "Distribution function of demand sizes: ";

    for (int i = 1; i <= num_values_demand; ++i)
        outfile << prob_distrib_demand[i] << " ";
        
    outfile << "\n\nMean inter-demand time: " << mean_interdemand << " months\n\n";
    outfile << "Delivery lag range: " << minlag << " to " << maxlag << " months\n\n";
    outfile << "Length of simulation: " << num_months << " months\n\n";
    outfile << "Costs:\n";
    outfile << "K = " << setup_cost << "\n";
    outfile << "i = " << incremental_cost << "\n";
    outfile << "h = " << holding_cost << "\n";
    outfile << "pi = " << shortage_cost << "\n\n";
    outfile << "Number of policies: " << num_policies << "\n\n";
    outfile << "Policies:\n";
    outfile << "--------------------------------------------------------------------------------------------------\n";
    outfile << " Policy        Avg_total_cost     Avg_ordering_cost      Avg_holding_cost     Avg_shortage_cost\n";
    outfile << "--------------------------------------------------------------------------------------------------\n";

    for (int i = 1; i <= num_policies; ++i) {
        infile >> smalls >> bigs;
        initialize();
        
        do {
            processNextEvent();
            update_time_avg_stats();
            switch (next_event_type) {
                case 1: order_arrival(); break;
                case 2: demand(); break;
                case 4: evaluateInventory(); break;
                case 3: generateReport(); break;
            }
        } while (next_event_type != 3);
    }

    outfile << "--------------------------------------------------------------------------------------------------\n";

    infile.close();
    outfile.close();
    return 0;
}

