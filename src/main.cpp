#include <iostream>
#include <sstream>
#include <ctime>
#include <stdlib.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <map>

#include "PosDataStructures.hpp"
#include "Visiontek.hpp"

extern "C"
{
#include <X6x8.h>
#include <0202lcd.h>
#include <V91magswipe.h>
#include <header.h>
#include <printer.h>
}

namespace
{
	std::string 
		gUserId = "", 
		gUserName = "", 
		gTransId = "", 
		gCustomerId = "", 
		gCustomerName = "", 
		gCustomerBalance = "", 
		gCustomerContact = "",
        gCompanyName = "", 
        gCompanyAddress = "", 
        gProductName = "";

	SQLite::Database* gDatabasePtr = NULL;

	float gBillAmt = 0.00 ;


	std::map<std::string,klok::pc::POSBillEntry> gBillData;
	std::map<std::string,klok::pc::Product> gAllSelectedProducts;

	typedef std::map<std::string,klok::pc::POSBillEntry>::iterator BillIter_t;

}


static void addItemToBill(const klok::pc::Product & product,float quantity){

	if(gBillData.count(product.id) > 0){

		printf("The bill already contains item %s - %s  : %f , changing to %f\n", product.short_name.c_str(),product.code.c_str(),gBillData[product.id].Quantity,quantity);

		if(quantity == 0) gBillData.erase(product.id);
		else gBillData[product.id].Quantity = quantity;

	}
	else
	{

		float salesRateParsed = 0.0f;

		if(std::sscanf(product.sales_rate.c_str() , "%f",&salesRateParsed) == 1){

			klok::pc::POSBillEntry newEntry;
			newEntry.Quantity = quantity;
			newEntry.SalesRate = salesRateParsed;
			gBillData[product.id] = newEntry;

			printf("inserted new item to bill \n");
		}
		else
		{
			printf("parsing price for item failed %s - %s\n",product.short_name.c_str(),product.sales_rate.c_str());
		}


	}
}

void prompt_shutdown()
{
    lcd::DisplayText(3, 0, "press power to shutdown", 1);
    while(true);
}


void display_fatal_error(const char* userError, std::exception& exc)
{
    // call DisplayText
    lk_dispclr();
    lcd::DisplayText(1, 0, userError, 1);

    // Display exc.what()
    std::string err = exc.what();
    lcd::DisplayText(2, 0, err.c_str(), 1);
}

static void closeDatabase()
{
    if(gDatabasePtr != NULL)
    {
        delete gDatabasePtr;
        gDatabasePtr = NULL;
    }
}

static SQLite::Database& getDatabase()
{
    if(gDatabasePtr == NULL)
    {
        try
        {
            gDatabasePtr = new SQLite::Database("/mnt/jffs2/PayCollect.db", SQLite::OPEN_READWRITE, SQLite::OPEN_CREATE);
        }
        catch(std::exception & e)
        {
            display_fatal_error("Database Open Failed", e);
            prompt_shutdown();
        }
    }
    else
    {
        return *gDatabasePtr;
    }
}

const char* getPosCustomerDisplayName(const klok::pc::Customer& inCustomer)
{
    return inCustomer.id.c_str();
}
const char* getPosProductDisplayName(const klok::pc::Product& inProduct)
{
    std::string product_name_list = inProduct.code +" :"+ inProduct.name;

    return product_name_list.c_str();
}


const char* getPosTransactionDisplayName(const klok::pc::Transaction& inTransaction)
{
    return inTransaction.trans_id.c_str();
}
const char* getPosTransactionDatesDisplayName(const std::string & inDate)
{
    return inDate.c_str();
}


std::string getCurrentTime()
{

    time_t now = time(0);
    tm *ltm = localtime(&now);

    char buffer[50] = {0};
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon,
    ltm->tm_mday,1 + ltm->tm_hour, 1 + ltm->tm_min, 1 + ltm->tm_sec);

    return buffer;

}

int returncheck(int r)
{
    switch(r)
    {
    case -1:
        lk_dispclr();
        lcd::DisplayText(2, 5, "device not opened", 1);
        break;

    case -2:
        lk_dispclr();
        lcd::DisplayText(2, 5, "length error", 1);
        break;

    case -3:
        lk_dispclr();
        lcd::DisplayText(2, 5, "NO Paper", 1);
        break;

    case -4:
        lk_dispclr();
        lcd::DisplayText(2, 5, "Low Battery", 1);
        break;

    case -5:
        lk_dispclr();
        lcd::DisplayText(2, 5, "Max temp", 1);
        break;

    case -6:
        lk_dispclr();
        lcd::DisplayText(2, 5, "No Lines", 1);
        break;
    case -7:
        lk_dispclr();
        lcd::DisplayText(2, 5, "WRITE_ERROR", 1);
        break;

    case 0:
        return 0;
    }

    return -1;
}

void insertAndPrint(std::string principleAmtString, std::string addLessString, std::string netAmtString,
                    std::string customerId, std::string userId, std::string transId, std::string customerBalance)
{

    klok::pc::Transaction toInsert;
    toInsert.cust_id = customerId;
    toInsert.user_id = userId;
    toInsert.gross_amt = principleAmtString;
    toInsert.add_less = addLessString;
    toInsert.net_amt = netAmtString;
    toInsert.date_time = getCurrentTime();


    if(klok::pc::Transaction::InsertIntoTable(getDatabase(), toInsert) == 0)
    {

        std::string buff, buff1, buff2, buff3, buff4, buff5;

        int x = lk_getkey();

        if(x == klok::pc::KEYS::KEY_ENTER)
        {
            prn_open();
            if(prn_paperstatus() != 0)
            {
                lk_dispclr();
                lcd::DisplayText(3, 5, "No Paper !", 1);
                lk_getkey();
                return;
            }

            buff.append(" ");
            buff.append(gCompanyName);
            buff1.append("");
            buff1.append(gCompanyAddress);
            buff1.append("\n\n");
            buff2.append("      CASH BILL\n");
            buff3.append("     Bill No             ");
            buff3.append(transId);
            buff3.append("\n");
            buff3.append("     Name                ");
            buff3.append(gCustomerName);
            buff3.append("\n");
            buff3.append("     Contact             ");
            buff3.append(gCustomerContact);
            buff3.append("\n");
            buff3.append("     Gross Amount        ");
            buff3.append(principleAmtString);
            buff3.append("\n");
            buff3.append("     Add/Less            ");
            buff3.append(addLessString);
            buff3.append("\n");
            buff3.append("     -------------------------------\n");
            buff4.append("  CASH       ");
            buff4.append(netAmtString);
            buff4.append("\n");
            buff5.append("     Balance             ");
            buff5.append(customerBalance);
            buff5.append("\n");
            buff5.append("     Billing Username    ");
            buff5.append(gUserName);
            buff5.append("\n");
            buff5.append("     -------------------------------\n");
            buff5.append("          THANK YOU VISIT AGAIN\n");
            buff5.append("           ");
            buff5.append(getCurrentTime());
            buff5.append("\n");


            lk_dispclr();
            lcd::DisplayText(3, 5, "PRINTING BILL", 1);

            int ret;

            ret = printer::WriteText(buff.c_str(), buff.size(), 2);
            returncheck(ret);

            ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
            returncheck(ret);

            ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
            returncheck(ret);

            ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
            returncheck(ret);

            ret = printer::WriteText(buff4.c_str(), buff4.size(), 2);
            returncheck(ret);

			ret = printer::WriteText(buff5.c_str(), buff5.size(), 1);
            returncheck(ret);

            ret = printer::WriteText("\n\n\n", 3, 1);
            returncheck(ret);
            ret = prn_paper_feed(1);
            prn_close();

            if(ret == -3)
            {
                printf("out of the paper");
            }
            else
            {
                return;
            }
        }
        else if(x == klok::pc::KEYS::KEY_CANCEL)
        {
            return;
        }
    }
    else
    {
        printf("failed to InsertIntoTable\n");
    }
}

