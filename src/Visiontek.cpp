#include "Visiontek.hpp"
#include <cstdlib>
#include <cstring>
extern "C"{
	#include<0202lcd.h>
}

namespace lcd{
	void DisplayText(unsigned char line_no, unsigned char column, const char *data, unsigned char font){
			static unsigned char toDisplayBuffer[250] = {0};

			const int toCopy = std::strlen(data);
			if(toCopy > 0){
				std::memset(toDisplayBuffer,0,sizeof(toDisplayBuffer));
				std::strncpy((char*)toDisplayBuffer,(const char * )data,sizeof(toDisplayBuffer) - 1);
				lk_disptext(line_no,column,toDisplayBuffer,font);
			}
	}
}