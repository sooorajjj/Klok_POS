#include "PosDataStructures.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

#include <cstdio>
#include <vector>

namespace klok
{
    namespace pc
    {
        namespace
        {
            using SQLite::Database;
            using SQLite::Column;
            using SQLite::Statement;
        }

        int32_t User::GetAllFromDatabase(SQLite::Database& db, std::vector<User>& outUsers, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, User::Queries::GET_ALL_QUERY);

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
            catch(std::exception& e)
            {
                std::printf("User::GetAllFromDatabase -> Fatal Error query :%s\n %s\n", User::Queries::GET_NEXT_TRANS_ID_FOR_USER, e.what());
                return -1;
            }

            return 0;
        }

        int32_t User::GetNextTransactionIDForUser(SQLite::Database& db, const char* id, std::string& outID)
        {
            try
            {
                SQLite::Statement query(db, User::Queries::GET_NEXT_TRANS_ID_FOR_USER);
                query.bind(1, id);

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
            catch(std::exception& e)
            {
                std::printf("User::GetNextTransactionIDForUser -> Fatal Error query : %s \n %s\n", User::Queries::GET_NEXT_TRANS_ID_FOR_USER, e.what());
                return -1;
            }

            return 0;
        }

        int32_t User::FromDatabase(SQLite::Database& db, const char* id, User& outUser)
        {
            try
            {
                SQLite::Statement query(db, User::Queries::SELECT_USER_WITH_ID_FROM_TABLE);
                query.bind(1, id);

                if(query.executeStep())
                {
                    outUser.id = query.getColumn(0).getString();
                    outUser.name = query.getColumn(1).getString();
                    outUser.password = query.getColumn(2).getString();
                    outUser.company_id = query.getColumn(3).getString();
                    outUser.company_name = query.getColumn(4).getString();
                    outUser.company_address = query.getColumn(5).getString();
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            catch(std::exception& e)
            {
                std::printf("User::FromDatabase -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t User::CreateTable(SQLite::Database& db,bool dropIfExist)
        {
            try
            {
                if(dropIfExist)
                {
                    SQLite::Statement query(db, User::Queries::DROP_USER_TABLE_QUERY);
                    query.executeStep();
                }

                SQLite::Statement query(db, User::Queries::CREATE_USER_TABLE_QUERY);
                if(query.executeStep())
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            catch(std::exception& e)
            {
                std::printf("User::CreateTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t Customer::GetAllFromDatabase(SQLite::Database& db,std::vector<Customer>& outCustomers, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, Customer::Queries::GET_ALL_QUERY);

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

        int32_t Customer::GetNextTransactionIDForCustomer(SQLite::Database& db, const char* id, std::string& outID)
        {
            try
            {
                SQLite::Statement query(db, Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER);
                query.bind(1, id);

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
            catch(std::exception& e)
            {
                std::printf("Customer::GetNextTransactionIDForCustomer -> Fatal Error query : %s \n %s\n", Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Customer::FromDatabase(SQLite::Database& db,const char* id,Customer& outCustomer)
        {

            try
            {
                SQLite::Statement query(db, Customer::Queries::SELECT_CUSTOMER_WITH_ID_FROM_TABLE);
                query.bind(1, id);

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
            catch(std::exception& e)
            {
                std::printf("Customer::FromDatabase -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t Customer::CreateTable(SQLite::Database& db, bool dropIfExist)
        {
            try
            {
                if(dropIfExist)
                {
                    SQLite::Statement query(db, Customer::Queries::DROP_CUSTOMER_TABLE_QUERY);
                    query.executeStep();
                }

                SQLite::Statement query(db, Customer::Queries::CREATE_CUSTOMER_TABLE_QUERY);
                if(query.executeStep())
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            catch(std::exception& e)
            {
                std::printf("Customer::CreateTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t Customer::UpdateCustomerBalance(SQLite::Database & db,const char* id, Customer& toUpdate)

		{
			try
			{
				SQLite::Statement query(db,Customer::Queries::UPDATE_CUSTOMER_BALANCE);
				query.bind(1,toUpdate.cur_amt);
				query.bind(2,toUpdate.id);

				query.executeStep();
             
			}
			catch(std::exception & e)
			{
				std::printf("Customer::UpdateCustomerBalance -> Fatal Error %s\n", e.what());
				return -1;
			}

			return 0;
		}

        int32_t Transaction::GetAllFromDatabase(SQLite::Database& db, std::vector<Transaction>& outTransactions, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, Transaction::Queries::GET_ALL_QUERY);

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
            catch(std::exception& e)
            {
                std::printf("Transaction::GetAllFromDatabase -> Fatal Error query :%s\n %s\n", Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::FromDatabase(SQLite::Database& db, const char* trans_id, Transaction& outTransaction)
        {
            try
            {
                SQLite::Statement query(db, Transaction::Queries::SELECT_TRANSACTION_WITH_ID_FROM_TABLE);
                query.bind(1, trans_id);

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
            catch(std::exception& e)
            {
                std::printf("Transaction::FromDatabase -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::CreateTable(SQLite::Database& db, bool dropIfExist)
        {
            try
            {
                if(dropIfExist)
                {
                    SQLite::Statement query(db, Transaction::Queries::DROP_TRANSACTION_TABLE_QUERY);
                    query.executeStep();
                }

                SQLite::Statement query(db, Transaction::Queries::CREATE_TRANSACTION_TABLE_QUERY);
                if(query.executeStep())
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            catch(std::exception& e)
            {
                std::printf("Transaction::CreateTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::InsertIntoTable(SQLite::Database& db, const Transaction& toInsert)
        {
            try
            {
                SQLite::Statement query(db, Transaction::Queries::INSERT_INTO_TABLE);
                query.bind(1, toInsert.cust_id);
                query.bind(2, toInsert.user_id);
                query.bind(3, toInsert.gross_amt);
                query.bind(4, toInsert.add_less);
                query.bind(5, toInsert.net_amt);
                query.bind(6, toInsert.date_time);

                query.executeStep();

            }
            catch(std::exception& e)
            {
                std::printf("Transaction::InsertIntoTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }
        int32_t Transaction::GetTransactionsForDate(SQLite::Database& db, std::vector<Transaction>& outTransactions, const char * date, uint32_t maxToRead)
        {

        	 try
            {
				SQLite::Statement query(db, Transaction::Queries::GET_ALL_FOR_DATE);
				query.bind(1, date);


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
            catch(std::exception& e)
            {
                std::printf("Transaction::GetTransactionsForDate -> Fatal Error query :%s\n %s\n", Transaction::Queries::GET_ALL_FOR_DATE, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::ListUniqueDates(SQLite::Database& db, std::vector<std::string>& dates_unique, uint32_t maxToRead)
        {
        	try
            {
                SQLite::Statement query(db, Transaction::Queries::LIST_ALL_DATES);

                while(query.executeStep() && (maxToRead--))
                {
                    dates_unique.push_back(query.getColumn(0).getString());
                }

                return 0;

            }
            catch(std::exception& e)
            {
                std::printf("Transaction::⁠⁠⁠ListUniqueDates -> Fatal Error query :%s\n %s\n", Transaction::Queries::LIST_ALL_DATES, e.what());
                return -1;
            }

            return 0;

        }

        int32_t Transaction::GetTransactionsForMonth(SQLite::Database& db, std::vector<Transaction>& outTransactions, const char * month, uint32_t maxToRead)
        {

        	 try
            {
				SQLite::Statement query(db, Transaction::Queries::GET_ALL_FOR_MONTH);
				query.bind(1, month);


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
            catch(std::exception& e)
            {
                std::printf("Transaction::GetTransactionsForMonth -> Fatal Error query :%s\n %s\n", Transaction::Queries::GET_ALL_FOR_MONTH, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::ListUniqueMonths(SQLite::Database& db, std::vector<std::string>& months_unique, uint32_t maxToRead)
        {
        	try
            {
                SQLite::Statement query(db, Transaction::Queries::LIST_ALL_MONTHS);

                while(query.executeStep() && (maxToRead--))
                {
                    months_unique.push_back(query.getColumn(0).getString());
                }

                return 0;

            }
            catch(std::exception& e)
            {
                std::printf("Transaction::⁠⁠⁠ListUniqueMonths -> Fatal Error query :%s\n %s\n", Transaction::Queries::LIST_ALL_MONTHS, e.what());
                return -1;
            }

            return 0;

        }

        int32_t Transaction::GetTransactionsForYear(SQLite::Database& db, std::vector<Transaction>& outTransactions, const char * year, uint32_t maxToRead)
        {

        	try
            {
				SQLite::Statement query(db, Transaction::Queries::GET_ALL_FOR_YEAR);
				query.bind(1, year);


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
            catch(std::exception& e)
            {
                std::printf("Transaction::GetTransactionsForYear -> Fatal Error query :%s\n %s\n", Transaction::Queries::GET_ALL_FOR_YEAR, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::ListUniqueYears(SQLite::Database& db, std::vector<std::string>& years_unique, uint32_t maxToRead)
        {
        	try
            {
                SQLite::Statement query(db, Transaction::Queries::LIST_ALL_YEARS);

                while(query.executeStep() && (maxToRead--))
                {
                    years_unique.push_back(query.getColumn(0).getString());
                }

                return 0;

            }
            catch(std::exception& e)
            {
                std::printf("Transaction::⁠⁠⁠ListUniqueYears -> Fatal Error query :%s\n %s\n", Transaction::Queries::LIST_ALL_YEARS, e.what());
                return -1;
            }

            return 0;

        }
        int32_t Transaction::GetTransactionsForCustomer(SQLite::Database& db, std::vector<Transaction>& outTransactions, const char * customer, uint32_t maxToRead)
        {

        	try
            {
				SQLite::Statement query(db, Transaction::Queries::GET_ALL_FOR_CUSTOMER);
				query.bind(1, customer);


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
            catch(std::exception& e)
            {
                std::printf("Transaction::GetTransactionsForCustomer -> Fatal Error query :%s\n %s\n", Transaction::Queries::GET_ALL_FOR_CUSTOMER, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Transaction::ListUniqueCustomers(SQLite::Database& db, std::vector<std::string>& customers_unique, uint32_t maxToRead)
        {
        	try
            {
                SQLite::Statement query(db, Transaction::Queries::LIST_ALL_CUSTOMERS);

                while(query.executeStep() && (maxToRead--))
                {
                    customers_unique.push_back(query.getColumn(0).getString());
                }

                return 0;

            }
            catch(std::exception& e)
            {
                std::printf("Transaction::ListUniqueCustomers -> Fatal Error query :%s\n %s\n", Transaction::Queries::LIST_ALL_CUSTOMERS, e.what());
                return -1;
            }

            return 0;

        }

        int32_t Product::GetAllFromDatabase(SQLite::Database& db, std::vector<Product>& outProducts, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, Product::Queries::GET_ALL_QUERY);

                while(query.executeStep() && (maxToRead--))
                {
                    Product outProduct;
                    outProduct.id = query.getColumn(0).getString();
                    outProduct.name = query.getColumn(1).getString();
                    outProduct.short_name = query.getColumn(2).getString();
                    outProduct.code = query.getColumn(3).getString();
                    outProduct.sales_rate = query.getColumn(4).getString();
                    outProduct.stock_quantity = query.getColumn(5).getString();
                    outProducts.push_back(outProduct);
                }

                return 0;
            }
            catch(std::exception& e)
            {
                std::printf("Product::GetAllFromDatabase -> Fatal Error query :%s\n %s\n", Product::Queries::GET_ALL_QUERY, e.what());
                return -1;
            }

            return 0;
        }

        int32_t Product::CreateTable(SQLite::Database& db,bool dropIfExist)
        {
            try
            {
                if(dropIfExist)
                {
                    SQLite::Statement query(db, Product::Queries::DROP_PRODUCT_TABLE_QUERY);
                    query.executeStep();
                }

                SQLite::Statement query(db, Product::Queries::DROP_PRODUCT_TABLE_QUERY);
                if(query.executeStep())
                {
                    return 0;
                }
            }
            catch(std::exception& e)
            {
                std::printf("Product::CreateTable -> Fatal Error %s\n", e.what());
            }
                return -1;

        }

        int32_t Product::FromDatabase(SQLite::Database& db, const char* id, Product& outProduct)
        {
            try
            {
                SQLite::Statement query(db, Product::Queries::SELECT_PRODUCT_WITH_ID_FROM_TABLE);
                query.bind(1, id);

                if(query.executeStep())
                {
                    outProduct.id = query.getColumn(0).getString();
                    outProduct.name = query.getColumn(1).getString();
                    outProduct.short_name = query.getColumn(2).getString();
                    outProduct.code = query.getColumn(3).getString();
                    outProduct.sales_rate = query.getColumn(4).getString();
                    return 0;
                }
            }
            catch(std::exception& e)
            {
                std::printf("Product::FromDatabase -> Fatal Error %s\n", e.what());
            }
           return -1;
        }

        int32_t PosBillHeader::GetAllFromDatabase(SQLite::Database& db, std::vector<PosBillHeader>& outPosBillHeaders, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::GET_ALL_QUERY);

                while(query.executeStep() && (maxToRead--))
                {
                    PosBillHeader outPosBillHeader;
                    outPosBillHeader.id = query.getColumn(0).getString();
                    outPosBillHeader.cust_id = query.getColumn(1).getString();
                    outPosBillHeader.user_id = query.getColumn(2).getString();
                    outPosBillHeader.gross_amt = query.getColumn(3).getString();
                    outPosBillHeader.add_less = query.getColumn(4).getString();
                    outPosBillHeader.net_amt = query.getColumn(5).getString();
                    outPosBillHeader.date_time = query.getColumn(6).getString();
                    outPosBillHeader.device_id = query.getColumn(7).getString();
                    outPosBillHeader.unique_items = query.getColumn(8).getString();
                    outPosBillHeader.is_deleted = query.getColumn(9).getString();
                    outPosBillHeader.deleted_at = query.getColumn(10).getString();
                    outPosBillHeaders.push_back(outPosBillHeader);
                }

                return 0;
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::GetAllFromDatabase -> Fatal Error query :%s\n %s\n", PosBillHeader::Queries::GET_ALL_QUERY, e.what());
                return -1;
            }

            return 0;
        }
        int32_t PosBillHeader::GetAllNonDeleted(SQLite::Database& db, std::vector<PosBillHeader>& outPosBillHeaders, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::GET_ALL_NOT_DELETED);

                while(query.executeStep() && (maxToRead--))
                {
                    PosBillHeader outPosBillHeader;
                    outPosBillHeader.id = query.getColumn(0).getString();
                    outPosBillHeader.cust_id = query.getColumn(1).getString();
                    outPosBillHeader.user_id = query.getColumn(2).getString();
                    outPosBillHeader.gross_amt = query.getColumn(3).getString();
                    outPosBillHeader.add_less = query.getColumn(4).getString();
                    outPosBillHeader.net_amt = query.getColumn(5).getString();
                    outPosBillHeader.date_time = query.getColumn(6).getString();
                    outPosBillHeader.device_id = query.getColumn(7).getString();
                    outPosBillHeader.unique_items = query.getColumn(8).getString();
                    outPosBillHeader.is_deleted = query.getColumn(9).getString();
                    outPosBillHeader.deleted_at = query.getColumn(10).getString();
                    outPosBillHeaders.push_back(outPosBillHeader);
                }

                return 0;
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::GetAllNonDeleted -> Fatal Error query :%s\n %s\n", PosBillHeader::Queries::GET_ALL_NOT_DELETED, e.what());
                return -1;
            }

            return 0;
        }


        int32_t PosBillHeader::CreateTable(SQLite::Database& db,bool dropIfExist)
        {
            try
            {
                if(dropIfExist)
                {
                    SQLite::Statement query(db, PosBillHeader::Queries::DROP_POS_BILL_HEADER_TABLE_QUERY);
                    query.executeStep();
                }

                SQLite::Statement query(db, PosBillHeader::Queries::DROP_POS_BILL_HEADER_TABLE_QUERY);
                if(query.executeStep())
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::CreateTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t PosBillHeader::FromDatabase(SQLite::Database& db, const char* id, PosBillHeader& outPosBillHeader)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::SELECT_POS_BILL_HEADER_WITH_ID_FROM_TABLE);
                query.bind(1, id);

                if(query.executeStep())
                {
                    outPosBillHeader.id = query.getColumn(0).getString();
                    outPosBillHeader.cust_id = query.getColumn(1).getString();
                    outPosBillHeader.user_id = query.getColumn(2).getString();
                    outPosBillHeader.gross_amt = query.getColumn(3).getString();
                    outPosBillHeader.add_less = query.getColumn(4).getString();
                    outPosBillHeader.net_amt = query.getColumn(5).getString();
                    outPosBillHeader.date_time = query.getColumn(6).getString();
                    outPosBillHeader.device_id = query.getColumn(7).getString();
                    outPosBillHeader.unique_items = query.getColumn(8).getString();
                    outPosBillHeader.is_deleted = query.getColumn(9).getString();
                    outPosBillHeader.deleted_at = query.getColumn(10).getString();
                    return 0;
                }
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::FromDatabase -> Fatal Error %s\n", e.what());
            }
           return -1;
        }

        int32_t PosBillHeader::InsertIntoTable(SQLite::Database& db, const PosBillHeader& toInsert)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::INSERT_INTO_TABLE);
                query.bind(1, toInsert.cust_id);
                query.bind(2, toInsert.user_id);
                query.bind(3, toInsert.gross_amt);
                query.bind(4, toInsert.add_less);
                query.bind(5, toInsert.net_amt);
                query.bind(6, toInsert.date_time);
                query.bind(7, toInsert.device_id);
                query.bind(8, toInsert.unique_items);
                query.bind(9, toInsert.is_deleted);
                query.bind(10, toInsert.deleted_at);


                query.executeStep();

            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::InsertIntoTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t PosBillHeader::GetLastBillID(SQLite::Database& db, std::string& outID)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::GET_LAST_POS_BILL_HEADER_ID_FROM_TABLE);

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
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::GetLastBillID -> Fatal Error query : %s \n %s\n", PosBillHeader::Queries::GET_LAST_POS_BILL_HEADER_ID_FROM_TABLE, e.what());
                return -1;
            }

            return 0;
        }


        int32_t PosBillHeader::ListUniqueDates(SQLite::Database& db, std::vector<std::string>& dates_unique, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::LIST_ALL_DATES);

                while(query.executeStep() && (maxToRead--))
                {
                    dates_unique.push_back(query.getColumn(0).getString());
                }

                return 0;

            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::⁠⁠⁠ListUniqueDates -> Fatal Error query :%s\n %s\n", PosBillHeader::Queries::LIST_ALL_DATES, e.what());
                return -1;
            }

            return 0;

        }

        int32_t PosBillHeader::GetTransactionsForDate(SQLite::Database& db, std::vector<PosBillHeader>& outBills, const char * date, uint32_t maxToRead)
        {

             try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::GET_ALL_FOR_DATE);
                query.bind(1, date);


                while(query.executeStep() && (maxToRead--))
                {
                    PosBillHeader outBill;
                    outBill.id  = query.getColumn(0).getString();
                    outBill.cust_id   = query.getColumn(1).getString();
                    outBill.user_id   = query.getColumn(2).getString();
                    outBill.gross_amt = query.getColumn(3).getString();
                    outBill.add_less  = query.getColumn(4).getString();
                    outBill.net_amt   = query.getColumn(5).getString();
                    outBill.date_time = query.getColumn(6).getString();
                    outBill.device_id = query.getColumn(7).getString();
                    outBill.unique_items = query.getColumn(8).getString();
                    outBill.is_deleted = query.getColumn(9).getString();
                    outBill.deleted_at = query.getColumn(10).getString();
                    outBills.push_back(outBill);
                }

                return 0;
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::GetTransactionsForDate -> Fatal Error query :%s\n %s\n", PosBillHeader::Queries::GET_ALL_FOR_DATE, e.what());
                return -1;
            }

            return 0;
        }

        int32_t PosBillHeader::DeleteAllFromTable(SQLite::Database& db)
        {

            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::DELETE_ALL_FROM_TABLE);
                SQLite::Statement query1(db, PosBillHeader::Queries::RESET_PRIMARY_KEY);
                query.executeStep();
                query1.executeStep();
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::DeleteAllFromTable -> Fatal Error %s\n%s\n", PosBillHeader::Queries::DELETE_ALL_FROM_TABLE, e.what());
                return -1;
            }
            return 0;
        }


        int32_t PosBillHeader::MarkBillAsDeleted(SQLite::Database& db,const char * bill_id,const char * deletd_at){
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::DELETE_SPECIFIC_BILL);
                query.bind(1, deletd_at);
                query.bind(2, bill_id);
                query.executeStep();
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::MarkBillAsDeleted -> Fatal Error %s\n%s\n", PosBillHeader::Queries::DELETE_SPECIFIC_BILL, e.what());
                return -1;
            }
            return 0;

        }

        int32_t PosBillHeader::ListAllBills(SQLite::Database& db, std::vector<PosBillHeader>& outPosBillHeaders, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, PosBillHeader::Queries::LIST_ALL_BILLS);

                while(query.executeStep() && (maxToRead--))
                {
                    PosBillHeader outPosBillHeader;
                    outPosBillHeader.id = query.getColumn(0).getString();
                    outPosBillHeader.cust_id = query.getColumn(1).getString();
                    outPosBillHeader.user_id = query.getColumn(2).getString();
                    outPosBillHeader.gross_amt = query.getColumn(3).getString();
                    outPosBillHeader.add_less = query.getColumn(4).getString();
                    outPosBillHeader.net_amt = query.getColumn(5).getString();
                    outPosBillHeader.date_time = query.getColumn(6).getString();
                    outPosBillHeader.device_id = query.getColumn(7).getString();
                    outPosBillHeader.unique_items = query.getColumn(8).getString();
                    outPosBillHeader.is_deleted = query.getColumn(9).getString();
                    outPosBillHeader.deleted_at = query.getColumn(10).getString();
                    outPosBillHeaders.push_back(outPosBillHeader);
                }

                return 0;
            }
            catch(std::exception& e)
            {
                std::printf("PosBillHeader::ListAllBills -> Fatal Error query :%s\n %s\n", PosBillHeader::Queries::LIST_ALL_BILLS, e.what());
                return -1;
            }

            return 0;
        }


        int32_t PosBillItem::GetAllFromDatabase(SQLite::Database& db, std::vector<PosBillItem>& outPosBillItems, uint32_t maxToRead)
        {
            try
            {
                SQLite::Statement query(db, PosBillItem::Queries::GET_ALL_QUERY);

                while(query.executeStep() && (maxToRead--))
                {
                    PosBillItem outPosBillItem;
                    outPosBillItem.bill_id = query.getColumn(1).getString();
                    outPosBillItem.product_id = query.getColumn(2).getString();
                    outPosBillItem.quantity = query.getColumn(3).getString();
                    outPosBillItem.net_amt = query.getColumn(4).getString();
                    outPosBillItems.push_back(outPosBillItem);
                }

                return 0;
            }
            catch(std::exception& e)
            {
                std::printf("PosBillItem::GetAllFromDatabase -> Fatal Error query :%s\n %s\n", PosBillItem::Queries::GET_ALL_QUERY, e.what());
                return -1;
            }

            return 0;
        }


        int32_t PosBillItem::CreateTable(SQLite::Database& db,bool dropIfExist)
        {
            try
            {
                if(dropIfExist)
                {
                    SQLite::Statement query(db, PosBillItem::Queries::DROP_POS_BILL_ITEM_TABLE_QUERY);
                    query.executeStep();
                }

                SQLite::Statement query(db, PosBillItem::Queries::DROP_POS_BILL_ITEM_TABLE_QUERY);
                if(query.executeStep())
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            catch(std::exception& e)
            {
                std::printf("PosBillItem::CreateTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t PosBillItem::FromDatabase(SQLite::Database& db, const char* id, PosBillItem& outPosBillItem)
        {
            try
            {
                SQLite::Statement query(db, PosBillItem::Queries::SELECT_POS_BILL_ITEM_WITH_ID_FROM_TABLE);
                query.bind(1, id);

                if(query.executeStep())
                {
                    outPosBillItem.bill_id = query.getColumn(1).getString();
                    outPosBillItem.product_id = query.getColumn(2).getString();
                    outPosBillItem.quantity = query.getColumn(3).getString();
                    outPosBillItem.net_amt = query.getColumn(4).getString();
                    return 0;
                }
            }
            catch(std::exception& e)
            {
                std::printf("PosBillItem::FromDatabase -> Fatal Error %s\n", e.what());
            }
           return -1;
        }

        int32_t PosBillItem::InsertIntoTable(SQLite::Database& db, const PosBillItem& toInsert)
        {
            try
            {
                SQLite::Statement query(db, PosBillItem::Queries::INSERT_INTO_TABLE);
                query.bind(1, toInsert.bill_id);
                query.bind(2, toInsert.product_id);
                query.bind(3, toInsert.quantity);
                query.bind(4, toInsert.net_amt);

                query.executeStep();

            }
            catch(std::exception& e)
            {
                std::printf("PosBillItem::InsertIntoTable -> Fatal Error %s\n", e.what());
                return -1;
            }

            return 0;
        }

        int32_t PosBillItem::DeleteAllFromTable(SQLite::Database& db)
        {

            try
            {
                SQLite::Statement query(db, PosBillItem::Queries::DELETE_ALL_FROM_TABLE);
                SQLite::Statement query1(db, PosBillItem::Queries::RESET_PRIMARY_KEY);
                query.executeStep();
                query1.executeStep();

            }
            catch(std::exception& e)
            {
                std::printf("PosBillItem::DeleteAllFromTable -> Fatal Error %s\n%s\n", PosBillItem::Queries::DELETE_ALL_FROM_TABLE, e.what());
                return -1;
            }
            return 0;
        }


        float PosBillItem::GetTotalSold(SQLite::Database & db,const char * prod){
            float totals = 0;

            SQLite::Statement stmt(db,Queries::GET_TOTAL_SOLD_PER_ITEM);
            try
            {
                if(!stmt.executeStep()){
                    std::printf("Getting total sold failed");
                    return -1;
                }
                else
                {
                    if(!stmt.getColumn(0).isNull()){
                        const std::string val = stmt.getColumn(0).getString();
                        totals = static_cast<float>(atof(val.c_str()));
                    }
                }

            }
            catch(std::exception & exc)
            {
                std::printf("Error PosBillItem::GetTotalSold %s\n" , exc.what());
                return -1;
            }

            return totals;
        }
        // User Queries

        const char * User::Queries::GET_NEXT_TRANS_ID_FOR_USER = "select MAX(Trans_No)+1 as Next_ID from pay_coll_trans where User_ID=?";
        const char * User::Queries::TABLE_NAME = "pay_coll_user";
        const char * User::Queries::GET_ALL_QUERY = "SELECT * FROM pay_coll_user";
        const char * User::Queries::SELECT_USER_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_user WHERE User_ID=?";
        const char * User::Queries::CREATE_USER_TABLE_QUERY =
            "CREATE TABLE pay_coll_user ("
            "User_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "User_Name 	 VARCHAR(20) NOT NULL UNIQUE,"
            "Password	  	 VARCHAR(8) NOT NULL,"
            "Comp_ID        INTEGER NOT NULL,"
            "Comp_Name      VARCHAR(25) NOT NULL,"
            "Comp_Address   VARCHAR(100) NOT NULL);";

        const char * User::Queries::DROP_USER_TABLE_QUERY = "DROP TABLE pay_coll_user IF EXISTS;";

        // Customer Queries

        const char * Customer::Queries::GET_NEXT_TRANS_ID_FOR_CUSTOMER = "select MAX(Trans_No)+1 as Next_ID from pay_coll_trans where Cust_ID=?";
        const char * Customer::Queries::GET_ALL_QUERY = "SELECT * FROM pay_coll_cust";
        const char * Customer::Queries::TABLE_NAME = "pay_coll_cust";
        const char * Customer::Queries::SELECT_CUSTOMER_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_cust WHERE Cust_ID=?";
        const char * Customer::Queries::CREATE_CUSTOMER_TABLE_QUERY =
            "CREATE TABLE pay_coll_cust ("
            "Cust_ID       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "Cust_Name 	 VARCHAR(20) NOT NULL UNIQUE,"
            "Cust_Contact TEXT NOT NULL,"
            "Cur_Amt REAL NOT NULL,"
            "Sub_Amt REAL NOT NULL,"
            "Due_Amt REAL NOT NULL);";

		const char * Customer::Queries::UPDATE_CUSTOMER_BALANCE = "UPDATE pay_coll_cust SET Cur_Amt=? WHERE Cust_ID=?;";
        const char * Customer::Queries::DROP_CUSTOMER_TABLE_QUERY = "DROP TABLE pay_coll_cust IF EXISTS;";

        // Transaction Queries

        const char * Transaction::Queries::GET_ALL_QUERY = "SELECT * FROM pay_coll_trans";
        const char * Transaction::Queries::GET_ALL_FOR_DATE = "SELECT * FROM pay_coll_trans WHERE Date_Time LIKE ? || '\%'";
        const char * Transaction::Queries::LIST_ALL_DATES = "select  distinct substr(Date_Time,0,11) from pay_coll_trans where  Date_Time like '____-__-__\%' ORDER BY Trans_No DESC";
        const char * Transaction::Queries::GET_ALL_FOR_MONTH = "SELECT * FROM pay_coll_trans WHERE Date_Time LIKE ? || '\%'";
        const char * Transaction::Queries::LIST_ALL_MONTHS = "select  distinct substr(Date_Time,0,9) from pay_coll_trans where  Date_Time like '____-__-\%' ORDER BY Trans_No DESC";
        const char * Transaction::Queries::GET_ALL_FOR_YEAR = "SELECT * FROM pay_coll_trans WHERE Date_Time LIKE ? || '\%'";
        const char * Transaction::Queries::LIST_ALL_YEARS = "select  distinct substr(Date_Time,0,6) from pay_coll_trans where  Date_Time like '____-\%' ORDER BY Trans_No DESC";
        const char * Transaction::Queries::GET_ALL_FOR_CUSTOMER = "SELECT * FROM pay_coll_trans WHERE Cust_ID LIKE ? || '\%' ";
        const char * Transaction::Queries::LIST_ALL_CUSTOMERS = "select  distinct substr(Cust_ID,0,7) from pay_coll_trans where  Cust_ID like '______\%' ";
        const char * Transaction::Queries::TABLE_NAME = "pay_coll_trans";
        const char * Transaction::Queries::SELECT_TRANSACTION_WITH_ID_FROM_TABLE = "SELECT * FROM pay_coll_trans WHERE Trans_No=?";

        const char * Transaction::Queries::CREATE_TRANSACTION_TABLE_QUERY =
            "CREATE TABLE pay_coll_trans ("
            "Trans_No       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "Cust_ID 		 INT   NOT NULL,"
            "User_ID	  	 INT	 NOT NULL,"
            "Grs_Amt 		 REAL	 NOT NULL,"
            "Add_Less 		 REAL	 NOT NULL,"
            "Net_Amt 		 REAL	 NOT NULL,"
            "Date_Time		 DATE	 NOT NULL, "
            "FOREIGN KEY (Cust_ID) REFERENCES pay_coll_cust(Cust_ID) ON UPDATE CASCADE,"
            "FOREIGN KEY (User_ID) REFERENCES pay_coll_cust(User_ID) ON UPDATE CASCADE );";

        const char * Transaction::Queries::DROP_TRANSACTION_TABLE_QUERY = "DROP TABLE pay_coll_trans IF EXISTS;";
        const char * Transaction::Queries::INSERT_INTO_TABLE =
            "INSERT INTO pay_coll_trans ("
            " Cust_ID,"
            " User_ID,"
            " Grs_Amt,"
            " Add_Less,"
            " Net_Amt,"
            " Date_Time) VALUES ("
            "?, ?, ?, ?, ?, ?);";

        // Product Queries

        const char * Product::Queries::TABLE_NAME = "pos_product";
        const char * Product::Queries::GET_ALL_QUERY = "SELECT * FROM pos_product";
        const char * Product::Queries::SELECT_PRODUCT_WITH_ID_FROM_TABLE = "SELECT * FROM pos_product WHERE Id=?";
        const char * Product::Queries::DROP_PRODUCT_TABLE_QUERY = "DROP TABLE pos_product IF EXISTS;";

        const char * Product::Queries::CREATE_PRODUCT_TABLE_QUERY =
            "CREATABLE TE pos_product ("
            "Id           INTEGER PRIMARY KEY AUTOINCREMENT,"
            "Name         TEXT,"
            "ShortName    TEXT,"
            "Code         TEXT    UNIQUE,"
            "SalesRate    REAL,    "
            "StockQuantity REAL DEFAULT 0);";

        // POS Bill Header Queries

        const char * PosBillHeader::Queries::TABLE_NAME = "pos_bill_header";
        const char * PosBillHeader::Queries::GET_ALL_QUERY = "SELECT * FROM pos_bill_header";
        const char * PosBillHeader::Queries::SELECT_POS_BILL_HEADER_WITH_ID_FROM_TABLE = "SELECT * FROM pos_bill_header WHERE Id=?";
        const char * PosBillHeader::Queries::DROP_POS_BILL_HEADER_TABLE_QUERY = "DROP TABLE pos_bill_header IF EXISTS;";
        const char * PosBillHeader::Queries::GET_LAST_POS_BILL_HEADER_ID_FROM_TABLE = "select MAX(Id) from pos_bill_header ";
        const char * PosBillHeader::Queries::GET_ALL_FOR_DATE = "SELECT * FROM pos_bill_header WHERE Date_Time LIKE ? || '\%'";
        const char * PosBillHeader::Queries::LIST_ALL_DATES = "select  distinct substr(Date_Time,0,11) from pos_bill_header where  Date_Time like '____-__-__\%' ORDER BY Id DESC";
        const char * PosBillHeader::Queries::DELETE_ALL_FROM_TABLE = "DELETE FROM pos_bill_header";
        const char * PosBillHeader::Queries::RESET_PRIMARY_KEY = "delete from sqlite_sequence where name='pos_bill_header'";
        const char * PosBillHeader::Queries::LIST_ALL_BILLS = "select * from pos_bill_header ORDER BY Id DESC";
        const char * PosBillHeader::Queries::DELETE_SPECIFIC_BILL = "update pos_bill_header set Is_Deleted=1,Delete_AT=? where Id=?";
        const char * PosBillHeader::Queries::GET_ALL_NOT_DELETED = "SELECT * FROM pos_bill_header where Is_Deleted!=1 or Is_Deleted=NULL ORDER BY Id DESC";


        const char * PosBillHeader::Queries::CREATE_POS_BILL_HEADER_TABLE_QUERY =
            "CREATE TABLE pos_bill_header ("
            "Id           INTEGER    NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "Cust_ID         INT     NOT NULL,"
            "User_ID         INT     NOT NULL,"
            "Grs_Amt         REAL     NOT NULL,"
            "Add_Less        REAL     NOT NULL,"
            "Net_Amt         REAL     NOT NULL,"
            "Date_Time       DATE    NOT NULL,"
            "Device_ID       INT     NOT NULL,"
            "Unique_Items    INT     NOT NULL,"
            "Is_Deleted      INT     NOT NULL,"
            "Delete_AT       DATE    NOT NULL,"
            "FOREIGN KEY (Cust_ID) REFERENCES pay_coll_cust(Cust_ID) ON UPDATE CASCADE,"
            "FOREIGN KEY (User_ID) REFERENCES pay_coll_cust(User_ID) ON UPDATE CASCADE );";

        const char * PosBillHeader::Queries::INSERT_INTO_TABLE =
            "INSERT INTO pos_bill_header ("
            " Cust_ID,"
            " User_ID,"
            " Grs_Amt,"
            " Add_Less,"
            " Net_Amt,"
            " Date_Time,"
            " Device_ID,"
            " Unique_Items,"
            " Is_Deleted,"
            " Delete_AT) VALUES ("
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";


        // POS Bill Item Queries

        const char * PosBillItem::Queries::TABLE_NAME = "pos_bill_item";
        const char * PosBillItem::Queries::GET_ALL_QUERY = "SELECT * FROM pos_bill_item";
        const char * PosBillItem::Queries::SELECT_POS_BILL_ITEM_WITH_ID_FROM_TABLE = "SELECT * FROM pos_bill_item WHERE Id=?";
        const char * PosBillItem::Queries::DROP_POS_BILL_ITEM_TABLE_QUERY = "DROP TABLE pos_bill_item IF EXISTS;";
        const char * PosBillItem::Queries::DELETE_ALL_FROM_TABLE = "DELETE FROM pos_bill_item";
        const char * PosBillItem::Queries::RESET_PRIMARY_KEY = "delete from sqlite_sequence where name='pos_bill_item'";

        const char * PosBillItem::Queries::CREATE_POS_BILL_ITEM_TABLE_QUERY =
            "CREATE TABLE pos_bill_item ("
            "Bill_ID         INT     NOT NULL,"
            "Product_ID      INT     NOT NULL,"
            "Quantity        REAL     NOT NULL,"
            "Net_Amt         REAL     NOT NULL,"
            "FOREIGN KEY (Bill_ID) REFERENCES pos_bill_header(Id) ON UPDATE CASCADE,"
            "FOREIGN KEY (Product_ID) REFERENCES pos_product(Product_ID) ON UPDATE CASCADE );";

        const char * PosBillItem::Queries::INSERT_INTO_TABLE =
            "INSERT INTO pos_bill_item ("
            " Bill_ID,"
            " Product_ID,"
            " Quantity,"
            " Net_Amt) VALUES ("
            "?, ?, ?, ?);";

        const char * PosBillItem::Queries::GET_TOTAL_SOLD_PER_ITEM = "select SUM(Quantity) from pos_bill_item where Product_ID=? "
                                                                     "and ((select Is_Deleted from pos_bill_header where Id = "
                                                                     "pos_bill_item.Bill_ID) != '1')";

        // POS Stock Queries

        const char * PosStock::Queries::CREATE_POS_STOCK_TABLE_QUERY =
            "CREATE TABLE pos_stock ("
            "Code         INT     NOT NULL,"
            "Quantity     REAL     NOT NULL,"
            "Updated_ON   INT     NOT NULL);";

    }
}
