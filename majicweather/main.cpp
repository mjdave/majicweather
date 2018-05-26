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

#include <iostream>
#include <getopt.h>
#include "GribLoader.h"

#include "json11.hpp"
using namespace json11;

#define DEFAULT_OUTPUT_WIDTH 2048
#define DEFAULT_OUTPUT_HEIGHT 1024

#define DEFAULT_BOTTOM_LEFT_LATITUDE -90
#define DEFAULT_BOTTOM_LEFT_LONGITUDE -180
#define DEFAULT_WIDTH_LONGITUDE 360



#include <stdio.h>
#include <fstream>

std::string get_file_contents(const std::string& filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return(contents);
    }
    
    return "";
}

static const struct option longopts[] = {
    { "input",    required_argument, NULL, 'i' },
    { "outputFileName",    required_argument, NULL, 'o' },
    { "outputBaseDir",    required_argument, NULL, 'b' },
    { "outputStamp",    required_argument, NULL, 's' },
    { "resources",    required_argument, NULL, 'r' },
    { "width",    required_argument, NULL, 'w' },
    { "height",    required_argument, NULL, 'h' },
    { "latitude",    required_argument, NULL, 'y' },
    { "longitude",    required_argument, NULL, 'x' },
    { "dataWidth",    required_argument, NULL, 'd' },
    { "jsonFile",    required_argument, NULL, 'j' },
    { "ocean",    no_argument, NULL, 'O' },
    {},
};



void printErrorForMissingArgForOption(int option)
{
    printf("Given option: %c must be given a value.\n", option);
}

void printUsage()
{
    std::string usageString = "Usage: majicweather -i input_file [-o output_file | -b outputBaseDir -j jsonFile] -r resources_directory\n\
Other options:\n\
-s outputStamp      when used with outputBaseDir, adds outputStamp to the output file name\n\
-w width            output file width in pixels\n\
-h height           output file height in pixels\n\
-y latitude         lower left latitude of output image\n\
-x longitude        lower left longitude of output image\n\
-d dataWidth        width in degrees of output image. Data height is calculated based on -h\n\
-O                  specifies Ocean - will look for swell information in the input file instead of wind\n\
";
    
    printf("%s\n", usageString.c_str());
}

int main(int argc, const char * argv[]) {
    
    int option_index = 0;
    int opt = 0;
    
    bool ocean = false;
    
    std::string inputName = "";
    std::string outputFileName = "";
    std::string outputBaseDir = "";
    std::string outputStamp = "";
    std::string resourcesDirName = "";
    std::string jsonFileName = "";
    
    int width = DEFAULT_OUTPUT_WIDTH;
    int height = DEFAULT_OUTPUT_HEIGHT;
    float latitude = DEFAULT_BOTTOM_LEFT_LATITUDE;
    float longitude = DEFAULT_BOTTOM_LEFT_LONGITUDE;
    float dataWidthDegrees = DEFAULT_WIDTH_LONGITUDE;
    
    while ((opt = getopt_long_only(argc, (char **)argv, "i:o:r:w:y:x:d:j:b:s:O",
                                   longopts, &option_index)) != -1)
    {
        
        switch (opt) {
            case 'o':
            {
                if(optarg)
                {
                    outputFileName = optarg;
                }
            }
                break;
            case 'O':
            {
                ocean = true;
            }
                break;
            case 'b':
            {
                if(optarg)
                {
                    outputBaseDir = optarg;
                }
            }
                break;
            case 's':
            {
                if(optarg)
                {
                    outputStamp = optarg;
                }
            }
                break;
            case 'i':
            {
                if(optarg)
                {
                    inputName = optarg;
                }
            }
                break;
            case 'r':
            {
                if(optarg)
                {
                    resourcesDirName = optarg;
                }
            }
                break;
            case 'j':
            {
                if(optarg)
                {
                    jsonFileName = optarg;
                }
            }
                break;
            case 'w':
            {
                if(optarg)
                {
                    width = atoi(optarg);
                }
            }
                break;
            case 'h':
            {
                if(optarg)
                {
                    height = atoi(optarg);
                }
            }
                break;
            case 'y':
            {
                if(optarg)
                {
                    latitude = atof(optarg);
                }
            }
                break;
            case 'x':
            {
                if(optarg)
                {
                    longitude = atof(optarg);
                }
            }
                break;
            case 'd':
            {
                if(optarg)
                {
                    dataWidthDegrees = atof(optarg);
                }
            }
                break;
            case '?':
            {
                printErrorForMissingArgForOption(optopt);
                return 0;
            }
                break;
                
            default:
                break;
        }
    }
    
    if(inputName.empty() || (outputFileName.empty() && (outputBaseDir.empty() || jsonFileName.empty())) || resourcesDirName.empty())
    {
        printUsage();
        return 0;
    }
    
    GribLoader* gribLoader = new GribLoader(inputName,
                                            resourcesDirName,
                                            ocean);
    
    if(!jsonFileName.empty())
    {
        std::string fileContents = get_file_contents(jsonFileName);
        std::string errorMessage;
        Json json = Json::parse(fileContents, errorMessage, JsonParse::STANDARD);
        if(json.is_null())
        {
            printf("Failed to parse json data at path:%s - error message:%s", jsonFileName.c_str(), errorMessage.c_str());
        }
        else
        {
            std::vector<Json> locations = json["locations"].array_items();
            for(Json& location : locations)
            {
                std::string locationName = location["key"].string_value();
                float latitude = location["latitude"].number_value();
                float longitude = location["longitude"].number_value();
                float dataWidth = location["dataWidth"].number_value();
                
                std::vector<Json> outputs = location["outputs"].array_items();
                for(Json& output : outputs)
                {
                    std::string outputKey = output["key"].string_value();
                    int width = output["width"].int_value();
                    int height = output["height"].int_value();
                    
                    std::string fullOutputPath = outputBaseDir + "/" + locationName + "/";
                    
                    std::string mkdirCommand = std::string("mkdir -p \"") + fullOutputPath + "\"";
                    
                    system(mkdirCommand.c_str());
                    
                    fullOutputPath = fullOutputPath + outputKey;
                    if(!outputStamp.empty())
                    {
                        fullOutputPath = fullOutputPath + "_" + outputStamp;
                    }
                    
                    fullOutputPath = fullOutputPath + ".jpg";
                    
                    gribLoader->outputFile(fullOutputPath,
                                           width,
                                           height,
                                           latitude,
                                           longitude,
                                           dataWidth);
                }
            }
        }
    }
    else
    {
        gribLoader->outputFile(outputFileName,
                               width,
                               height,
                               latitude,
                               longitude,
                               dataWidthDegrees);
    }

    delete gribLoader;
    
    
    return 0;
}
