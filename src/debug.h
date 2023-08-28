#pragma once

#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...) ;
#endif