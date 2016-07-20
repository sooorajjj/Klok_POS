#include "PosDataStructures.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

namespace sooraj 
{
	namespace pc 
	{
		namespace 
		{
			using SQLite::Database;
			using SQLite::Column;
			using SQLite::Statement;

		}

		int32_t User::FromDatabase(SQLite::Database & db,const char * id,User & outUser)
		{

			const std::string queryString = "SELECT * FROM pay_coll_user";
			try
			{
				SQLite::Statement query(db,User::Queries::SELECT_USER_WITH_ID_FROM_TABLE);
				query.bind(1,User::Queries::TABLE_NAME);
				query.bind(2,id);

				if(query.executeStep())
				{
					outUser.id = query.getColumn(0).getString();
					outUser.name = query.getColumn(1).getString();
					outUser.password = query.getColumn(2).getString();
					return 0;
				}
				else
				{
					return -1;
				}
			}
			catch(std::exception & e)
			{
				std::printf("User::FromDatabase -> Fatal Error %s\n", e.what());
				return -1;
			}

			return 0;
		}

		int32_t User::CreateTable(SQLite::Database & db,bool dropIfExist)
		{
			const std::string queryString = "SELECT * FROM pay_coll_user WHERE User_ID=?";
			try
			{
				SQLite::Statement query(db,queryString);
				if(query.executeStep()) {
					return 0;
				}
				else {
					return -1;
				}
			}
			catch(std::exception & e)
			{
				std::printf("User::CreateTable -> Fatal Error %s\n", e.what());
				return -1;
			}

			return 0;
		}
		const char * User::Queries::TABLE_NAME = "pay_coll_user";
		const char * User::Queries::SELECT_USER_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_user WHERE User_ID=?";
		const char * User::Queries::CREATE_USER_TABLE_QUERY = 	"CREATE TABLE ? (" 
																"User_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," 
																"User_Name 	 VARCHAR(20) NOT NULL UNIQUE," 
																"Password	  	 VARCHAR(8) NOT NULL );";
		const char * User::Queries::DROP_USER_TABLE_QUERY = "DROP TABLE ? IF EXISTS;";
	}
} 
