/* Copyright (c) 2018 Jungle Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//links:
//live data: http://www.nco.ncep.noaa.gov/pmb/products/gfs/
//archive data: https://www.ncdc.noaa.gov/data-access/model-data/model-datasets/global-forcast-system-gfs
// fields list: http://www.nco.ncep.noaa.gov/pmb/products/gfs/gfs.t00z.pgrb2.0p25.f000.shtml
//eccodes: https://software.ecmwf.int/wiki/display/ECC/ecCodes+Home


#include "GribLoader.h"
#include <stdio.h>
#include <stdlib.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <glm.hpp>
using namespace glm;

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

extern "C" {
#include "stb_image_write.h"
#include "stb_image.h"
#include "eccodes.h"
}
#pragma clang diagnostic pop

#define MAX_VAL_LEN  1024

#define CLAMP_8(_a_) (_a_ < 0 ? 0 : (_a_ > 255 ? 255 : _a_))


#define INTENISTY (ocean ? 0.02f : 0.08f)
#define LINE_HALF_LENGTH (ocean ? 64 : 64)
#define POINT_STRIDE (ocean ? 4 : 8)
#define PADDING 64


inline vec2 biLerpVec2(vec2 q11, vec2 q12, vec2 q21, vec2 q22,
                      vec2 quantizedMin, vec2 quantizedMax,
                      vec2 point)
{
    vec2 v2v1 = quantizedMax - quantizedMin;
    vec2 v2v = quantizedMax - point;
    vec2 vv1 = point - quantizedMin;
    return vec2(1.0) / (v2v1.x * v2v1.y) * (q11 * v2v.x * v2v.y +
                                            q21 * vv1.x * v2v.y +
                                            q12 * v2v.x * vv1.y +
                                            q22 * vv1.x * vv1.y
                                            );
}

inline vec3 biLerpVec3(vec3 q11, vec3 q12, vec3 q21, vec3 q22,
                       vec2 quantizedMin, vec2 quantizedMax,
                       vec2 point)
{
    vec2 v2v1 = quantizedMax - quantizedMin;
    vec2 v2v = quantizedMax - point;
    vec2 vv1 = point - quantizedMin;
    return vec3(1.0) / (v2v1.x * v2v1.y) * (q11 * v2v.x * v2v.y +
                                            q21 * vv1.x * v2v.y +
                                            q12 * v2v.x * vv1.y +
                                            q22 * vv1.x * vv1.y
                                            );
}

inline float biLerpFloat(float q11, float q12, float q21, float q22,
                         vec2 quantizedMin, vec2 quantizedMax,
                         vec2 point)
{
    vec2 v2v1 = quantizedMax - quantizedMin;
    vec2 v2v = quantizedMax - point;
    vec2 vv1 = point - quantizedMin;
    return 1.0f / (v2v1.x * v2v1.y) * (q11 * v2v.x * v2v.y +
                                       q21 * vv1.x * v2v.y +
                                       q12 * v2v.x * vv1.y +
                                       q22 * vv1.x * vv1.y
                                       );
}

inline float bilerpFloatData(std::vector<float>& data, vec2 dataPoint, int bufferWidth, int bufferHeight)
{
    ivec2 lookup11 = dataPoint;
    ivec2 lookup12 = lookup11 + ivec2(0,1);
    ivec2 lookup21 = lookup11 + ivec2(1,0);
    ivec2 lookup22 = lookup11 + ivec2(1,1);
    
    ivec2 clmapedLookup11 = min(lookup11, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup12 = min(lookup12, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup21 = min(lookup21, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup22 = min(lookup22, ivec2(bufferWidth - 1, bufferHeight - 1));
    
    float q11 = data[clmapedLookup11.y * bufferWidth + clmapedLookup11.x];
    float q12 = data[clmapedLookup12.y * bufferWidth + clmapedLookup12.x];
    float q21 = data[clmapedLookup21.y * bufferWidth + clmapedLookup21.x];
    float q22 = data[clmapedLookup22.y * bufferWidth + clmapedLookup22.x];
    
    float result = biLerpFloat(q11, q12, q21, q22,
                             lookup11, lookup22,
                             dataPoint);
    return result;
}



inline vec2 bilerpVec2Data(std::vector<vec2>& data, vec2 dataPoint, int bufferWidth, int bufferHeight)
{
    ivec2 lookup11 = dataPoint;
    ivec2 lookup12 = lookup11 + ivec2(0,1);
    ivec2 lookup21 = lookup11 + ivec2(1,0);
    ivec2 lookup22 = lookup11 + ivec2(1,1);
    
    ivec2 clmapedLookup11 = min(lookup11, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup12 = min(lookup12, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup21 = min(lookup21, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup22 = min(lookup22, ivec2(bufferWidth - 1, bufferHeight - 1));
    
    vec2 q11 = data[clmapedLookup11.y * bufferWidth + clmapedLookup11.x];
    vec2 q12 = data[clmapedLookup12.y * bufferWidth + clmapedLookup12.x];
    vec2 q21 = data[clmapedLookup21.y * bufferWidth + clmapedLookup21.x];
    vec2 q22 = data[clmapedLookup22.y * bufferWidth + clmapedLookup22.x];
    
    vec2 result = biLerpVec2(q11, q12, q21, q22,
                             lookup11, lookup22,
                             dataPoint);
    return result;
}

inline float closeWeightDirSize(vec2 a, vec2 b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

inline vec2 bestMatchBilerpVec2Data(std::vector<vec2>* datas, vec2 matchValue, vec2 dataPoint, int bufferWidth, int bufferHeight)
{
    ivec2 lookup11 = dataPoint;
    ivec2 lookup12 = lookup11 + ivec2(0,1);
    ivec2 lookup21 = lookup11 + ivec2(1,0);
    ivec2 lookup22 = lookup11 + ivec2(1,1);
    
    ivec2 clmapedLookup11 = min(lookup11, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup12 = min(lookup12, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup21 = min(lookup21, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup22 = min(lookup22, ivec2(bufferWidth - 1, bufferHeight - 1));
    
    vec2 q11 = datas[0][clmapedLookup11.y * bufferWidth + clmapedLookup11.x];
    vec2 q12 = datas[0][clmapedLookup12.y * bufferWidth + clmapedLookup12.x];
    vec2 q21 = datas[0][clmapedLookup21.y * bufferWidth + clmapedLookup21.x];
    vec2 q22 = datas[0][clmapedLookup22.y * bufferWidth + clmapedLookup22.x];
    
    for(int i = 1; i < 3; i++)
    {
        vec2 q11b = datas[i][clmapedLookup11.y * bufferWidth + clmapedLookup11.x];
        vec2 q12b = datas[i][clmapedLookup12.y * bufferWidth + clmapedLookup12.x];
        vec2 q21b = datas[i][clmapedLookup21.y * bufferWidth + clmapedLookup21.x];
        vec2 q22b = datas[i][clmapedLookup22.y * bufferWidth + clmapedLookup22.x];
        
        if(closeWeightDirSize(q11b, matchValue) < closeWeightDirSize(q11, matchValue))
        {
            q11 = q11b;
        }
        if(closeWeightDirSize(q12b, matchValue) < closeWeightDirSize(q12, matchValue))
        {
            q12 = q12b;
        }
        if(closeWeightDirSize(q21b, matchValue) < closeWeightDirSize(q21, matchValue))
        {
            q21 = q21b;
        }
        if(closeWeightDirSize(q22b, matchValue) < closeWeightDirSize(q22, matchValue))
        {
            q22 = q22b;
        }
    }
    
    vec2 result = biLerpVec2(q11, q12, q21, q22,
                             lookup11, lookup22,
                             dataPoint);
    return result;
}

inline vec2 unlerpedVec2Data(std::vector<vec2>& data, vec2 dataPoint, int bufferWidth, int bufferHeight)
{
    ivec2 lookup11 = dataPoint;
    ivec2 clmapedLookup11 = min(lookup11, ivec2(bufferWidth - 1, bufferHeight - 1));
    return data[clmapedLookup11.y * bufferWidth + clmapedLookup11.x];
}

inline float bilerpImageData(unsigned char * data, vec2 dataPoint, int bufferWidth, int bufferHeight)
{
    ivec2 lookup11 = dataPoint;
    ivec2 lookup12 = lookup11 + ivec2(0,1);
    ivec2 lookup21 = lookup11 + ivec2(1,0);
    ivec2 lookup22 = lookup11 + ivec2(1,1);
    
    ivec2 clmapedLookup11 = min(lookup11, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup12 = min(lookup12, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup21 = min(lookup21, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup22 = min(lookup22, ivec2(bufferWidth - 1, bufferHeight - 1));
    
    float q11 = data[clmapedLookup11.y * bufferWidth + clmapedLookup11.x];
    float q12 = data[clmapedLookup12.y * bufferWidth + clmapedLookup12.x];
    float q21 = data[clmapedLookup21.y * bufferWidth + clmapedLookup21.x];
    float q22 = data[clmapedLookup22.y * bufferWidth + clmapedLookup22.x];
    
    float result = biLerpFloat(q11, q12, q21, q22,
                             lookup11, lookup22,
                             dataPoint);
    return result;
}


inline vec3 bilerpVec3ImageData(unsigned char * data, vec2 dataPoint, int bufferWidth, int bufferHeight)
{
    ivec2 lookup11 = dataPoint;
    ivec2 lookup12 = lookup11 + ivec2(0,1);
    ivec2 lookup21 = lookup11 + ivec2(1,0);
    ivec2 lookup22 = lookup11 + ivec2(1,1);
    
    ivec2 clmapedLookup11 = min(lookup11, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup12 = min(lookup12, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup21 = min(lookup21, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 clmapedLookup22 = min(lookup22, ivec2(bufferWidth - 1, bufferHeight - 1));
    
    vec3 q11 = vec3(data[(clmapedLookup11.y * bufferWidth + clmapedLookup11.x) * 3 + 0],
                    data[(clmapedLookup11.y * bufferWidth + clmapedLookup11.x) * 3 + 1],
                    data[(clmapedLookup11.y * bufferWidth + clmapedLookup11.x) * 3 + 2]);
    vec3 q12 = vec3(data[(clmapedLookup12.y * bufferWidth + clmapedLookup12.x) * 3 + 0],
                    data[(clmapedLookup12.y * bufferWidth + clmapedLookup12.x) * 3 + 1],
                    data[(clmapedLookup12.y * bufferWidth + clmapedLookup12.x) * 3 + 2]);
    vec3 q21 = vec3(data[(clmapedLookup21.y * bufferWidth + clmapedLookup21.x) * 3 + 0],
                    data[(clmapedLookup21.y * bufferWidth + clmapedLookup21.x) * 3 + 1],
                    data[(clmapedLookup21.y * bufferWidth + clmapedLookup21.x) * 3 + 2]);
    vec3 q22 = vec3(data[(clmapedLookup22.y * bufferWidth + clmapedLookup22.x) * 3 + 0],
                    data[(clmapedLookup22.y * bufferWidth + clmapedLookup22.x) * 3 + 1],
                    data[(clmapedLookup22.y * bufferWidth + clmapedLookup22.x) * 3 + 2]);
    
    vec3 result = biLerpVec3(q11, q12, q21, q22,
                               lookup11, lookup22,
                               dataPoint);
    return result;
}

inline vec2 getDataLookup(vec2 imagePos,
                   float dataScale,
                   int dataWidth,
                   int dataHeight,
                   int outputWidth,
                   int outputHeight,
                   float bottomLeftLatitude,
                   float bottomLeftLongitude,
                   float outputWidthDegrees,
                   float outputHeightDegrees)
{
    float dataX = (bottomLeftLongitude * dataScale) + (imagePos.x / outputWidth) * outputWidthDegrees * dataScale;
    float dataY = (bottomLeftLatitude + 90) * dataScale + (imagePos.y / outputHeight) * outputHeightDegrees * dataScale;
    
    dataX = (dataX < 0 ? dataX + dataWidth : (dataX >= dataWidth ? dataX - dataWidth : dataX));
    dataY = (dataY < 0 ? dataY + dataHeight : (dataY >= dataHeight ? dataY - dataHeight : dataY));
    
    return vec2(dataX, dataY);
}

inline void writeFloatValue(float value, float* buffer, vec2 pos, int bufferWidth, int bufferHeight)
{
    vec2 fractions = fract(pos);
    vec2 inverseFractions = vec2(1.0) - fractions;
    
    ivec2 p11 = floor(pos);
    p11 = min(p11, ivec2(bufferWidth - 1, bufferHeight - 1));
    ivec2 p12 = p11 + ivec2(0,1);
    ivec2 p21 = p11 + ivec2(1,0);
    ivec2 p22 = p11 + ivec2(1,1);
    
    p12 = min(p12, ivec2(bufferWidth - 1, bufferHeight - 1));
    p21 = min(p21, ivec2(bufferWidth - 1, bufferHeight - 1));
    p22 = min(p22, ivec2(bufferWidth - 1, bufferHeight - 1));
    
    buffer[p11.y * bufferWidth + p11.x] += value * (inverseFractions.x * inverseFractions.y);
    buffer[p12.y * bufferWidth + p12.x] += value * (inverseFractions.x * fractions.y);
    buffer[p21.y * bufferWidth + p21.x] += value * (fractions.x * inverseFractions.y);
    buffer[p22.y * bufferWidth + p22.x] += value * (fractions.x * fractions.y);
    
}

void readKeys(codes_handle* h)
{
    unsigned long key_iterator_filter_flags = CODES_KEYS_ITERATOR_ALL_KEYS |
    CODES_KEYS_ITERATOR_SKIP_DUPLICATES;
    
    /* Choose a namespace. E.g. "ls", "time", "parameter", "geography", "statistics" */
    const char* name_space="ls";
    
    int msg_count=0;
    
    char value[MAX_VAL_LEN];
    size_t vlen=MAX_VAL_LEN;
    
    codes_keys_iterator* kiter=NULL;
    msg_count++;
    printf("-- GRIB N. %d --\n",msg_count);
    if(!h) {
        printf("ERROR: Unable to create grib handle\n");
        exit(1);
    }
    
    kiter=codes_keys_iterator_new(h,key_iterator_filter_flags,name_space);
    if (!kiter) {
        printf("ERROR: Unable to create keys iterator\n");
        exit(1);
    }
    
    while(codes_keys_iterator_next(kiter))
    {
        const char* name = codes_keys_iterator_get_name(kiter);
        vlen=MAX_VAL_LEN;
        bzero(value,vlen);
        CODES_CHECK(codes_get_string(h,name,value,&vlen),name);
        printf("%s = %s\n",name,value);
        
        /* Alternative way of getting the string value */
        CODES_CHECK(codes_keys_iterator_get_string(kiter, value, &vlen),0);
    }
    
    codes_keys_iterator_delete(kiter);
}