void updateCustomerBalance(float netAmt, std::string principleAmtString, std::string addLessString, 
						   std::string netAmtString, std::string gCustomerBalance)
{

	klok::pc::Customer toUpdate;

	int scanned = atoi(gCustomerBalance.c_str());
	float currentAmt = scanned + netAmt;

	char customerBalance[10] = {0};
	sprintf(customerBalance ,"%0.2f", currentAmt);

	toUpdate.id = gCustomerId;
	toUpdate.cur_amt = customerBalance;

	std::string display_customerBalance = " Balance    : ";
    display_customerBalance += gCustomerBalance;
    std::string display_principleAmt = " Gross Amt  : ";
    display_principleAmt += principleAmtString;
    std::string display_addLess = " Add/Less   :  ";
    display_addLess += addLessString;
    std::string display_updatedBalance = "Collection  : ";
    display_updatedBalance += customerBalance;

	lk_dispclr();
	lcd::DisplayText(0, 0, display_customerBalance.c_str(), 0);
    lcd::DisplayText(1, 0, display_principleAmt.c_str(), 0);
    lcd::DisplayText(2, 0, display_addLess.c_str(), 0);
    lcd::DisplayText(3, 0, "--------------------", 0);
	lcd::DisplayText(4, 0, display_updatedBalance.c_str(), 0);
	lcd::DisplayText(5, 0, "Press Enter to print", 0);

	insertAndPrint(principleAmtString, addLessString, netAmtString, gCustomerId, gUserId, gTransId, 
	               customerBalance);

	if(klok::pc::Customer::UpdateCustomerBalance(getDatabase(),gCustomerId.c_str(),toUpdate) ==0)

	{
		printf("Getting selected cust_id for user %s is \n", gCustomerId.c_str());
        return;

    }else{
        printf("CustomerId  %s is not available\n", gCustomerId.c_str());
        return;
        
    }
}

void net_amt(float principleAmt, float addLess)
{

    float netAmt = principleAmt + addLess;
    char principleAmtString[30] = {0};
    char addLessString[30] = {0};
    char netAmtString[30] = {0};

    sprintf(principleAmtString, "%0.2f", principleAmt);
    sprintf(addLessString, "%0.2f", addLess);
    sprintf(netAmtString, "%0.2f", netAmt);

    updateCustomerBalance(netAmt, principleAmtString, addLessString, netAmtString, gCustomerBalance);
}

void add_less(float principleAmt)
{

    lk_dispclr();
	lcd::DisplayText(2, 5, "Add/Less", 1);
    lcd::DisplayText(4, 0, "Press F2 to Add, F3 to Less, ENTER to skip", 0);

    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_F2)
    {
    	
        int res = 0;
        char addLess[10]= {0};

        lcd::DisplayText(1, 0, "Type in Amount to ADD", 0);
        res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

        float scanned = 0;

        if(sscanf(addLess, "%f", &scanned) == 1 && res > 0)
        {
            net_amt(principleAmt, scanned);
        }
        else
        {
            lk_dispclr();
            lcd::DisplayText(4, 0, "Enter correct Amt", 0);
            lk_getkey();
        }
    
    }
    else if(x == klok::pc::KEYS::KEY_F3)
    {
    	int res = 0;
        char addLess[10]= {0};

        lcd::DisplayText(1, 0, "Type Amount to LESS", 0);
        res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

        float scanned = 0;

        if(sscanf(addLess, "%f", &scanned) == 1 && res > 0)
        {
            net_amt(principleAmt, -scanned);
        }
        else
        {
            lk_dispclr();
            lcd::DisplayText(4, 0, "Enter correct Amt", 0);
            lk_getkey();
        }
    }else if(x == klok::pc::KEYS::KEY_ENTER)
    {

    	float scanned = 0;
    	net_amt(principleAmt, scanned);
    } 
}

void display_customer_details(const klok::pc::Customer& inCustomer)
{
    lk_dispclr();

    std::string Cust_Name = "Name :" + inCustomer.name;
    lcd::DisplayText(0, 0, Cust_Name.c_str(), 0);
    printf("%s\n", Cust_Name.c_str());

    std::string Cust_Bal = "Balance Amt:" + inCustomer.cur_amt;
    lcd::DisplayText(1, 0, Cust_Bal.c_str(), 0);
    printf("%s\n", Cust_Bal.c_str());

    gCustomerName = inCustomer.name;
    gCustomerBalance = inCustomer.cur_amt;
    gCustomerContact = inCustomer.contact;

   	int res = 0;
    char grossAmt[10] = {0};

    lcd::DisplayText(3, 0, "Gross Amount", 0);
    res = lk_getnumeric(4, 0, (unsigned char*)grossAmt, 10, strlen(grossAmt));
    float scanned =0;

    if(sscanf(grossAmt, "%f", &scanned) == 1 && res > 0)
    {
        add_less(scanned);
    }
    else
    {
        lk_dispclr();
        lcd::DisplayText(4, 0, "Enter correct Amt", 0);
        lk_getkey();
    }

}

