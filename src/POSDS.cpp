#include "PosDataStructures.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

#include <cstdio>
#include <vector>

namespace klok {
	namespace pc {

			namespace {

				using SQLite::Database;
				using SQLite::Column;
				using SQLite::Statement;
			}
			int32_t User::GetAllFromDatabase(SQLite::Database & db,std::vector<User> &outUsers,uint32_t maxToRead){
				try {
					SQLite::Statement query(db,User::Queries::GET_ALL_QUERY);

					while(query.executeStep() && (maxToRead--))
					{
						User outUser;
						outUser.id = query.getColumn(0).getString();
						outUser.name = query.getColumn(1).getString();
						outUser.password = query.getColumn(2).getString();
						outUsers.push_back(outUser);
					}
					return 0;
				}
				catch(std::exception & e)
				{
					std::printf("User::GetAllFromDatabase -> Fatal Error query :%s\n %s\n",User::Queries::GET_NEXT_TRANS_ID_FOR_USER, e.what());
					return -1;
				}
				return 0;
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

			int32_t Customer::GetAllFromDatabase(SQLite::Database & db,std::vector<Customer> &outCustomers,uint32_t maxToRead){
				try {
					SQLite::Statement query(db,Customer::Queries::GET_ALL_QUERY);

					while(query.executeStep() && (maxToRead--))
					{
						Customer outCustomer;
						outCustomer.id = query.getColumn(0).getString();
						outCustomer.name = query.getColumn(1).getString();
						outCustomer.contact = query.getColumn(2).getString();
						outCustomer.cur_amt = query.getColumn(3).getString();
						outCustomer.sub_amt = query.getColumn(4).getString();
						outCustomer.due_amt = query.getColumn(5).getString();
						outCustomers.push_back(outCustomer);
					}
					return 0;
				}
				catch(std::exception & e)
				{
					std::printf("Customer::GetAllFromDatabase -> Fatal Error query :%s\n %s\n",Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER, e.what());
					return -1;
				}
				return 0;
			}

			int32_t Customer::GetNextTransactionIDForCustomer(SQLite::Database & db,const char * id,std::string & outID){
				try
				{
					SQLite::Statement query(db,Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER);
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
					std::printf("Customer::GetNextTransactionIDForCustomer -> Fatal Error query : %s \n %s\n",Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER, e.what());
					return -1;
				}

				return 0;
			}

			int32_t Customer::FromDatabase(SQLite::Database & db,const char * id,Customer & outCustomer){

				try
				{
					SQLite::Statement query(db,Customer::Queries::SELECT_CUSTOMER_WITH_ID_FROM_TABLE);
					query.bind(1,id);

					if(query.executeStep())
					{
						outCustomer.id = query.getColumn(0).getString();
						outCustomer.name = query.getColumn(1).getString();
						outCustomer.contact = query.getColumn(2).getString();
						outCustomer.cur_amt = query.getColumn(3).getString();
						outCustomer.sub_amt = query.getColumn(4).getString();
						outCustomer.due_amt = query.getColumn(5).getString();
						return 0;
					}
					else
					{
						return -1;
					}
				}
				catch(std::exception & e)
				{
					std::printf("Customer::FromDatabase -> Fatal Error %s\n", e.what());
					return -1;
				}

				return 0;
			}

			int32_t Customer::CreateTable(SQLite::Database & db,bool dropIfExist){
				try
				{
					if(dropIfExist){
						SQLite::Statement query(db,Customer::Queries::DROP_CUSTOMER_TABLE_QUERY);
						query.executeStep();
					}

					SQLite::Statement query(db,Customer::Queries::CREATE_CUSTOMER_TABLE_QUERY);
					if(query.executeStep()) {
						return 0;
					}
					else {
						return -1;
					}
				}
				catch(std::exception & e)
				{
					std::printf("Customer::CreateTable -> Fatal Error %s\n", e.what());
					return -1;
				}

				return 0;
			}
			
			int32_t Transaction::GetAllFromDatabase(SQLite::Database & db,std::vector<Transaction> &outTransactions,uint32_t maxToRead){
				try {
					SQLite::Statement query(db,Transaction::Queries::GET_ALL_QUERY);

					while(query.executeStep() && (maxToRead--))
					{
						Transaction outTransaction;
						outTransaction.trans_id  = query.getColumn(0).getString();
						outTransaction.cust_id   = query.getColumn(1).getString();
						outTransaction.user_id   = query.getColumn(2).getString();
						outTransaction.gross_amt = query.getColumn(3).getString();
						outTransaction.add_less  = query.getColumn(4).getString();
						outTransaction.net_amt   = query.getColumn(5).getString();
						outTransaction.date_time = query.getColumn(6).getString();
						outTransactions.push_back(outTransaction);
					}
					return 0;
				}
				catch(std::exception & e)
				{
					std::printf("Transaction::GetAllFromDatabase -> Fatal Error query :%s\n %s\n",Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER, e.what());
					return -1;
				}
				return 0;
			}

			int32_t Transaction::FromDatabase(SQLite::Database & db,const char * trans_id,Transaction & outTransaction){

				try
				{
					SQLite::Statement query(db,Transaction::Queries::SELECT_TRANSACTION_WITH_ID_FROM_TABLE);
					query.bind(1,trans_id);

					if(query.executeStep())
					{
						outTransaction.trans_id  = query.getColumn(0).getString();
						outTransaction.cust_id   = query.getColumn(1).getString();
						outTransaction.user_id   = query.getColumn(2).getString();
						outTransaction.gross_amt = query.getColumn(3).getString();
						outTransaction.add_less  = query.getColumn(4).getString();
						outTransaction.net_amt   = query.getColumn(5).getString();
						outTransaction.date_time = query.getColumn(6).getString();
						return 0;
					}
					else
					{
						return -1;
					}
				}
				catch(std::exception & e)
				{
					std::printf("Transaction::FromDatabase -> Fatal Error %s\n", e.what());
					return -1;
				}

				return 0;
			}

			int32_t Transaction::CreateTable(SQLite::Database & db,bool dropIfExist){
				try
				{
					if(dropIfExist){
						SQLite::Statement query(db,Transaction::Queries::DROP_TRANSACTION_TABLE_QUERY);
						query.executeStep();
					}

					SQLite::Statement query(db,Transaction::Queries::CREATE_TRANSACTION_TABLE_QUERY);
					if(query.executeStep()) {
						return 0;
					}
					else {
						return -1;
					}
				}
				catch(std::exception & e)
				{
					std::printf("Transaction::CreateTable -> Fatal Error %s\n", e.what());
					return -1;
				}

				return 0;
			}

			int32_t Transaction::InsertIntoTable(SQLite::Database & db,const Transaction & toInsert){
				try
				{
					SQLite::Statement query(db,Transaction::Queries::INSERT_INTO_TABLE);
						query.bind(1,toInsert.cust_id);
						query.bind(2,toInsert.user_id);
						query.bind(3,toInsert.gross_amt);
						query.bind(4,toInsert.add_less);
						query.bind(5,toInsert.net_amt);
						query.bind(6,toInsert.date_time);

						query.executeStep();
						
				}
				catch(std::exception & e)
				{
					std::printf("Transaction::InsertIntoTable -> Fatal Error %s\n", e.what());
					return -1;
				}

				return 0;

			}

			const char * User::Queries::GET_NEXT_TRANS_ID_FOR_USER = "select MAX(Trans_ID)+1 as Next_ID from pay_coll_trans where User_ID=?";
			const char * User::Queries::TABLE_NAME = "pay_coll_user";
			const char * User::Queries::GET_ALL_QUERY = "SELECT * FROM pay_coll_user";
			const char * User::Queries::SELECT_USER_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_user WHERE User_ID=?";
			const char * User::Queries::CREATE_USER_TABLE_QUERY = 	"CREATE TABLE pay_coll_user (" 
																	"User_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," 
																	"User_Name 	 VARCHAR(20) NOT NULL UNIQUE," 
																	"Password	  	 VARCHAR(8) NOT NULL );";
			const char * User::Queries::DROP_USER_TABLE_QUERY = "DROP TABLE pay_coll_user IF EXISTS;";

			const char * Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER = "select MAX(Trans_ID)+1 as Next_ID from pay_coll_trans where Cust_ID=?";
			const char * Customer::Queries::GET_ALL_QUERY = "SELECT * FROM pay_coll_cust";
			const char * Customer::Queries::TABLE_NAME = "pay_coll_cust";
			const char * Customer::Queries::SELECT_CUSTOMER_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_cust WHERE Cust_ID=?";
			const char * Customer::Queries::CREATE_CUSTOMER_TABLE_QUERY = 	"CREATE TABLE pay_coll_cust (" 
																	"Cust_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," 
																	"Cust_Name 	 VARCHAR(20) NOT NULL UNIQUE," 
																	"Cust_Contact TEXT NOT NULL,"
																	"Cur_Amt INTEGER NOT NULL,"
																	"Sub_Amt INTEGER NOT NULL,"
																	"Due_Amt INTEGER NOT NULL);";
			const char * Customer::Queries::DROP_CUSTOMER_TABLE_QUERY = "DROP TABLE pay_coll_cust IF EXISTS;";

			const char * Transaction::Queries::GET_ALL_QUERY = "SELECT * FROM pay_coll_trans";
			const char * Transaction::Queries::TABLE_NAME = "pay_coll_trans";
			const char * Transaction::Queries::SELECT_TRANSACTION_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_trans WHERE Trans_ID=?";
			const char * Transaction::Queries::CREATE_TRANSACTION_TABLE_QUERY = 	"CREATE TABLE pay_coll_trans (" 
																	 "Trans_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," 
															     	 "Cust_ID 		 INT   NOT NULL,"
																	 "User_ID	  	 INT	 NOT NULL,"
																	 "Grs_Amt 		 INT	 NOT NULL,"
																	 "Add_Less 		 INT	 NOT NULL,"
																	 "Net_Amt 		 INT	 NOT NULL,"
																	 "Date_Time		 DATE	 NOT NULL, "
															         "FOREIGN KEY (Cust_ID) REFERENCES pay_coll_cust(Cust_ID) ON UPDATE CASCADE,"
															         "FOREIGN KEY (User_ID) REFERENCES pay_coll_cust(User_ID) ON UPDATE CASCADE );";
			const char * Transaction::Queries::DROP_TRANSACTION_TABLE_QUERY = "DROP TABLE pay_coll_trans IF EXISTS;";
			const char * Transaction::Queries::INSERT_INTO_TABLE = "INSERT INTO pay_coll_trans ("
																   " Cust_ID,"
																   " User_ID,"
																   " Grs_Amt,"
																   " Add_Less,"
																   " Net_Amt,"
																   " Date_Time) VALUES ("
																   "?, ?, ?, ?, ?, ?);";
	}
} 
