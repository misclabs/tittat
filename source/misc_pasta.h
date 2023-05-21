#pragma once

typedef unsigned char MpByte;

#define MpArrayLen(array) (sizeof(array)/sizeof((array)[0]))

float MpClampF(float val, float min, float max);
int MpClampI(int val, int min, int max);