void getCustomerDetails(std::string display_transid)
{
    std::vector<klok::pc::Customer> allCustomers;
    if(klok::pc::Customer::GetAllFromDatabase(getDatabase(), allCustomers, 10) == 0)
    {
        for(int i = 0; i != allCustomers.size(); i++)
        {
            printf("CustomerId :%s\n", allCustomers[i].id.c_str());
            printf("CustomerName :%s\n", allCustomers[i].name.c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range_with_title(allCustomers, display_transid.c_str(), 4, res, &getPosCustomerDisplayName);

        if(!res.wasCancelled)
        {
            gCustomerId = allCustomers[res.selectedIndex].id;
            display_customer_details(allCustomers[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to GetAllFromDatabase -> getCustomerDetails \n");
    }
}

void PayCollection()
{
    printf("PayCollection Activity\n");

    std::string Trans_ID = "";
    if(klok::pc::User::GetNextTransactionIDForUser(getDatabase(), gUserId.c_str(), Trans_ID) != 0)
    {
        printf("Getting Trans_ID for user %s failed \n", gUserId.c_str());
        return;
    }
    else if(Trans_ID == "")
    {
        printf("User name  %s is not available\n", gUserId.c_str());
        return;
    }

    gTransId = Trans_ID;


    std::string display_transid = "TransNo :";
    display_transid += Trans_ID;

    printf("Trans_ID :%s\n", Trans_ID.c_str());

    lk_dispclr();
    getCustomerDetails(display_transid);
}

struct BillSummaryDisplayEntry
{

	std::string id,code,short_name;
	klok::pc::POSBillEntry details;
};

static const char * BillSummaryDisplayEntryToString(const BillSummaryDisplayEntry & entry){
	char displayBuffer[30] = {0};
	snprintf(displayBuffer,sizeof(displayBuffer) - 1 , "%s:%s - %.02f" ,entry.code.c_str(),entry.short_name.c_str(), entry.details.Quantity); 
	std::string display = displayBuffer;
	return display.c_str();
}

void edit_product_quantity(const BillSummaryDisplayEntry & entry)
{

	lk_dispclr();
	int res = 0;

	char qty[10] = {0};
	float scanned =0;

	lcd::DisplayText(2, 0, "Enter Quantity", 0);
	res = lk_getnumeric(4, 0, (unsigned char*)qty, 10, strlen(qty));
    if(sscanf(qty, "%f", &scanned) == 1 && res > 0)
    {
    	if(scanned == 0)
    	{
			gBillData.erase(entry.id);
			gAllSelectedProducts.erase(entry.id);

			printf("removing %s from bill\n", entry.short_name.c_str());
		}
		else
		{
			printf("updating %s from bill\n", entry.short_name.c_str());
			gBillData[entry.id] .Quantity = scanned;
		}
    }

}
float add_less_pos()
{

    lk_dispclr();
	lcd::DisplayText(2, 5, "Add/Less", 1);
    lcd::DisplayText(4, 0, "Press F2 to Add, F3 to Less, ENTER to skip", 0);

    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_F2)
    {
    	
        int res = 0;
        char addLess[10]= {0};

        lcd::DisplayText(1, 0, "Type in Amount to ADD", 0);
        res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

        float scanned = 0;

        if(sscanf(addLess, "%f", &scanned) == 1 && res > 0)
        {
            return scanned;
        }
        else
        {
            lk_dispclr();
            lcd::DisplayText(4, 0, "Enter correct Amt", 0);
            lk_getkey();
        }
    
    }
    else if(x == klok::pc::KEYS::KEY_F3)
    {
    	int res = 0;
        char addLess[10]= {0};

        lcd::DisplayText(1, 0, "Type Amount to LESS", 0);
        res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

        float scanned = 0;

        if(sscanf(addLess, "%f", &scanned) == 1 && res > 0)
        {
            return -scanned;
        }
        else
        {
            lk_dispclr();
            lcd::DisplayText(4, 0, "Enter correct Amt", 0);
            lk_getkey();
        }
    }else if(x == klok::pc::KEYS::KEY_ENTER)
    {

        return 0;
    } 

   return 0;
}


template <typename T> std::string tostr(const T& t) { 
   std::ostringstream os; 
   os<<t; 
   return os.str(); 
} 

void display_bill_summary()
{

	lk_dispclr();

	gBillAmt = 0;
	gAllSelectedProducts.clear();







    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

	std::vector<BillSummaryDisplayEntry> productDisplayList;
    while(true)
    {
		productDisplayList.clear();

		gBillAmt = 0;
		for(BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter)
		{

			klok::pc::Product selected;

			if(klok::pc::Product::FromDatabase(getDatabase(),iter->first.c_str(),selected) == 0)
			{
				BillSummaryDisplayEntry newEntry;
				newEntry.id = selected.id;
				newEntry.code = selected.code;
				newEntry.short_name = selected.short_name;
				newEntry.details = iter->second;
				productDisplayList.push_back(newEntry);
			}


			gBillAmt += iter->second.SalesRate * iter->second.Quantity;
			printf("%s - { %f,%f }\n", iter->first.c_str() , iter->second.SalesRate , iter->second.Quantity);
		}

	    char totalAmountLabel[30] = {0};
	    snprintf(totalAmountLabel,sizeof(totalAmountLabel) - 1 , "Total Amt : %.02f" , gBillAmt);

	    klok::pc::display_sub_range_with_title(productDisplayList, totalAmountLabel, 4, res, &BillSummaryDisplayEntryToString);

	    if(!res.wasCancelled){


	    	edit_product_quantity(productDisplayList[res.selectedIndex]);
	    	if(gBillData.count(productDisplayList[res.selectedIndex].id) == 0){

	    		productDisplayList.erase(productDisplayList.begin() + res.selectedIndex );

	    		if(!productDisplayList.size()) return;

    			gBillAmt = 0;

    			for(BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter)
				{
					gBillAmt += iter->second.SalesRate * iter->second.Quantity;
					printf("%s - { %f,%f }\n", iter->first.c_str() , iter->second.SalesRate , iter->second.Quantity);
				}

	    	}


	    }
	    else if( klok::pc::KEYS::KEY_F6 == res.lastSpecialKey )
	    {
	    	const float add_less_amt = add_less_pos();

			gBillAmt = 0;

			for(BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter)
			{
				gBillAmt += iter->second.SalesRate * iter->second.Quantity;
				printf("%s - { %f,%f }\n", iter->first.c_str() , iter->second.SalesRate , iter->second.Quantity);
			}

	    	klok::pc::PosBillHeader header;

            header.cust_id = "101001";
            header.gross_amt = tostr(gBillAmt);
            header.add_less = tostr(add_less_amt);
            header.net_amt = tostr(gBillAmt + add_less_amt);
            header.date_time = getCurrentTime();
            header.user_id = gUserId;
            header.device_id = "KLOK01";
            header.unique_items = tostr(gBillData.size());


		    if(klok::pc::PosBillHeader::InsertIntoTable(getDatabase(), header) == 0)
		    {
		    	std::string newBillId = "";
		    	if(klok::pc::PosBillHeader::GetLastBillID(getDatabase(), newBillId ) != 0)
			    {
			        printf("newBillId  is not fetch failed\n");
			        return;
			    }
			    else if(newBillId == "")
			    {
			        printf("newBillId  is not available\n");
			        return;
			    }

			for(BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter)
			{

				klok::pc::PosBillItem item;
				item.bill_id = newBillId;
				item.product_id = tostr(iter->first);
				item.quantity = tostr(iter->second.Quantity);
				item.net_amt = tostr(iter->second.SalesRate);

				if(klok::pc::PosBillItem::InsertIntoTable(getDatabase(),item)!= 0 ){
					printf("adding item to database failed for bill %s\n", newBillId.c_str());
					return;
				}
				else{
					printf("adding item  {%s} to database for bill %s\n",item.product_id.c_str(),newBillId.c_str());
				}



			}

	        lcd::DisplayText(5, 0, "Press Enter to print", 0);

			std::string buff, buff1, buff2, buff3, buff4, buff5,buffx;

	        int x = lk_getkey();

	        if(x == klok::pc::KEYS::KEY_ENTER)
	        {
	            prn_open();
	            if(prn_paperstatus() != 0)
	            {
	                lk_dispclr();
	                lcd::DisplayText(3, 5, "No Paper !", 1);
	                lk_getkey();
	                return;
	            }

	            buff.append(" ");
	            buff.append(gCompanyName);
	            buff1.append("");
	            buff1.append(gCompanyAddress);
	            buff1.append("\n\n");
	            buff2.append("      CASH BILL\n");
	            buff3.append("     Bill No             ");
	            buff3.append(newBillId);
	            buff3.append("\n");
	            buff3.append("     Name                ");
	            buff3.append("Customer 1\n");
	            buff3.append("     -------------------------------\n");

				for(int i = 0; i != productDisplayList.size(); i++)
				{

		            buff3.append("     ");
		            buff3.append(productDisplayList[i].code + " - " + productDisplayList[i].short_name + " - " +
		            tostr(productDisplayList[i].details.Quantity) + " X " + tostr(productDisplayList[i].details.SalesRate) + " : " +
		             tostr(productDisplayList[i].details.Quantity*productDisplayList[i].details.SalesRate) );
		            buff3.append("\n");

				}

	            buff3.append("     -------------------------------\n");
	            buff3.append("\n");
	            buff3.append("     Gross Amount        " + tostr(gBillAmt));
	            buff3.append("\n");
	           buffx.append("     Add/Less            " + tostr(add_less_amt));
	           buffx.append("\n");
	            buff3.append("     -------------------------------\n");
	            buff4.append("  CASH       ");
	            buff4.append(tostr(gBillAmt + add_less_amt));
	            buff4.append("\n");
	            buff5.append("     Billing Username    ");
	            buff5.append(gUserName);
	            buff5.append("\n");
	            buff5.append("     -------------------------------\n");
	            buff5.append("          THANK YOU VISIT AGAIN\n");
	            buff5.append("           ");
	            buff5.append(getCurrentTime());
	            buff5.append("\n");


	            lk_dispclr();
	            lcd::DisplayText(3, 5, "PRINTING BILL", 1);

	            int ret;

	            ret = printer::WriteText(buff.c_str(), buff.size(), 2);
	            returncheck(ret);

	            ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
	            returncheck(ret);

	            ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
	            returncheck(ret);

	            ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
	            returncheck(ret);

	            ret = printer::WriteText(buff4.c_str(), buff4.size(), 2);
	            returncheck(ret);

				ret = printer::WriteText(buff5.c_str(), buff5.size(), 1);
	            returncheck(ret);

	            ret = printer::WriteText("\n\n\n", 3, 1);
	            returncheck(ret);
	            ret = prn_paper_feed(1);
	            prn_close();


            gBillAmt = 0;
            gBillData.clear();
            gAllSelectedProducts.clear();

	            if(ret == -3)
	            {
	                printf("out of the paper");
	            }
	            else
	            {
	                return;
	            }
	        }
	        else if(x == klok::pc::KEYS::KEY_CANCEL)
	        {
	            return;
	        }




		    }
		    else
		    {
    	        printf("failed to InsertIntoTable\n");
		    }








            printf("bill saved\n");

            return;





	    }
	    else
	    {
	    	return;
	    }

	}

}

void ask_product_quantity(const klok::pc::Product& inProduct)
{
	lk_dispclr();
	int res = 0;

	char qty[10] = {0};
	float scanned =0;

	lcd::DisplayText(2, 0, "Enter Quantity", 0);
	res = lk_getnumeric(4, 0, (unsigned char*)qty, 10, strlen(qty));
    if(sscanf(qty, "%f", &scanned) == 1 && res > 0)
    	{
    		printf("Quantity Entered%f \n", scanned);
    		addItemToBill(inProduct,scanned);

    		gBillAmt = 0;
    		for(BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter){
    			gBillAmt += iter->second.SalesRate * iter->second.Quantity;
    			printf("%s - { %f,%f }\n", iter->first.c_str() , iter->second.SalesRate , iter->second.Quantity);
    		}
    	}

}

void POS()
{
    printf("POS Activity\n");
    lk_dispclr();
    gBillData.clear();
    std::vector<klok::pc::Product> allProducts;
    if(klok::pc::Product::GetAllFromDatabase(getDatabase(), allProducts, 500) == 0)
    {

    	while(1)
    	{

	    	// Debug
	        for(int i = 0; i != allProducts.size(); i++)
	        {
	            printf("ProductId :%s\n", allProducts[i].id.c_str());
	            printf("ProductName :%s\n", allProducts[i].name.c_str());
	        }

	        klok::pc::MenuResult res;
	        res.wasCancelled = false;
	        res.selectedIndex = -1;

	        char totalAmountLabel[30] = {0};
	        snprintf(totalAmountLabel,sizeof(totalAmountLabel) - 1 , "Total Amt : %.02f" , gBillAmt);

	        klok::pc::display_sub_range_with_title(allProducts, totalAmountLabel, 4, res, &getPosProductDisplayName);

	        if(!res.wasCancelled)
	        {
	            gProductName = allProducts[res.selectedIndex].name;
	            ask_product_quantity(allProducts[res.selectedIndex]);
	        }
	        else if(klok::pc::KEYS::KEY_F6 == res.lastSpecialKey)
	        {
	        	printf("User wants to proceed to billing\n");
	        	display_bill_summary();
	        	printf("User finished with billing\n");

	        }
	        else
	        {
	        	break;
	        }
	    }

    }
    else
    {
        printf("failed to GetAllFromDatabase -> getProductDetails \n");
    }
}

void Billing()
{
    printf("Billing\n");

    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 2;
        strcpy(menu.menu[0],"Pay Collection");
        strcpy(menu.menu[1],"POS");

        while(1)
        {
            lk_dispclr();

            opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

            switch(opt)
            {
            case CANCEL:
                return;

            case ENTER:
                switch(selItem + 1)
                {
                case 1:
                    PayCollection();
                    break;
                case 2:
                    POS();
                    break;
                }
                break;
            }
        }
    }
}

void getDateWiseDetails(std::string date)
{
    std::vector<klok::pc::Transaction> allTransactions;
    if(klok::pc::Transaction::GetTransactionsForDate(getDatabase(), allTransactions, date.c_str(), 20) == 0)
    {
    	std::string transDate = " Report on " + date;
        prn_open();
        std::string buff, buff1;

        buff.append(transDate);
        buff.append("\n\n");
        
        int ret;
        ret = printer::WriteText(buff.c_str(), buff.size(), 2);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
		    }
		}

        float totaAmt = 0;
		float parsedNetAmt = 0;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

            std::string buff2;

            buff2.append("    Bill No          ");
            buff2.append(allTransactions[i].trans_id);
            buff2.append("\n");
            buff2.append("    ID               ");
            buff2.append(allTransactions[i].cust_id);
            buff2.append("\n");
            buff2.append("    DATE AND TIME    ");
            buff2.append(allTransactions[i].date_time);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");
            buff2.append("    CASH             ");
            buff2.append(allTransactions[i].net_amt);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");

	        sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
	        parsedNetAmt += totaAmt;

            int ret;

            ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
            if(ret == -3)
	        {
		   		while(prn_paperstatus() != 0)
			    {
			        lk_dispclr();
			        lcd::DisplayText(3, 5, "No Paper !", 1);
			        int x = lk_getkey();
			        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
			        {
			            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
			                break;            
			        }
			        else if (x == klok::pc::KEYS::KEY_CANCEL) 
			        {
			        return;
			    	}
			    }
			}
        }
		
		char totaAmtString [30] = {0};
	    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

        buff1.append("    TOTAL             ");
        buff1.append(totaAmtString);
	    buff1.append("\n");
        buff1.append("     -------------------------------\n");

        ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
			}
		}

        ret = prn_paper_feed(1);
        prn_close();

    }
    else
    {
        printf("failed to GetTransactionsForDate -> getDateWiseDetails \n");
    }
}

