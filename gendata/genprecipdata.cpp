/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Climatology Plugin
 * Author:   Sean D'Epagnier
 *
 ***************************************************************************
 *   Copyright (C) 2016 by Sean D'Epagnier                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

/* This program takes the precipitation data rom

   ftp://ftp.cdc.noaa.gov/Datasets/cmap/std/precip.mon.mean.nc
   to produce a much condensed binary file to be compressed
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <string.h>

#include <netcdfcpp.h>

const char* precippath = "precip.mon.mean.nc";
#define LATS 72
#define LONS 144

int main(int argc, char *argv[])
{
    NcFile precip(precippath, NcFile::ReadOnly);
    if(!precip.is_valid() || precip.num_dims() != 3 || precip.num_vars() != 4) {
        fprintf(stderr, "failed reading file: %s\n", precippath);
        return 1;
    }

    NcVar* data = precip.get_var("precip");
    if(!data->is_valid() || data->num_dims() != 3) {
        fprintf(stderr, "precip has incorrect dimensions");
        return 1;
    }

    NcDim* time = data->get_dim(0);
    int timecnt = time->size();
    float  *precipd = new float[timecnt*LATS*LONS];
    data->get(precipd, timecnt, LATS, LONS);

    float  precipdmon[12][LATS][LONS];
    for(int i = 0; i<LATS; i++)
        for(int j = 0; j<LONS; j++) {
            float precipdmoncnt[12];
            for(int k = 0; k<12; k++) {
                precipdmon[k][i][j] = 0;
                precipdmoncnt[k] = 0;
            }
            for(int k = 0; k<timecnt; k++) {
                double v = precipd[(k*LATS+i)*LONS+j];
                if(v >= 0 && v <= 30) {
                    precipdmon[k%12][i][j] += v;
                    precipdmoncnt[k%12]++;
                }
            }
            for(int k = 0; k<12; k++)
                precipdmon[k][i][j] /= precipdmoncnt[k];
        }
    delete [] precipd;

    /* use a single byte instead of 2 to save memory,
       resolution of 1/5th of a mm/day resolution */
    uint8_t precipbyte[12][LATS][LONS];
    for(int i = 0; i<12; i++)
        for(int j = 0; j<LATS; j++)
            for(int k = 0; k<LONS; k++)
                if(std::isnan(precipdmon[i][j][k]))
                    precipbyte[i][j][k] = 255;
                else
                    precipbyte[i][j][k] = precipdmon[i][j][k]*5.0;
    
    fwrite(precipbyte, sizeof precipbyte, 1, stdout);
    return 0;
}
