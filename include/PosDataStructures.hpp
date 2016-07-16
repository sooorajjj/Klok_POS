#ifndef POS_DATA_STRUCTURES_HPP
#define POS_DATA_STRUCTURES_HPP

#include <string>
#include <stdint.h>
namespace SQLite
{
	class Database;
}
namespace klok {
	namespace pc {
		class User {
			std::string id,name,password;
		public:
			static int32_t FromDatabase(SQLite::Database & db,const char * id,User & outUser);
			static int32_t CreateTable(SQLite::Database & db,bool dropIfExist);

			struct Queries {
				static const char * TABLE_NAME;
				static const char * CREATE_USER_TABLE_QUERY;
				static const char * DROP_USER_TABLE_QUERY;
				static const char * SELECT_USER_WITH_ID_FROM_TABLE;
			};
		};
	}
}

#endif

