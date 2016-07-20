#include "PosDataStructures.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

namespace klok {
	namespace pc {

			namespace {

				using SQLite::Database;
				using SQLite::Column;
				using SQLite::Statement;

			}

			int32_t User::GetNextTransactionIDForUser(SQLite::Database & db,const char * id,std::string & outID){
				try
				{
					SQLite::Statement query(db,User::Queries::GET_NEXT_TRANS_ID_FOR_USER);
					query.bind(1,id);

					if(query.executeStep())
					{
						outID = query.getColumn(0).getString();
						return 0;
					}
					else
					{
						return -1;
					}
				}
				catch(std::exception & e)
				{
					std::printf("User::GetNextTransactionIDForUser -> Fatal Error query : %s \n %s\n",User::Queries::GET_NEXT_TRANS_ID_FOR_USER, e.what());
					return -1;
				}

				return 0;
			}

			int32_t User::FromDatabase(SQLite::Database & db,const char * id,User & outUser){

				try
				{
					SQLite::Statement query(db,User::Queries::SELECT_USER_WITH_ID_FROM_TABLE);
					query.bind(1,id);

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

			int32_t User::CreateTable(SQLite::Database & db,bool dropIfExist){
				try
				{
					if(dropIfExist){
						SQLite::Statement query(db,User::Queries::DROP_USER_TABLE_QUERY);
						query.executeStep();
					}

					SQLite::Statement query(db,User::Queries::CREATE_USER_TABLE_QUERY);
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
			const char * User::Queries::GET_NEXT_TRANS_ID_FOR_USER = "select MAX(Trans_ID)+1 as Next_ID from pay_coll_trans where User_ID=?";
			const char * User::Queries::TABLE_NAME = "pay_coll_user";
			const char * User::Queries::SELECT_USER_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_user WHERE User_ID=?";
			const char * User::Queries::CREATE_USER_TABLE_QUERY = 	"CREATE TABLE pay_coll_user (" 
																	"User_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," 
																	"User_Name 	 VARCHAR(20) NOT NULL UNIQUE," 
																	"Password	  	 VARCHAR(8) NOT NULL );";
			const char * User::Queries::DROP_USER_TABLE_QUERY = "DROP TABLE pay_coll_user IF EXISTS;";
	}
} 
