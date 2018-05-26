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

#ifndef __GribLoader__
#define __GribLoader__

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <glm.hpp>
using namespace glm;
#pragma clang diagnostic pop

class GribLoader {
private:
    unsigned char *maskData;
    unsigned char *colorsOverlayData;
    unsigned char *normalOverlayData;
    
    std::vector<vec2> globalUVs[3];
    std::vector<float> wavePeriods[3];
    
    bool ocean;
    
    int maskWidth;
    int maskHeight;
    int maskChannels;
    
    int colorsOverlayWidth;
    int colorsOverlayHeight;
    int colorsOverlayChannels;
    
    int normalOverlayWidth;
    int normalOverlayHeight;
    int normalOverlayChannels;
    
    int dataWidth;
    int dataHeight;
    float dataScale;
    
    bool setup;
    
public:
    GribLoader(std::string inputName,
               std::string resourcesDirName,
               bool ocean);
    ~GribLoader();
    
    
    void outputFile(std::string outputName,
                    int outputWidth,
                    int outputHeight,
                    float bottomLeftLatitude,
                    float bottomLeftLongitude,
                    float outputWidthDegrees);
    
private:
};

#endif /* defined(__GribLoader__) */
