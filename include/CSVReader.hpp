#ifndef CSV_READER_HPP
#define CSV_READER_HPP
#include <string>
#include <vector>
#include <stdint.h>
namespace klok
{
	namespace data
	{
		
		struct ProductStock
		{
			std::string Code,SalesRate,Name,ShortName,StockQuantity;
			static int32_t fromFile(const char * fileName,std::string & error_desc,std::vector<ProductStock> & outStockData);
		};



		struct UserData
		{
			std::string Code,Name,Password;
			static int32_t fromFile(const char * fileName,std::string & error_desc,std::vector<UserData> & outUserData);
		};


	}
}

#endif