void EnteringDate()
{

	int res = 0;

	lk_dispclr();
    lcd::DisplayText(1, 0, "Enter Date ", 0);
    lcd::DisplayText(3, 0, "Format : YYYY.MM.DD ", 0);

    char typedBuffer[12] = {0};
    res = lk_getalpha(4, 0, (unsigned char*)typedBuffer, 12, strlen(typedBuffer), 0);

    std::string asString = typedBuffer;
    for(int i = 0; i < asString.size(); i++)
    {
    	if (asString[i]=='.')
    	{
    		asString[i] = '-';
    	}
    }
    if(res > 0)
    {

        printf("Transactions on %s %d\n", asString.c_str(), res);

        std::vector<klok::pc::Transaction> transObj;
        if(klok::pc::Transaction::GetTransactionsForDate(getDatabase(), transObj, asString.c_str(), 20)==0)
        {
        	getDateWiseDetails(asString);            
        }
    }
}

void ListDates()
{
	std::vector<std::string> datesUnique;
    if(klok::pc::Transaction::ListUniqueDates(getDatabase(), datesUnique, 20) == 0)
    {
    	for(int i = 0; i != datesUnique.size(); i++)
        {

            printf("Transaction On :%s\n", datesUnique[i].c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range(datesUnique, 5, res, &getPosTransactionDatesDisplayName);

        if(!res.wasCancelled)
        {
            getDateWiseDetails(datesUnique[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to ListUniqueDates -> ListDates \n");
    }
}

void DailyCollectionReport()
{
    printf("DailyCollectionReport Activity\n");

    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 2;
        strcpy(menu.menu[0],"By Entering Date");
        strcpy(menu.menu[1],"List Dates");

        while(1)
        {
            lk_dispclr();

            opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

            switch(opt)
            {
            case CANCEL:
                return;

            case ENTER:
                switch(selItem + 1)
                {
                case 1:
                    EnteringDate();
                    break;

                case 2:
                    ListDates();
                    break; 
                }
                break;
            }
        }
    }      
}

void getMonthWiseDetails(std::string month)
{
    std::vector<klok::pc::Transaction> allTransactions;
    if(klok::pc::Transaction::GetTransactionsForMonth(getDatabase(), allTransactions, month.c_str(), 20) == 0)
    {
    	std::string transMonth = "  Report on " + month;
        prn_open();
        std::string buff, buff1;

        buff.append(transMonth);
        buff.append("\n\n");
        
        int ret;
        ret = printer::WriteText(buff.c_str(), buff.size(), 2);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
		    }
		}

        float totaAmt = 0;
		float parsedNetAmt = 0;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

            std::string buff2;

            buff2.append("    Bill No          ");
            buff2.append(allTransactions[i].trans_id);
            buff2.append("\n");
            buff2.append("    ID               ");
            buff2.append(allTransactions[i].cust_id);
            buff2.append("\n");
            buff2.append("    DATE AND TIME    ");
            buff2.append(allTransactions[i].date_time);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");
            buff2.append("    CASH             ");
            buff2.append(allTransactions[i].net_amt);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");

	        sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
	        parsedNetAmt += totaAmt;

            int ret;

            ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
            if(ret == -3)
	        {
		   		while(prn_paperstatus() != 0)
			    {
			        lk_dispclr();
			        lcd::DisplayText(3, 5, "No Paper !", 1);
			        int x = lk_getkey();
			        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
			        {
			            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
			                break;            
			        }
			        else if (x == klok::pc::KEYS::KEY_CANCEL) 
			        {
			        return;
			    	}
			    }
			}
        }
		
		char totaAmtString [30] = {0};
	    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

        buff1.append("    TOTAL             ");
        buff1.append(totaAmtString);
	    buff1.append("\n");
        buff1.append("     -------------------------------\n");

        ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
			}
		}

        ret = prn_paper_feed(1);
        prn_close();
    }
    else
    {
        printf("failed to GetTransactionsForMonth -> getMonthWiseDetails \n");
    }
}

