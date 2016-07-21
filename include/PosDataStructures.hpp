#ifndef POS_DATA_STRUCTURES_HPP
#define POS_DATA_STRUCTURES_HPP

#include <string>
#include <stdint.h>
#include <vector>

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
	}
}

#endif