GribLoader::GribLoader(std::string inputName,
                       std::string resourcesDirName,
                       bool ocean_)
{
    setup = false;
    ocean = ocean_;
    int err = 0;
    codes_handle *handle = NULL;

    char keyValue[MAX_VAL_LEN];
    size_t vlen=MAX_VAL_LEN;

    FILE* in = fopen(inputName.c_str(),"r");
    if(!in) {
        fprintf(stderr, "ERROR: unable to open file %s\n",inputName.c_str());
        return ;
    }
    
    
    stbi_set_flip_vertically_on_load(true);
    
    std::string maskName = resourcesDirName + "/mask.png";
    maskData = stbi_load(maskName.c_str(), &maskWidth, &maskHeight, &maskChannels, 0);
    
    std::string normalOverlayName = resourcesDirName + "/normals.png";
    normalOverlayData = stbi_load(normalOverlayName.c_str(), &normalOverlayWidth, &normalOverlayHeight, &normalOverlayChannels, 0);
    
    if(ocean)
    {
        std::string colorsOverlayName = resourcesDirName + "/colors.png";
        colorsOverlayData = stbi_load(colorsOverlayName.c_str(), &colorsOverlayWidth, &colorsOverlayHeight, &colorsOverlayChannels, 0);
    }
    else
    {
        std::string colorsOverlayName = resourcesDirName + "/colorsWind.png";
        colorsOverlayData = stbi_load(colorsOverlayName.c_str(), &colorsOverlayWidth, &colorsOverlayHeight, &colorsOverlayChannels, 0);
    }
    
    
    bool sizeFound = false;
    dataWidth = 0;
    dataHeight = 0;
    dataScale = 0;
    
    bool uFound[3] = {false,false,false};
    bool vFound[3] = {false,false,false};
    bool periodsFound[3] = {false,false,false};

    while ((handle = codes_handle_new_from_file(0, in, PRODUCT_GRIB, &err)) != NULL )
    {
        if (err != CODES_SUCCESS) CODES_CHECK(err,0);
        
        if(!handle) {
            printf("ERROR: Unable to create grib handle\n");
            exit(1);
        }
        
        //readKeys(handle);
        
        if(!sizeFound)
        {
            long xWidth = 0;
            CODES_CHECK(codes_get_long(handle,"Ni",&xWidth),0);
            if(xWidth != 0)
            {
                dataWidth = (int)xWidth;
                dataScale = xWidth / 360;
                dataHeight = 180 * dataScale;
                printf("dataWidth:%d dataHeight:%d dataScale:%.2f\n", dataWidth, dataHeight, dataScale);
                sizeFound = true;
                globalUVs[0].resize(dataWidth * dataHeight);
                if(ocean)
                {
                    globalUVs[1].resize(dataWidth * dataHeight);
                    globalUVs[2].resize(dataWidth * dataHeight);
                    
                    wavePeriods[0].resize(dataWidth * dataHeight);
                    wavePeriods[1].resize(dataWidth * dataHeight);
                    wavePeriods[2].resize(dataWidth * dataHeight);
                }
            }
        }
        
        if(sizeFound)
        {
            vlen=MAX_VAL_LEN;
            bzero(keyValue,vlen);
            CODES_CHECK(codes_get_string(handle,"shortName",keyValue,&vlen), 0);
            //printf("shortName= %s\n",keyValue);
            
            long level = 99;
            CODES_CHECK(codes_get_long(handle,"level",&level),0);
            
            if(ocean)
            {
                if(level == 1 || level == 2)
                {
                    int globalUVIndex = 0;
                    int uvXorYOrPerIndex = -1;
                    
                    if(std::string(keyValue) == "wvdir")
                    {
                        globalUVIndex = 0;
                        uFound[globalUVIndex] = true;
                        uvXorYOrPerIndex = 0;
                    }
                    else if(std::string(keyValue) == "shww")
                    {
                        globalUVIndex = 0;
                        vFound[globalUVIndex] = true;
                        uvXorYOrPerIndex = 1;
                    }
                    else if(std::string(keyValue) == "swdir")
                    {
                        globalUVIndex = (level == 1 ? 1 : 2);
                        uFound[globalUVIndex] = true;
                        uvXorYOrPerIndex = 0;
                    }
                    else if(std::string(keyValue) == "swell")
                    {
                        globalUVIndex = (level == 1 ? 1 : 2);
                        vFound[globalUVIndex] = true;
                        uvXorYOrPerIndex = 1;
                    }
                    else if(std::string(keyValue) == "swper")
                    {
                        globalUVIndex = (level == 1 ? 1 : 2);
                        periodsFound[globalUVIndex] = true;
                        uvXorYOrPerIndex = 2;
                    }
                    else if(std::string(keyValue) == "mpww")
                    {
                        globalUVIndex = 0;
                        periodsFound[globalUVIndex] = true;
                        uvXorYOrPerIndex = 2;
                    }
                    
                    if(uvXorYOrPerIndex >= 0)
                    {
                        codes_iterator* iter=codes_grib_iterator_new(handle,0,&err);
                        if (err != CODES_SUCCESS) CODES_CHECK(err,0);
                        double lat,lon,value;
                        while(codes_grib_iterator_next(iter,&lat,&lon,&value))
                        {
                            int xIndex = floor(lon * dataScale);
                            xIndex = clamp(xIndex, 0, dataWidth - 1);
                            int yIndex = floor((lat + 90) * dataScale);
                            yIndex = clamp(yIndex, 0, dataHeight - 1);
                            
                            int index = yIndex * dataWidth + xIndex;
                            if(value < 500.0)
                            {
                                if(uvXorYOrPerIndex < 2)
                                {
                                    globalUVs[globalUVIndex][index][uvXorYOrPerIndex] = value;
                                }
                                else
                                {
                                    wavePeriods[globalUVIndex][index] = value;
                                }
                            }
                        }
                        
                        codes_grib_iterator_delete(iter);
                    }
                }
            }
            else
            {
                if(level == 0)
                {
                    int uvIndex = -1;
                    if(std::string(keyValue) == "u")
                    {
                        uFound[0] = true;
                        uvIndex = 0;
                    }
                    else if(std::string(keyValue) == "v")
                    {
                        vFound[0] = true;
                        uvIndex = 1;
                    }
                    
                    if(uvIndex >= 0)
                    {
                        codes_iterator* iter=codes_grib_iterator_new(handle,0,&err);
                        if (err != CODES_SUCCESS) CODES_CHECK(err,0);
                        double lat,lon,value;
                        while(codes_grib_iterator_next(iter,&lat,&lon,&value))
                        {
                            int xIndex = floor(lon * dataScale);
                            xIndex = clamp(xIndex, 0, dataWidth - 1);
                            int yIndex = floor((lat + 90) * dataScale);
                            yIndex = clamp(yIndex, 0, dataHeight - 1);
                            
                            int index = yIndex * dataWidth + xIndex;
                            globalUVs[0][index][uvIndex] = value;
                        }
                        
                        codes_grib_iterator_delete(iter);
                    }
                }
            }
        }

        codes_handle_delete(handle);
        
        
        if(ocean)
        {
            bool allFound = true;
            for(int i = 0; i < 3; i++)
            {
                if(!uFound[i] || !vFound[i])
                {
                    allFound = false;
                    break;
                }
            }
            if(allFound)
            {
                setup = true;
                break;
            }
        }
        else
        {
            if(uFound[0] && vFound[0])
            {
                setup = true;
                break;
            }
        }
    }
    
    
    if(ocean)
    {
        
        for(int type = 0; type < 3; type++)
        {
            for(int i = 0; i < globalUVs[type].size(); i++)
            {
                vec2 dirAndSize = globalUVs[type][i];
                
                if(dirAndSize.y > 0.001)
                {
                    vec2 baseDir = normalize(vec2(sin(dirAndSize.x / 360.0 * M_PI * 2.0), cos(dirAndSize.x / 360.0 * M_PI * 2.0)));
                    globalUVs[type][i] = baseDir * dirAndSize.y * 3.0f;
                }
                else
                {
                    globalUVs[type][i] = vec2(0.0f, 0.0f);
                }
            }
        }
    }
}

