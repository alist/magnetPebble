#pragma once

#define NSTimeInterval double
  
typedef struct{
    double alpha;
    double Yi;
    double Yim1;
    double Xi;
    double Xim1;

    double delT;
    double fc; //cutoff frequency
    
    double minAccel;
    NSTimeInterval minKnockSeparation;
    NSTimeInterval lastKnock;
}hpf;
