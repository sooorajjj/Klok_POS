#ifndef POS_DATA_STRUCTURES_HPP
#define POS_DATA_STRUCTURES_HPP

#include <string>
#include <stdint.h>
#include <vector>
#include <cstdio>

#include "Visiontek.hpp"

extern "C"{
	#include<0202lcd.h>
}

namespace SQLite
{
	class Database;
}
namespace klok {
	namespace pc {
		class User {
		public:
			std::string id,name,password;
			static int32_t FromDatabase(SQLite::Database & db,const char * id,User & outUser);
			static int32_t CreateTable(SQLite::Database & db,bool dropIfExist);
			static int32_t GetNextTransactionIDForUser(SQLite::Database & db,const char * id,std::string & outID);
			static int32_t GetAllFromDatabase(SQLite::Database & db,std::vector<User> &outUsers,uint32_t maxToRead);
			struct Queries {
				static const char * TABLE_NAME;
				static const char * GET_ALL_QUERY;
				static const char * CREATE_USER_TABLE_QUERY;
				static const char * DROP_USER_TABLE_QUERY;
				static const char * SELECT_USER_WITH_ID_FROM_TABLE;
				static const char * GET_NEXT_TRANS_ID_FOR_USER;
			};
		};
		class Customer
		{
		public:
			std::string id,name,contact,cur_amt,sub_amt,due_amt;
			static int32_t FromDatabase(SQLite::Database & db,const char * id,Customer & outCustomer);
			static int32_t CreateTable(SQLite::Database & db,bool dropIfExist);
			static int32_t GetNextTransactionIDForCustomer(SQLite::Database & db,const char * id,std::string & outID);
			static int32_t GetAllFromDatabase(SQLite::Database & db,std::vector<Customer> &outCustomers,uint32_t maxToRead);
			struct Queries {
				static const char * TABLE_NAME;
				static const char * GET_ALL_QUERY;
				static const char * CREATE_CUSTOMER_TABLE_QUERY;
				static const char * DROP_CUSTOMER_TABLE_QUERY;
				static const char * SELECT_CUSTOMER_WITH_ID_FROM_TABLE;
				static const char * GET_NEXT_TRANS_ID_FOR_CUSTOMER;
			};		
		};
		struct MenuResult
			{
			bool wasCancelled;
			size_t selectedIndex;
			};

			struct KEYS
			{
			static const uint8_t KEY_F2 = 18;
			static const uint8_t KEY_F3 = 19;
			static const uint8_t KEY_ENTER = 0x0f;
			static const uint8_t KEY_CANCEL = 0x0c;	
			};


		template<typename T>
		int display_sub_range(const std::vector<T> & all,uint8_t MaximumRowsToDisplay,MenuResult & outMenuResult,const char * (*toStringConverter)(const T & )){
			size_t offset=0,selected=0;

			typedef std::vector<T> InputList;
			typedef typename InputList::const_iterator ListPointer;


			ListPointer startPtr = all.begin();
			ListPointer endPtr = all.end();

			if(MaximumRowsToDisplay  > all.size())
				MaximumRowsToDisplay = all.size();


			bool running = true;
			printf("ONCE ;  [MaximumRowsToDisplay : %d] \n",MaximumRowsToDisplay );
			while(running){

				std::printf("\n\n");
				//clear display
				lk_dispclr();

				// printf("PER DRAW ;  [offset : %d] \n",offset );
				// printf("PER DRAW ;  [selected : %d] \n",selected );

				for (size_t i = offset ; (i < all.size()) && i < (offset + MaximumRowsToDisplay); i++)
				{
					// printf("PER ROW ;  [i : %d] \n",i );
					// printf("PER ROW ;  [ (offset + MaximumRowsToDisplay) : %d] \n",(offset + MaximumRowsToDisplay) );
					// printf("PER ROW ;  [ selected %% MaximumRowsToDisplay : %d] \n",selected % MaximumRowsToDisplay );
					lcd::DisplayText(i % MaximumRowsToDisplay,0,toStringConverter(all[i]),0);
					if(i == selected){
          				lk_disphlight(i % MaximumRowsToDisplay);
          				printf("* - %s\n",toStringConverter(all[i]));
          			}
          			else printf(" - %s\n",toStringConverter(all[i]));

				}

				// lk_get key
				int x = lk_getkey();

				if(KEYS::KEY_F2 == x && selected > 0)
				{
					--selected;
				}
				else if(KEYS::KEY_F3 == x && selected < all.size() - 1 )
				{
					++selected;
				}
				else if (KEYS::KEY_ENTER == x)
				{
					outMenuResult.selectedIndex = selected;
					outMenuResult.wasCancelled = false;
					printf("User selected : %d,%s\n",(int)selected,toStringConverter(all[selected]));
					return 0;
				}
				else if(KEYS::KEY_CANCEL == x)
				{
					outMenuResult.selectedIndex = selected;
					outMenuResult.wasCancelled = true;
					printf("User cancelled : %d,%s\n",(int)selected,toStringConverter(all[selected]));
					return -1;
				}
				offset = ((selected / MaximumRowsToDisplay) * MaximumRowsToDisplay);
			}
			return 0;
		}
	}
}

#endif

