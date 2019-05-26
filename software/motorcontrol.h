#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

#include "826api.h"
#include <cmath>

bool connectToS826();
void disconnectFromS826();
bool initMotor(uint channel);
bool initEncod(uint channel);
bool checkEncod(uint channel);
void setVolts(uint channel, double V);
void setTorque(uint channel, double T);
int setCounts(uint channel, uint counts);
int getCounts(uint channel);
double getAngle(uint channel, int prev);

#endif // MOTORCONTROL_H
