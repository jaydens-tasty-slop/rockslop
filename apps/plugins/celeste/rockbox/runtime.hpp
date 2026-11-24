/* Bounded replacement for the calculator's heap-backed object vector. */
#pragma once
#include "api.h"
#define CE_OBJECTS 96
#define dbg_printf(...) ((void)0)
#define profiler_add(...) ((void)0)
#define profiler_end(...) ((void)0)
int rand();
static inline size_t strlen(const char *s){size_t n=0;while(s[n])++n;return n;}
static inline int abs(int v) {return v<0?-v:v;}
class Object;
struct ObjectList {
    Object *items[CE_OBJECTS];
    unsigned count;
    Object **begin(){return items;}
    Object **end(){return items+count;}
    unsigned size(){return count;}
    bool empty(){return count==0;}
    Object *operator[](unsigned n){return items[n];}
    void push_back(Object *p){if(count<CE_OBJECTS)items[count++]=p;else ce_fault=1;}
    void erase(Object **p){unsigned i=p-items;while(i+1<count){items[i]=items[i+1];++i;}if(count)--count;}
};