GribLoader::~GribLoader()
{
    
}

void GribLoader::outputFile(std::string outputName,
                            int outputWidth,
                int outputHeight,
                float bottomLeftLatitude,
                float bottomLeftLongitude,
                float outputWidthDegrees)
{
    if(!setup)
    {
        printf("failed to create output file:%s. Not setup.\n", outputName.c_str());
        return;
    }
    float outputHeightDegrees = ((((float)outputHeight) / outputWidth) * outputWidthDegrees);
    
    uint8_t* outData = (uint8_t*)malloc(sizeof(uint8_t) * outputWidth * outputHeight * 3);
    
    float* outDataFloats[3];
    float* outDataUs[3];
    float* outDataVs[3];
    int typeCount = (ocean ? 3 : 1);
    
    for(int type = 0; type < typeCount; type++)
    {
        outDataFloats[type] = (float*)calloc(sizeof(float), outputWidth * outputHeight);
        if(ocean)
        {
            outDataUs[type] = (float*)calloc(sizeof(float), outputWidth * outputHeight);
            outDataVs[type] = (float*)calloc(sizeof(float), outputWidth * outputHeight);
        }
    }
    
    for(int y = 0; y < outputHeight; y++)
    {
        for(int x = 0; x < outputWidth; x++)
        {
            vec2 dataLookup = getDataLookup(vec2(x, y),
                                            dataScale,
                                            dataWidth,
                                            dataHeight,
                                            outputWidth,
                                            outputHeight,
                                            bottomLeftLatitude,
                                            bottomLeftLongitude,
                                            outputWidthDegrees,
                                            outputHeightDegrees);
            
            float maskX = fract((dataLookup.x + (dataWidth / 2)) / dataWidth) * maskWidth;
            float maskY = dataLookup.y / dataHeight * maskHeight;
            float maskValue = 1.0 - (bilerpImageData(maskData, vec2(maskX, maskY), maskWidth, maskHeight) / 255);
            
            
            if(maskValue > 0.5)
            {
                maskValue = 0.3;
            }
            else
            {
                maskValue = 0.0;
            }
            
            int writeValue = maskValue * 255;
            
            writeValue = CLAMP_8(writeValue);
            
            int outIndex = ((y * outputWidth) + x) * 3;
            
            if(maskValue > 0.1)
            {
                float normalX = fract((dataLookup.x + (dataWidth / 2)) / dataWidth) * normalOverlayWidth;
                float normalY = dataLookup.y / dataHeight * normalOverlayHeight;
                ivec3 normal255Value = bilerpVec3ImageData(normalOverlayData, vec2(normalX, normalY), normalOverlayWidth, normalOverlayHeight);
                
                vec3 normal = vec3(normal255Value.x, normal255Value.y, normal255Value.z) / 127.0f - vec3(1.0,1.0,1.0);
                
                normal = normalize(normal);
                
                if(!ocean && dataLookup.y > dataHeight / 2)
                {
                    normal.y = -normal.y;
                }
                
                float colorX = (normal.x * 0.5 + 0.5)  * colorsOverlayWidth;
                float colorY = (normal.y * -0.5 + 0.5)  * colorsOverlayHeight;
                ivec3 colorValue = bilerpVec3ImageData(colorsOverlayData, vec2(colorX, colorY), colorsOverlayWidth, colorsOverlayHeight);
                
                float intensity = (1.2 - normal.z) * 2.0f;
                
                if(ocean)
                {
                    colorValue = vec3(colorValue) * intensity;
                }
                else
                {
                    colorValue = vec3(colorValue) * intensity * 0.05f + vec3(16,16,16);
                }
                
                outData[outIndex + 0] = CLAMP_8(colorValue.x);
                outData[outIndex + 1] = CLAMP_8(colorValue.y);
                outData[outIndex + 2] = CLAMP_8(colorValue.z);
            }
            else
            {
                outData[outIndex + 0] = writeValue;
                outData[outIndex + 1] = writeValue;
                outData[outIndex + 2] = writeValue;
            }
        }
    }
    
    for(int type = 0; type < typeCount; type++)
    {
        for(int y = -PADDING; y < outputHeight + PADDING; y+=POINT_STRIDE)
        {
            for(int x = -PADDING; x < outputWidth + PADDING; x+=POINT_STRIDE)
            {
                vec2 basePos[2];
                
                basePos[0] = vec2(x + rand() % 8,y + rand() % 8);
                basePos[1] = basePos[0];
                
                vec2 previousNormal[2];
                vec2 prevValue[2];
                int lineLength = LINE_HALF_LENGTH;
                
                for(int i = 0; i < lineLength; i++)
                {
                    for(int direction = 0; direction < 2; direction++)
                    {
                        vec2 dataLookup = getDataLookup(basePos[direction],
                                                        dataScale,
                                                        dataWidth,
                                                        dataHeight,
                                                        outputWidth,
                                                        outputHeight,
                                                        bottomLeftLatitude,
                                                        bottomLeftLongitude,
                                                        outputWidthDegrees,
                                                        outputHeightDegrees);
                        
                        vec2 value;
                        if(!ocean || i == 0)
                        {
                            value = bilerpVec2Data(globalUVs[type], dataLookup, dataWidth, dataHeight);
                        }
                        else
                        {
                            value = bestMatchBilerpVec2Data(globalUVs, prevValue[direction], dataLookup, dataWidth, dataHeight);
                        }
                        
                        prevValue[direction] = value;
                        
                        if(ocean && i == 0 && direction == 0)
                        {
                            float period = bilerpFloatData(wavePeriods[type], dataLookup, dataWidth, dataHeight);
                            lineLength = powf(period * 0.5, 1.8) * 0.75;
                            lineLength = clamp(lineLength, 4, LINE_HALF_LENGTH);
                        }
                        
                        vec2 fraction = value * INTENISTY;
                        float intensity = length(fraction);
                        vec2 normal = fraction / intensity;
                        
                        if(intensity < 0.01)
                        {
                            break;
                        }
                        
                        if(ocean && i != 0)
                        {
                            //normal = normalize(mix(normal, previousNormal, 0.95));
                            if(dot(normal, previousNormal[direction]) < 0.9)
                            {
                                break;
                            }
                        }
                        
                        previousNormal[direction] = normal;
                        
                        
                        vec2 outPos = basePos[direction];
                        
                        if((i != 0 || direction == 0) &&
                           outPos.x < outputWidth &&
                           outPos.x >= 0 &&
                           outPos.y < outputHeight &&
                           outPos.y >= 0)
                        {
                            float writeIntensity = 0.0f;
                            if(ocean)
                            {
                                float falloff = (1.0f - ((float)i) / lineLength);
                                falloff = powf(falloff, 0.6);
                                writeIntensity = intensity * falloff;
                            }
                            else
                            {
                                float falloff = (1.0f - ((float)i) / lineLength);
                                writeIntensity = pow(intensity, 1.5) * falloff;
                            }
                            writeFloatValue(writeIntensity, outDataFloats[type], outPos, outputWidth, outputHeight);
                            
                            if(ocean)
                            {
                                writeFloatValue(value.x, outDataUs[type], outPos, outputWidth, outputHeight);
                                writeFloatValue(value.y, outDataVs[type], outPos, outputWidth, outputHeight);
                            }
                        }
                        
                        if(ocean)
                        {
                            normal = vec2(normal.y, -normal.x);
                        }
                        
                        if(direction == 0)
                        {
                            basePos[direction] = basePos[direction] + normal;
                        }
                        else
                        {
                            basePos[direction] = basePos[direction] - normal;
                        }
                    }
                }
            }
        }
    }
    
    vec3 southColor = normalize(vec3(0.3,0.5,1.0)) * 1.34f;
    vec3 northColor = normalize(vec3(1.0,1.0,0.2)) * 1.34f;
    vec3 eastColor = normalize(vec3(1.0,0.2,0.2)) * 1.34f;
    vec3 westColor = normalize(vec3(0.2,1.0,1.0)) * 1.34f;
    
    /*printf("southColor:%d, %d, %d\n", (int)(255 * southColor.x), (int)(255 * southColor.y), (int)(255 * southColor.z));
    printf("northColor:%d, %d, %d\n", (int)(255 * northColor.x), (int)(255 * northColor.y), (int)(255 * northColor.z));
    printf("eastColor:%d, %d, %d\n", (int)(255 * eastColor.x), (int)(255 * eastColor.y), (int)(255 * eastColor.z));
    printf("westColor:%d, %d, %d\n", (int)(255 * westColor.x), (int)(255 * westColor.y), (int)(255 * westColor.z));*/
        
        for(int y = 0; y < outputHeight; y++)
        {
            for(int x = 0; x < outputWidth; x++)
            {
                int outIndex = ((y * outputWidth) + x) * 3;
                int outIndexFloat = ((y * outputWidth) + x);
                
                int maskValue = outData[outIndex + 0];
                
                if(ocean && (outData[outIndex + 0] > 1 || outData[outIndex + 1] > 1 || outData[outIndex + 2] > 1))
                {
                    //outData[outIndex + 0] = 32;
                    //outData[outIndex + 1] = 32;
                   // outData[outIndex + 2] = 32;
                }
                else
                {
                    
                    vec2 dataLookup = getDataLookup(vec2(x,y),
                                                    dataScale,
                                                    dataWidth,
                                                    dataHeight,
                                                    outputWidth,
                                                    outputHeight,
                                                    bottomLeftLatitude,
                                                    bottomLeftLongitude,
                                                    outputWidthDegrees,
                                                    outputHeightDegrees);
                    
                    for(int type = 0; type < typeCount; type++)
                    {
                        vec2 fraction = vec2(0.0);
                        float dataValue = outDataFloats[type][outIndexFloat];;
                        
                        
                        if(ocean)
                        {
                            vec2 value = vec2(outDataUs[type][outIndexFloat], outDataVs[type][outIndexFloat]);
                            fraction = clamp(normalize(value), vec2(-1.0), vec2(1.0)); //clamp due to devide by zeroes
                            
                            //fraction = mix(fraction, value * dataValue * 2.0f, 0.2);
                            //fraction = mix(fraction, vec2(outDataUs[type][outIndexFloat], outDataVs[type][outIndexFloat]) * 2.0f, 0.0);
                        }
                        else
                        {
                            vec2 value = bilerpVec2Data(globalUVs[type], dataLookup, dataWidth, dataHeight);
                            fraction = clamp(value / vec2(10.0), vec2(-1.0), vec2(1.0));
                        }
                        
                        vec3 baseColor;
                        if(ocean)
                        {
                            vec3 baseColorNS = mix(southColor, northColor, fraction.y * 0.5 + 0.5);
                            vec3 baseColorEW = mix(westColor, eastColor , fraction.x * 0.5 + 0.5);
                            
                            baseColor = baseColorNS + baseColorEW;
                            
                        }
                        else
                        {
                            
                            vec3 coldColor = vec3(0.3,0.5,1.0);
                            vec3 warmColor = vec3(1.0,0.5,0.3);
                            vec3 equatorColor = (coldColor + warmColor) * 0.5f;
                            
                            float coldWarmLerpPole = fraction.y * -0.5 + 0.5;
                            if(dataLookup.y > dataHeight / 2)
                            {
                                coldWarmLerpPole = 1.0 - coldWarmLerpPole;
                            }
                            
                            vec3 baseColorPole = mix(coldColor, warmColor, coldWarmLerpPole);
                            vec3 baseColorEquator = equatorColor;
                            
                            float poleFraction = fabsf(((dataLookup.y) - dataHeight / 2) / (dataHeight / 2));
                            poleFraction = smoothstep(0.0f, 0.1f, poleFraction);
                            
                            baseColor = mix(baseColorPole, baseColorEquator, 1.0f - poleFraction);
                        }
                        
                        
                        vec3 outDataColorFloat = baseColor * 128.0f;
                        
                        ivec3 outColor = mix(vec3(maskValue), vec3(maskValue) * 0.5f, clamp(dataValue, 0.0f, 1.0f)) + outDataColorFloat * dataValue;
                        
                        outData[outIndex + 0] = CLAMP_8(outData[outIndex + 0] + outColor.x);
                        outData[outIndex + 1] = CLAMP_8(outData[outIndex + 1] + outColor.y);
                        outData[outIndex + 2] = CLAMP_8(outData[outIndex + 2] + outColor.z);
                    }
                }
            }
        }
    
    stbi_flip_vertically_on_write(true);
    //int result = stbi_write_png(outputName.c_str(), outputWidth, outputHeight, 3, outData, outputWidth * 3);
    int result = stbi_write_jpg(outputName.c_str(), outputWidth, outputHeight, 3, outData, 95);
    
    if(result)
    {
        printf("file written to:%s\n", outputName.c_str());
    }
    else
    {
        printf("failed to write output file:%s\n", outputName.c_str());
    }
    
    free(outData);
    
    for(int type = 0; type < typeCount; type++)
    {
        free(outDataFloats[type]);
        if(ocean)
        {
            free(outDataUs[type]);
            free(outDataVs[type]);
        }
    }
}
