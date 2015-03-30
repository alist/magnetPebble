#include <pebble.h>
#include "knockDetector.h"

/*  Even though these might be more useful as long doubles, POSIX requires
    that they be double-precision literals.                                   */
#define M_PI        3.14159265358979323846264338327950288   /* pi            */


time_t knockDectionStartTimeSeconds;
uint16_t knockDectionStartTimeMiliSeconds;

time_t lastKnockTimeSeconds;
uint16_t lastKnockTimeMiliSeconds;


uint16_t logIndex;

hpf alg;

HTKnockHandler knockDetectedHandler;

//we can use this one to reduce power consumption by only triggering realtime updates after the first pebble 
//void pebbleTapDetected(AccelAxisType axis, int32_t direction); 

static uint32_t ms_since_reference_date (time_t refseconds, uint16_t refms){
	time_t currentSeconds = 0;
	uint16_t currentMS = 0;
	time_ms(&currentSeconds, &currentMS);

	uint32_t returnVal = (uint32_t)((currentSeconds-refseconds)*1000 + (currentMS - refms));
	return returnVal;
}

static uint32_t detector_MS_since_start(void){

	uint32_t differenceMS = ms_since_reference_date(knockDectionStartTimeSeconds, knockDectionStartTimeMiliSeconds);

	return differenceMS;
}

static void processDataPoint(AccelData dataPoint){
	int16_t nextZ = (dataPoint.z + abs(dataPoint.y)*(dataPoint.z > 0? 1 : -1));
//first we apply algorithm
    alg.Xim1    = alg.Xi;
    alg.Xi      = nextZ;
    
    alg.Yim1    = alg.Yi;
    
    alg.Yi = alg.alpha*alg.Yim1 + alg.alpha*(alg.Xi-alg.Xim1);

    double newOutput = alg.Yi;

	time_t currentSeconds = 0;
	uint16_t currentMS = 0;
	time_ms(&currentSeconds, &currentMS);
	uint32_t msSinceLastKnock = ms_since_reference_date(lastKnockTimeSeconds, lastKnockTimeMiliSeconds);

 //    if (logIndex++, logIndex%100 == 0 || abs(newOutput) > 200){
	//   	APP_LOG(APP_LOG_LEVEL_DEBUG, "newOutput %i input %i time %u logIndex %i", (int)alg.Yi, (int)nextZ, (unsigned int)msSinceLastKnock, logIndex);
	// }

  if (abs(newOutput) > alg.minAccel*1000){
    if (msSinceLastKnock > (uint32_t)alg.minKnockSeparationMS){

		lastKnockTimeSeconds = currentSeconds;
		lastKnockTimeMiliSeconds = currentMS;

		knockDetectedHandler(detector_MS_since_start());

      	// APP_LOG(APP_LOG_LEVEL_DEBUG, "x %i y %i z %i, time %lu", dataPoint.x,dataPoint.y,dataPoint.z, (unsigned long)dataPoint.timestamp);
      	APP_LOG(APP_LOG_LEVEL_DEBUG, "newOutput %i input %i time %u", (int)alg.Yi, (int)nextZ, (unsigned int)msSinceLastKnock);
    }
  }
}

static void knock_detector_accel_update_timer_callback(void *data){
	AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
	accel_service_peek(&accel);

	processDataPoint(accel);

	app_timer_register(alg.delT*1000, knock_detector_accel_update_timer_callback, NULL);
}

static void start(void){
	time_ms(&knockDectionStartTimeSeconds, &knockDectionStartTimeMiliSeconds);

	accel_data_service_subscribe(0, NULL);
	app_timer_register(alg.delT*1000, knock_detector_accel_update_timer_callback, NULL);
}

static void stop(void){

}

void knock_detector_subscribe(HTKnockHandler handler){
	knockDetectedHandler = handler;
	if (knockDetectedHandler != NULL){
		start();
	}else{
		stop();
	}
}

void knock_detector_unsubscribe(void){
	if (knockDetectedHandler != NULL){
		knockDetectedHandler = NULL;
		
		stop();
	}
}

void knock_detector_setupAlgorithmForCutoffAndSampleInterval(double fc, double delT){
	alg.fc = fc;
    alg.delT = delT; 
    
    double RC = 1.0/(2*M_PI*fc);
    double alpha = RC/ (RC + delT);
    
    alg.alpha = alpha;
    
    alg.minAccel = 0.25f;
    alg.minKnockSeparationMS = 100; 
}


hpf knock_detector_get_algorithm(void){
	return alg;
}