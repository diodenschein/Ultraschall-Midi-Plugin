//
//  Helpers.cpp
//  reaper_csurf_fpxt
//
//  Created by Robert Nixdorf on 7.12.2016.
//  Copyright (c) 2016 . All rights reserved.
//

#include "Helpers.h"
#include "csurf.h"


double charToVol(unsigned char val)
{
    double pos=((double)val*1000.0)/127.0;
    pos=SLIDER2DB(pos);
    return DB2VAL(pos);
    
}

unsigned char volToChar(double vol)
{
    double d=(DB2SLIDER(VAL2DB(vol))*127.0/1000.0);
    if (d<0.0)d=0.0;
    else if (d>127.0)d=127.0;
    
    return (unsigned char)(d+0.5);
}

double charToPan(unsigned char val)
{
    double pos=((double)val*1000.0+0.5)/127.0;
    
    pos=(pos-500.0)/500.0;
    if (fabs(pos) < 0.08) pos=0.0;
    
    return pos;
}

unsigned char panToChar(double pan)
{
    pan = (pan+1.0)*63.5;
    
    if (pan<0.0)pan=0.0;
    else if (pan>127.0)pan=127.0;
    
    return (unsigned char)(pan+0.5);
}
