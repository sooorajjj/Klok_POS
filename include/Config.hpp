#ifndef POS_CONFIG_HPP
#define POS_CONFIG_HPP


#include <map>
#include <string>
#include <stdint.h>
namespace klok
{
	namespace pos
	{
		class Configuration
		{
			std::map<std::string, std::string> data;
			bool parseError;
		public:
			typedef std::map<std::string, std::string> Data_t;
			Configuration();
			static int32_t ParseFromFile(const char * fileName,Configuration & outConfig);	
			void setData(const Data_t & newData);
			Data_t & getData();

		};
	}
}

#endif