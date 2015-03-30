#pragma once
  
typedef struct{
    double alpha;
    double Yi;
    double Yim1;
    double Xi;
    double Xim1;

    double delT; //seconds
    double fc; //cutoff frequency
    
    double minAccel; //in Gs
    int32_t minKnockSeparationMS; //in ms
}hpf;


//you should know 
//submit a feature request for proxying raw accel if you need it

typedef void (*HTKnockHandler)(uint32_t knockMSSinceBegin);

//adding a handler immediately begins detection
void knock_detector_subscribe(HTKnockHandler handler);

//removing handlers disables detection
//stops energy usage associated
void knock_detector_unsubscribe(void);

//see above for algorithm. 
//all zero values will be set to default values if not set before run
hpf knock_detector_get_algorithm(void);
void knock_detector_setupAlgorithmForCutoffAndSampleInterval(double fc, double delT);
// void knock_detector_set_algorithm(hpf filterData);

//C routine- callback multiple--can you do that with method signatures?