void ListMonths()
{
	std::vector<std::string> monthsUnique;
    if(klok::pc::Transaction::ListUniqueMonths(getDatabase(), monthsUnique, 20) == 0)
    {
    	for(int i = 0; i != monthsUnique.size(); i++)
        {

            printf("Transaction No :%s\n", monthsUnique[i].c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range(monthsUnique, 5, res, &getPosTransactionDatesDisplayName);

        if(!res.wasCancelled)
        {
            getMonthWiseDetails(monthsUnique[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to ListUniqueMonths -> ListMonths \n");
    }
}

void getYearWiseDetails(std::string year)
{
    std::vector<klok::pc::Transaction> allTransactions;
    if(klok::pc::Transaction::GetTransactionsForYear(getDatabase(), allTransactions, year.c_str(), 20) == 0)
    {
    	std::string transYear = "   Report on " + year;
        prn_open();
        std::string buff,buff1;

        buff.append(transYear);
        buff.append("\n\n");
        
        int ret;
        ret = printer::WriteText(buff.c_str(), buff.size(), 2);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
		    }
		}

        float totaAmt = 0;
		float parsedNetAmt = 0;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

            std::string buff2;

            buff2.append("    Bill No          ");
            buff2.append(allTransactions[i].trans_id);
            buff2.append("\n");
            buff2.append("    ID               ");
            buff2.append(allTransactions[i].cust_id);
            buff2.append("\n");
            buff2.append("    DATE AND TIME    ");
            buff2.append(allTransactions[i].date_time);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");
            buff2.append("    CASH             ");
            buff2.append(allTransactions[i].net_amt);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");

	        sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
	        parsedNetAmt += totaAmt;

            int ret;

            ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
            if(ret == -3)
	        {
		   		while(prn_paperstatus() != 0)
			    {
			        lk_dispclr();
			        lcd::DisplayText(3, 5, "No Paper !", 1);
			        int x = lk_getkey();
			        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
			        {
			            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
			                break;            
			        }
			        else if (x == klok::pc::KEYS::KEY_CANCEL) 
			        {
			        return;
			    	}
			    }
			}
        }
		
		char totaAmtString [30] = {0};
	    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

        buff1.append("    TOTAL             ");
        buff1.append(totaAmtString);
	    buff1.append("\n");
        buff1.append("     -------------------------------\n");

        ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
			}
		}

        ret = prn_paper_feed(1);
        prn_close();
    }
    else
    {
        printf("failed to GetTransactionsForYear -> getYearWiseDetails \n");
    }
}

