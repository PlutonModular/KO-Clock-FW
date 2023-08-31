#pragma once
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define clamp(a,b,c) (min(max((a),(b)),(c)))
#define lerp(a,b,f) ((a) + (f) * ((b) - (a)))