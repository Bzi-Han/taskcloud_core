#ifndef GLOBAL_H // !GLOBAL_H
#define GLOBAL_H

#include "ThreadPool.h"

extern ThreadPool g_threadPool;

extern const char *g_serviceAddress;
extern uint16_t g_servicePort;
extern const char *g_serviceKey;

#endif // !GLOBAL_H