void ListYears()
{
	std::vector<std::string> yearsUnique;
    if(klok::pc::Transaction::ListUniqueYears(getDatabase(), yearsUnique, 20) == 0)
    {
    	for(int i = 0; i !=yearsUnique.size(); i++)
        {

            printf("Transaction No :%s\n", yearsUnique[i].c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range(yearsUnique, 5, res, &getPosTransactionDatesDisplayName);

        if(!res.wasCancelled)
        {
            getYearWiseDetails(yearsUnique[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to ListUniqueYears -> ListYears \n");
    }
}

void ConsolidatedReport()
{
    printf("ConsolidatedReport Activity\n");

    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 3;
        strcpy(menu.menu[0],"Daily");
        strcpy(menu.menu[1],"Monthly");
        strcpy(menu.menu[2],"Yearly");


        while(1)
        {
            lk_dispclr();

            opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

            switch(opt)
            {
            case CANCEL:
                return;

            case ENTER:
                switch(selItem + 1)
                {
                case 1:
                    ListDates();
                    break;

                case 2:
                    ListMonths();
                    break;

                case 3:
                    ListYears();
                    break;
                }
                break;
            }
        }
    }
}

void getCustomerWiseDetails(std::string customer)
{

    lk_dispclr();

	std::string transCustomer = "  Report of " + customer;
	lcd::DisplayText(1, 0, transCustomer.c_str(), 0);
    lcd::DisplayText(3, 0, "Press ENTER to print , else press CANCEL", 0);

    int x = lk_getkey();

    if(x == klok::pc::KEYS::KEY_ENTER)
    {
        std::vector<klok::pc::Transaction> allTransactions;
	    if(klok::pc::Transaction::GetTransactionsForCustomer(getDatabase(), allTransactions, customer.c_str(), 20) == 0)
	    {
	        prn_open();
	        std::string buff, buff1;

	        buff.append(transCustomer);
	        buff.append("\n\n");
	        
	        int ret;
	        ret = printer::WriteText(buff.c_str(), buff.size(), 2);
	        if(ret == -3)
	        {
		   		while(prn_paperstatus() != 0)
			    {
			        lk_dispclr();
			        lcd::DisplayText(3, 5, "No Paper !", 1);
			        int x = lk_getkey();
			        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
			        {
			            if(printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
			                break;            
			        }
			        else if (x == klok::pc::KEYS::KEY_CANCEL) 
			        {
			        return;
			    	}
			    }
			}

			float totaAmt = 0;
		float parsedNetAmt = 0;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

            std::string buff2;

            buff2.append("    Bill No          ");
            buff2.append(allTransactions[i].trans_id);
            buff2.append("\n");
            buff2.append("    User ID          ");
            buff2.append(allTransactions[i].user_id);
            buff2.append("\n");
            buff2.append("    DATE AND TIME    ");
            buff2.append(allTransactions[i].date_time);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");
            buff2.append("    CASH             ");
            buff2.append(allTransactions[i].net_amt);
            buff2.append("\n");
            buff2.append("     -------------------------------\n");

	        sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
	        parsedNetAmt += totaAmt;

            int ret;

            ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
            if(ret == -3)
	        {
		   		while(prn_paperstatus() != 0)
			    {
			        lk_dispclr();
			        lcd::DisplayText(3, 5, "No Paper !", 1);
			        int x = lk_getkey();
			        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
			        {
			            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
			                break;            
			        }
			        else if (x == klok::pc::KEYS::KEY_CANCEL) 
			        {
			        return;
			    	}
			    }
			}
        }
		
		char totaAmtString [30] = {0};
	    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

        buff1.append("    TOTAL             ");
        buff1.append(totaAmtString);
	    buff1.append("\n");
        buff1.append("     -------------------------------\n");

        ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
        if(ret == -3)
        {
	   		while(prn_paperstatus() != 0)
		    {
		        lk_dispclr();
		        lcd::DisplayText(3, 5, "No Paper !", 1);
		        int x = lk_getkey();
		        if(x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0 )
		        {
		            if(printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
		                break;            
		        }
		        else if (x == klok::pc::KEYS::KEY_CANCEL) 
		        {
		        return;
		    	}
			}
		}

        ret = prn_paper_feed(1);
        prn_close();

	    }
        else
	    {
	        printf("failed to GetTransactionsForCustomer -> getCustomerWiseDetails \n");
	    }
    }
    else if(x == klok::pc::KEYS::KEY_CANCEL)
    {
        printf("pressed cancel while display_customer_details\n");
    }
}


void CustomerWiseReport()
{
	std::vector<std::string> customersUnique;
    if(klok::pc::Transaction::ListUniqueCustomers(getDatabase(), customersUnique, 20) == 0)
    {
        for(int i = 0; i != customersUnique.size(); i++)
        {
            printf("Customer Id :%s\n", customersUnique[i].c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range(customersUnique, 5, res, &getPosTransactionDatesDisplayName);

        if(!res.wasCancelled)
        {
            getCustomerWiseDetails(customersUnique[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to GetAllFromDatabase -> getCustomerDetails \n");
    }
}

void POS_Daily_Report(){
	std::vector<std::string> uniqueDates;

	if(klok::pc::PosBillHeader::ListUniqueDates(getDatabase(),uniqueDates,100) == 0){

		klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range(uniqueDates, 5, res, &getPosTransactionDatesDisplayName);

        if(!res.wasCancelled)
        {
        	std::string dateToQuery = uniqueDates[res.selectedIndex];
		

			float dailyTotal = 0;
			std::vector<klok::pc::PosBillHeader> billsForDate;
			if(klok::pc::PosBillHeader::GetTransactionsForDate(getDatabase(),billsForDate ,dateToQuery.c_str(),100) == 0)
			{

			int x = lk_getkey();

	        if(x == klok::pc::KEYS::KEY_ENTER)
	        {

					std::string buff, buff1, buff2, buff3, buff4, buff5, buffx;
		            prn_open();
		            if(prn_paperstatus() != 0)
		            {
		                lk_dispclr();
		                lcd::DisplayText(3, 5, "No Paper !", 1);
		                lk_getkey();
		                return;
		            }

		        
		        

		            buff.append(" ");
		            buff.append(gCompanyName);
		            buff1.append("");
		            buff1.append(gCompanyAddress);
		            buff1.append("\n\n");
		            buff2.append("      Daily Report\n");
		            buff3.append("     Report Date         :");
		            buff3.append(dateToQuery);
		            buff3.append("\n");
		            buff3.append("     -------------------------------\n");

		            lk_dispclr();
		            lcd::DisplayText(3, 5, "PRINTING BILL", 1);
					for (int i = 0; i < billsForDate.size(); ++i)
					{
						float net_amt_for_bill = 0;

						sscanf(billsForDate[i].net_amt.c_str(),"%f",&net_amt_for_bill);

						dailyTotal += net_amt_for_bill;

						buff3.append(" " + billsForDate[i].id + "  " + billsForDate[i].date_time + "  :  " + billsForDate[i].net_amt + '\n');

			        }

			        buffx.append("     -------------------------------\n");

			        buffx.append(std::string("    TOTAL          :Rs ")) ;
			        	buffx.append(tostr(dailyTotal));
		            int ret;

		            ret = printer::WriteText(buff.c_str(), buff.size(), 2);
		            returncheck(ret);

		            ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
		            returncheck(ret);

		            ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
		            returncheck(ret);

		            ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
		            returncheck(ret);

		            ret = printer::WriteText(buffx.c_str(), buffx.size(), 1);
		            returncheck(ret);

		            ret = printer::WriteText(buff4.c_str(), buff4.size(), 2);
		            returncheck(ret);

					ret = printer::WriteText(buff5.c_str(), buff5.size(), 1);
		            returncheck(ret);

		            ret = printer::WriteText("\n\n\n", 3, 1);
		            returncheck(ret);
		            ret = prn_paper_feed(1);
		            prn_close();

		            if(ret == -3)
		            {
		                printf("out of the paper");
		            }
		            else
		            {
		                return;
		            }

			        }
			    

	        else if(x == klok::pc::KEYS::KEY_CANCEL)
	        {
	            return;
	        }

			}
			}

        

		



	}


}


void Reports()
{
    printf("Reports\n");

    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 4;
        strcpy(menu.menu[0],"Daily Collection Report");
        strcpy(menu.menu[1],"Consolidated Report");
        strcpy(menu.menu[2],"Customer Wise Report");
        strcpy(menu.menu[3],"POS Day Report");

        while(1)
        {
            lk_dispclr();

            opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

            switch(opt)
            {
            case CANCEL:
                return;

            case ENTER:
                switch(selItem + 1)
                {
                case 1:
                    DailyCollectionReport();
                    break;

                case 2:
                    ConsolidatedReport();
                    break;

                case 3:
                    CustomerWiseReport();
                    break;
                case 4:
                    POS_Daily_Report();
                    break;
                }
                break;
            }
        }
    }
}

void Export()
{
	lk_dispclr();
	lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
	printf("Export Activity\n");

	int ret=0;
	FILE *fp;
    	
	fp=fopen("/etc/mtab", "r");
	char str[100]="", flag=0;

		if (fp==NULL)
		fprintf(stderr, "File open Error\n");

		 while((fgets(str, 80, fp))!=NULL)
		{
       		if((strstr(str, "/mnt/usb")) != NULL)
			flag=1;
		}

	fclose(fp);

        int x = lk_getkey();

        if(x == klok::pc::KEYS::KEY_ENTER && flag==1)
        {
        	lk_dispclr();
			lcd::DisplayText(2, 2, "Already Mounted", 1);
			lcd::DisplayText(4, 0, "Press any key to copy database to USB", 0);

				lk_getkey();
				ret=system("cp /mnt/jffs2/PayCollect.db /mnt/usb/");

				lk_dispclr();
				lcd::DisplayText(3,2,"Copying ....",0);
				lcd::DisplayText(5,0,"This may take 5-8 seconds",0);
				sleep(6);
				
					if (ret == 0)
					{
					lk_dispclr();
					lcd::DisplayText(3,2,"Copying Successfull",0);
					lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                lk_getkey();

					lk_dispclr();
                	lcd::DisplayText(2,2,"Unmounting disk ..",0);
					ret = system("umount /mnt/usb");
					lk_dispclr();
					
						if (ret == 0)
						{
						lcd::DisplayText(3,2,"Unmout Successfull",0);
						}
						else
						{
						lcd::DisplayText(3,2,"Unmounting  Failed....",0);
		                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
		                lk_getkey();
		                }
		            }
					else
					{
					lcd::DisplayText(3,2,"Copying  Failed....",0);
	                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                ret = system("umount /mnt/usb");
	                lk_getkey();
	                }

	        return ;

        }
        else if(x == klok::pc::KEYS::KEY_ENTER && flag==0)
        {
        	ret=system("mount -t vfat /dev/sda1 /mnt/usb");
			if (ret==256)
			ret=system("mount -t vfat /dev/sdb1 /mnt/usb");
			if (ret==256)
    		ret=system("mount -t vfat /dev/sdc1 /mnt/usb");
			if (ret==256)
    		ret=system("mount -t vfat /dev/sdd1 /mnt/usb");
	
			if ( ret== 0)
			{
				lk_dispclr();
				fprintf(stdout, "mass storage mounted\n");
				lcd::DisplayText(2, 2, "MOUNT SUCCESS", 1);
				lcd::DisplayText(4, 0, "Press any key to copy database to USB", 0);

				lk_getkey();
				ret=system("cp /mnt/jffs2/PayCollect.db /mnt/usb/");
				
				lk_dispclr();
				lcd::DisplayText(3,2,"Copying ....",0);
				lcd::DisplayText(5,0,"This may take 5-8 seconds",0);
				sleep(6);
				
					if (ret == 0)
					{
					lk_dispclr();
					lcd::DisplayText(3,2,"Copying Successfull",0);
					lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                lk_getkey();

					lk_dispclr();
                	lcd::DisplayText(2,2,"Unmounting disk ..",0);
					ret = system("umount /mnt/usb");
					lk_dispclr();

						if (ret == 0)
						{
						lcd::DisplayText(3,2,"Unmout Successfull",0);
						}
						else
						{
						lcd::DisplayText(3,2,"Unmounting  Failed....",0);
		                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
		                lk_getkey();
		                }
		            }
					else
					{
					lcd::DisplayText(3,2,"Copying  Failed....",0);
	                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                ret = system("umount /mnt/usb");
	                lk_getkey();
	                }
				                
				return ;
			}
			else
			{
				lk_dispclr();
				fprintf(stderr, "Mass storage mounting Failed \n");
				lcd::DisplayText(3, 2, "MOUNT FAILED", 1);
                lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
                lk_getkey();
                return ;
			}

        }
}

void Import()
{
	lk_dispclr();
	lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
    printf("Import Activity\n");

	int ret=0;
	FILE *fp;
    	
	fp=fopen("/etc/mtab", "r");
	char str[100]="", flag=0;

		if (fp==NULL)
		fprintf(stderr, "File open Error\n");

		 while((fgets(str, 80, fp))!=NULL)
		{
       		if((strstr(str, "/mnt/usb")) != NULL)
			flag=1;
		}

	fclose(fp);

        int x = lk_getkey();

        if(x == klok::pc::KEYS::KEY_ENTER && flag==1)
        {
        	lk_dispclr();
			lcd::DisplayText(2, 2, "Already Mounted", 1);
			lcd::DisplayText(4, 0, "Press any key to copy database to device", 0);

				lk_getkey();
				ret=system("cp /mnt/usb/PayCollect.db /mnt/jffs2/");
				
				lk_dispclr();
				lcd::DisplayText(3,2,"Copying ....",0);
				lcd::DisplayText(5,0,"This may take 5-8 seconds",0);
				sleep(6);
				
					if (ret == 0)
					{
					lk_dispclr();
					lcd::DisplayText(3,2,"Copying Successfull",0);
					lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                lk_getkey();

					lk_dispclr();
                	lcd::DisplayText(2,2,"Unmounting disk ..",0);
					ret = system("umount /mnt/usb");

						lk_dispclr();
						if (ret == 0)
						{
						lcd::DisplayText(3,2,"Unmout Successfull",0);
						}
						else
						{
						lcd::DisplayText(3,2,"Unmounting  Failed....",0);
		                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
		                lk_getkey();
		                }
		            }
					else
					{
					lcd::DisplayText(3,2,"Copying  Failed....",0);
	                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                ret = system("umount /mnt/usb");
	                lk_getkey();
	                }
	        return ;
        }
        else if(x == klok::pc::KEYS::KEY_ENTER && flag==0)
        {
        	ret=system("mount -t vfat /dev/sda1 /mnt/usb");
			if (ret==256)
			ret=system("mount -t vfat /dev/sdb1 /mnt/usb");
			if (ret==256)
    		ret=system("mount -t vfat /dev/sdc1 /mnt/usb");
			if (ret==256)
    		ret=system("mount -t vfat /dev/sdd1 /mnt/usb");
	
			if ( ret== 0)
			{
				lk_dispclr();
				fprintf(stdout, "mass storage mounted\n");
				lcd::DisplayText(2, 2, "MOUNT SUCCESS", 1);
				lcd::DisplayText(4, 0, "Press any key to copy database to device", 0);

				lk_getkey();
				ret=system("cp /mnt/usb/PayCollect.db /mnt/jffs2/");
				
				lk_dispclr();
				lcd::DisplayText(3,2,"Copying ....",0);
				lcd::DisplayText(5,0,"This may take 5-8 seconds",0);
				sleep(6);
				
					if (ret == 0)
					{
					lk_dispclr();
					lcd::DisplayText(3,2,"Copying Successfull",0);
					lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                lk_getkey();

					lk_dispclr();
                	lcd::DisplayText(2,2,"Unmounting disk ..",0);
					ret = system("umount /mnt/usb");
					lk_dispclr();

						if (ret == 0)
						{
						lcd::DisplayText(3,2,"Unmout Successfull",0);
						}
						else
						{
						lcd::DisplayText(3,2,"Unmounting  Failed....",0);
		                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
		                lk_getkey();
		                }
		            }
					else
					{
					lcd::DisplayText(3,2,"Copying  Failed....",0);
	                lcd::DisplayText(5,0,"Press Any Key to Exit",0);
	                ret = system("umount /mnt/usb");
	                lk_getkey();
	                }
				                
				return ;
			}
			else
			{
				lk_dispclr();
				fprintf(stderr, "Mass storage mounting Failed \n");
				lcd::DisplayText(3, 2, "MOUNT FAILED", 1);
                lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
                lk_getkey();
                return ;
			}

        }
}

// void Logout(){

// 	main();
// }

void Settings()
{
    printf("Settings\n");

    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 2;
        strcpy(menu.menu[0],"Export");
        strcpy(menu.menu[1],"Import");
        // strcpy(menu.menu[2],"Logout");

        while(1)
        {
            lk_dispclr();

            opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

            switch(opt)
            {
            case CANCEL:
                return;

            case ENTER:
                switch(selItem + 1)
                {
                case 1:
                    Export();
                    break;

                case 2:
                    Import();
                    break;

                // case 3:
                //     Logout();
                //     break;
                }
                break;
            }
        }
    }
}
void main_menu(const char* user, const char* pwd)
{
    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 3;
        strcpy(menu.menu[0],"Billing");
        strcpy(menu.menu[1],"Reports");
        strcpy(menu.menu[2],"Settings");

        while(1)
        {
            lk_dispclr();

            opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

            switch(opt)
            {
            case CANCEL:
                break;

            case ENTER:
                switch(selItem + 1)
                {
                case 1:
                    Billing();
                    break;

                case 2:
                    Reports();
                    break;

                case 3:
                    Settings();
                    break;
                }
                break;
            }
        }
    }
}

void checkDatabaseFile()
{
    //emtpy?
}

int main(int argc, const char* argv[])
{
    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    lk_open();
    mscr_open();
    lk_dispclr();
    lk_dispfont(&(X6x8_bits[0]), 6);
    lk_lcdintensity(24);

    SQLite::Database& db = getDatabase();

    lcd::DisplayText(0, 0, "Klok Innovations", 1);
    lcd::DisplayText(4, 0, "  F1   F2   F3   F4", 0);
    lk_dispbutton((unsigned char*)"Home", (unsigned char*)"Up  ", (unsigned char*)"Down", (unsigned char*)"End ");
    lk_buzzer(2);

    char autobuf[80] = {0};
    char buff[80] = {0};
    struct tm intim;
    sprintf(autobuf, "%s-%02d%02d%02d%02d%02d%04d.txt", buff, intim.tm_hour, intim.tm_min, intim.tm_sec, intim.tm_mday, intim.tm_mon + 1, intim.tm_year + 1900);
    printf("Date-Time : %s\n", getCurrentTime().c_str());

    while(1)
    {
        int key1 = lk_getkey();
        if(key1 != 0xff)
            break;
    }

    lk_dispclr();
    strcpy(menu.title, "Login");

    lk_bkl_timeout(20);

    int res = 0;

    while(1)
    {
        lcd::DisplayText(2, 0, "Enter Username", 0);

        char user[10] = {0};
        res = lk_getalpha(4, 0, (unsigned char*)user, 9, strlen(user), 0);

        if(res > 0)
        {
            user[res] = '\0';

            printf("Username is %s %d\n", user, res);

            klok::pc::User userObj;
            if(klok::pc::User::FromDatabase(getDatabase(), user, userObj) != 0)
            {
                printf("No Such User %s\n", user);
                continue;
            }

            if(userObj.id == user)
            {
                while(1)
                {
                    lk_dispclr();

                    lcd::DisplayText(2,0,"Enter Password",0);

                    char pwd[10] = {0};
                    res = lk_getpassword((unsigned char*)pwd, 4, 9);

                    if(res > 0)
                    {
                        pwd[res]='\0';
                        printf("Password is %s %d\n", pwd, res);

                        if(userObj.password == pwd)
                        {
                            gUserId = user;
                            gUserName = userObj.name;
                            gCompanyName = userObj.company_name;
                            gCompanyAddress = userObj.company_address;
                            main_menu(user, pwd);
                            printf("main_menu\n");
                        }
                        else
                        {
                            goto AGAIN_ASK_USER_DETAILS;
                        }
                    }
                }
            }
        }

        AGAIN_ASK_USER_DETAILS: while(false); //! REALLY? A GOTO? BAD! BAD! BAD!
    }
}
