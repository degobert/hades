/*************************************************************************
  > File Name   : dispatch_common.h
  > Author      : dh
  > Mail        : dh03@163.com
  > Created Time: Wed 07 Jun 2017 04:35:48 PM CST
  > Version     : 1.0.0
 ************************************************************************/
#ifndef CLOUDAIR_DISPATCHER_COMMON_H
#define CLOUDAIR_DISPATCHER_COMMON_H
#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <set>
#include <map>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DISPATCHER_BS namespace cloudair {

#define DISPATCHER_ES }

#define DISPACTHER_US using namespace cloudair;


#define DELETE_AND_SET_NULL(X)  \
    do{                         \
        if(x){                  \
            delete x;           \
            x = NULL;           \
        }                       \
    }while(0)

#endif



