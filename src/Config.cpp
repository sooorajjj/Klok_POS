#include "Config.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>


namespace klok
{
	namespace pos
	{

			namespace
			{
				static bool is_space(char c){
					return c == ' ';
				}
			}
			Configuration::Configuration():parseError(false){}

			int32_t Configuration::ParseFromFile(const char * fileName,Configuration & outConfig){

				std::ifstream inFile(fileName,std::ifstream::in);

				if(!inFile.good() || !inFile.is_open()){
					std::cerr << "Opening file failed ! " << fileName << '\n';
					return -1;
				}

				std::string line = "";
				line.reserve(512);

				std::getline(inFile,line);

				Configuration::Data_t dat;
				while(line.size()){
					std::string::size_type pos = line.find_first_of('=');

					if(pos != std::string::npos){
						// std::cout << "Found a possible match!\n";

						std::string key,value;

						key = line.substr(0,pos);
						value = line.substr(pos+1);

						const std::string::size_type allSpaces = std::count_if(value.begin(),value.end(),is_space);

						if(allSpaces != value.size()){
							// std::cout << '{' << key << " : "  << value << '}' << '\n';
							dat[key] = value;
						}

					}

					if(!inFile.eof())
						std::getline(inFile,line);
					else
						break;
				}

				outConfig.setData(dat);

				return 0;
			}
			void Configuration::setData(const Data_t & newData){
				data = newData;
			}
			Configuration::Data_t & Configuration::getData(){
				return data;
			}
		
	}
}