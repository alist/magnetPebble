#include <pebble.h>
#include "knockDetector.h"

/*  Even though these might be more useful as long doubles, POSIX requires
    that they be double-precision literals.                                   */
#define M_PI        3.14159265358979323846264338327950288   /* pi            */


time_t knockDectionStartTime;
NSTimeInterval lastKnock;

hpf alg;

HTKnockHandler knockDetectedHandler;

//we can use this one to reduce power consumption by only triggering realtime updates after the first pebble 
//void pebbleTapDetected(AccelAxisType axis, int32_t direction); 

void setupAlgorithmForCutoffAndSampleInterval(double fc, double delT){
	alg.fc = fc;
    alg.delT = delT; 
    
    double RC = 1.0/(2*M_PI*fc);
    double alpha = RC/ (RC + delT);
    
    alg.alpha = alpha;
    
    alg.minAccel = 0.75f;
    alg.minKnockSeparation = 0.1f;
}


hpf knock_detector_get_algorithm(void){
	return alg;
}