#include <pebble.h>
#include "knockDetector.h"

/*  Even though these might be more useful as long doubles, POSIX requires
    that they be double-precision literals.                                   */
#define M_PI        3.14159265358979323846264338327950288   /* pi            */


time_t knockDectionStartTimeSeconds;
uint16_t knockDectionStartTimeMiliSeconds;

time_t lastKnockTimeSeconds;
uint16_t lastKnockTimeMiliSeconds;

bool isRealtime = false;

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

static void stopRealtime(void);
static void knockTimeout(void){
  isRealtime = false; 
  stopRealtime();
}

static void extendKnockTimeOut(void){
  
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

	//it also seems most normal taps go into the device so && newOutput < 0
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

	if (isRealtime){
		app_timer_register(alg.delT*1000, knock_detector_accel_update_timer_callback, NULL);
	}
}

static void startRealtime(void){
	isRealtime = true;
	accel_tap_service_unsubscribe();
	
	accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ); //does this change anything?
	accel_data_service_subscribe(0, NULL); //sort don't know what this does

	app_timer_register(alg.delT*1000, knock_detector_accel_update_timer_callback, NULL);
}

void accelTapHappened(AccelAxisType axis, int32_t direction){
	if (isRealtime == false){
		light_enable_interaction();
		startRealtime();
	}
}

static void stopRealtime(void){
	isRealtime = false;

	accel_data_service_unsubscribe();
	accel_tap_service_subscribe(accelTapHappened);
}

static void start(void){
	accel_tap_service_subscribe(accelTapHappened);

	time_ms(&knockDectionStartTimeSeconds, &knockDectionStartTimeMiliSeconds);
}

static void stop(void){
	accel_tap_service_unsubscribe();
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
    
    alg.minAccel = 0.30f;
    alg.minKnockSeparationMS = 100; 
}


hpf knock_detector_get_algorithm(void){
	return alg;
}