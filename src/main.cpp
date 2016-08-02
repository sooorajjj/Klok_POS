#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include "PosDataStructures.hpp"
#include "Visiontek.hpp"
#include <vector>

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
std::string gUserId = "", gUserName = "", gTransId = "", gCustomerId = "", gCustomerName = "", gCustomerBalance = "", gCustomerContact = "",
            gCompanyName = "", gCompanyAddress = "";
SQLite::Database* gDatabasePtr = NULL;
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
            gDatabasePtr = new SQLite::Database("PayCollect.db", SQLite::OPEN_READWRITE, SQLite::OPEN_CREATE);
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

    default:
        return 0;
    }

    return 0;
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

        lk_dispclr();
        lcd::DisplayText(1, 0, "Continue printing",1);
        lcd::DisplayText(4, 0, "Press Enter ", 0);

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
	float currentAmt = scanned - netAmt;

	char customerBalance[10] = {0};
	sprintf(customerBalance ,"%0.2f", currentAmt);

	toUpdate.id = gCustomerId;
	toUpdate.cur_amt = customerBalance;

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
    lk_getkey();
    lk_dispclr();

    float netAmt = principleAmt + addLess;
    char principleAmtString[30] = {0};
    char addLessString[30] = {0};
    char netAmtString[30] = {0};

    sprintf(principleAmtString, "%0.2f", principleAmt);
    sprintf(addLessString, "%0.2f", addLess);
    sprintf(netAmtString, "%0.2f", netAmt);

    lcd::DisplayText(1, 0, "Net Amount", 0);
    lcd::DisplayText(3, 0, netAmtString, 0);

    int x = lk_getkey();
    if(x == klok::pc::KEYS::KEY_ENTER)
    {
        updateCustomerBalance(netAmt, principleAmtString, addLessString, netAmtString, gCustomerBalance);
    }
}

void add_less(float principleAmt)
{
    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_ENTER)
	{

		lcd::DisplayText(2, 5, "Add/Less", 1);
	    lcd::DisplayText(4, 0, "Press F2 to Add OR F3 to Less", 0);

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
	            lcd::DisplayText(4, 0, "Press Enter once data have been confirmed", 0);
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
	            lcd::DisplayText(4, 0, "Press Enter once data have been confirmed", 0);
	        }
	        else
	        {
	            lk_dispclr();
	            lcd::DisplayText(4, 0, "Enter correct Amt", 0);
	            lk_getkey();
	        }
	    }
	}
	else
	{
    	lk_dispclr();
      	lcd::DisplayText(4, 0, "Enter correct Amt", 0);
        lk_getkey();
	}

    
}

void display_customer_details(const klok::pc::Customer& inCustomer)
{
    lk_dispclr();

    std::string Cust_Name = "Name :" + inCustomer.name;
    lcd::DisplayText(1, 0, Cust_Name.c_str(), 0);
    printf("%s\n", Cust_Name.c_str());

    std::string Cust_Bal = "Balance Amt:" + inCustomer.cur_amt;
    lcd::DisplayText(2, 0, Cust_Bal.c_str(), 0);
    printf("%s\n", Cust_Bal.c_str());

    gCustomerName = inCustomer.name;
    gCustomerBalance = inCustomer.cur_amt;
    gCustomerContact = inCustomer.contact;
    lcd::DisplayText(4, 0, "Press Enter once data have been confirmed", 0);

    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_ENTER)
    {
    	int res = 0;
        char grossAmt[10] = {0};

        lcd::DisplayText(1, 0, "Gross Amount", 0);
        res = lk_getnumeric(4, 0, (unsigned char*)grossAmt, 10, strlen(grossAmt));
        float scanned =0;

        if(sscanf(grossAmt, "%f", &scanned) == 1 && res > 0)
        {
            add_less(scanned);
            lcd::DisplayText(4, 0, "Press Enter once data have been confirmed", 0);
        }
        else
        {
            lk_dispclr();
            lcd::DisplayText(4, 0, "Enter correct Amt", 0);
            lk_getkey();
        }

    }
    else if(x == klok::pc::KEYS::KEY_CANCEL)
    {
        printf("pressed cancel while display_customer_details\n");
    }
}

void getCustomerDetails()
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

        klok::pc::display_sub_range(allCustomers, 4, res, &getPosCustomerDisplayName);

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

    lk_bkl_timeout(20);
    lk_dispclr();

    lcd::DisplayText(1, 0, "1.PayCollection Menu ", 0);
    lcd::DisplayText(4, 0, "Press any key", 0);

    lk_getkey();

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

    lk_dispclr();

    std::string display_transid = "TransNo :";
    display_transid += Trans_ID;

    lcd::DisplayText(2, 0, display_transid.c_str(), 0);
    printf("Trans_ID :%s\n", Trans_ID.c_str());

    lcd::DisplayText(4, 0, "Press Enter key to continue", 0);
    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_ENTER)
    {
        getCustomerDetails();

    }
    else if(x == klok::pc::KEYS::KEY_CANCEL)
    {
        printf("pressed cancel after TransNo screen\n");
    }
}

void POS()
{
    printf("POS Activity\n");
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

void display_transaction_details(const klok::pc::Transaction& inTransaction)
{
    lk_dispclr();

    std::string Trans_ID = "Trans No:" + inTransaction.trans_id;
    lcd::DisplayText(1, 0, Trans_ID.c_str(), 0);
    printf("%s\n", Trans_ID.c_str());

    std::string Cust_ID = "Cust ID:" + inTransaction.cust_id;
    lcd::DisplayText(2, 0, Cust_ID.c_str(), 0);
    printf("%s\n", Cust_ID.c_str());

    std::string Cur_Amt = "Amount:" + inTransaction.net_amt;
    lcd::DisplayText(3, 0, Cur_Amt.c_str(), 0);
    printf("%s\n", Cur_Amt.c_str());

    lcd::DisplayText(4, 0, "Press Enter to print, else press Cancel", 0);

    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_ENTER)
    {
    	std::string buff, buff1, buff2, buff3;

            prn_open();
            if(prn_paperstatus() != 0)
            {
                lk_dispclr();
                lcd::DisplayText(3, 5, "No Paper !", 1);
                lk_getkey();
                return;
            }

            buff.append("   Daily Report\n\n");
            buff1.append("    Bill No          ");
            buff1.append(inTransaction.trans_id);
            buff1.append("\n");
            buff1.append("    ID               ");
            buff1.append(inTransaction.cust_id);
            buff1.append("\n");
            buff1.append("    DATE AND TIME    ");
            buff1.append(inTransaction.date_time);
            buff1.append("\n");
            buff1.append("    Gross Amount     ");
            buff1.append(inTransaction.gross_amt);
            buff1.append("\n");
            buff1.append("    Add/Less         ");
            buff1.append(inTransaction.add_less);
            buff1.append("\n");
            buff1.append("     -------------------------------\n");
            buff2.append("  CASH       ");
            buff2.append(inTransaction.net_amt);
            buff2.append("\n");
            buff3.append("    Billing User ID     ");
            buff3.append(inTransaction.user_id);
            buff3.append("\n");
            buff3.append("     -------------------------------\n");
            buff3.append("          THANK YOU VISIT AGAIN\n");

            lk_dispclr();
            lcd::DisplayText(3, 5, "PRINTING Report", 1);

            int ret;

            ret = printer::WriteText(buff.c_str(), buff.size(), 2);
            returncheck(ret);

            ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
            returncheck(ret);

            ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
            returncheck(ret);

            ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
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
        printf("pressed cancel while display_transaction_details\n");
        return;
    }
}

void getDateWiseDetails(std::string date)
{
    std::vector<klok::pc::Transaction> allTransactions;
    if(klok::pc::Transaction::GetTransactionsForDate(getDatabase(), allTransactions, date.c_str(), 20) == 0)
    {
    	std::string transDate = "on " + date;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range_with_title(allTransactions, transDate.c_str(), 5, res, &getPosTransactionDisplayName);

        if(!res.wasCancelled)
        {
            display_transaction_details(allTransactions[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to GetTransactionsForDate -> getDateWiseDetails \n");
    }
}

void EnteringDate(){

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

            printf("Transaction No :%s\n", datesUnique[i].c_str());
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
    	std::string transmonth = "on " + month;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range_with_title(allTransactions, transmonth.c_str(), 5, res, &getPosTransactionDisplayName);

        if(!res.wasCancelled)
        {
            display_transaction_details(allTransactions[res.selectedIndex]);
        }
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
    	std::string transyear = "on " + year;

        for(int i = 0; i != allTransactions.size(); i++)
        {
            printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
            printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());
        }

        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        klok::pc::display_sub_range_with_title(allTransactions, transyear.c_str(), 5, res, &getPosTransactionDisplayName);

        if(!res.wasCancelled)
        {
            display_transaction_details(allTransactions[res.selectedIndex]);
        }
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
void display_customer_report(const klok::pc::Customer& inCustomer)
{
    lk_dispclr();

    std::string Cust_Name = "Name :" + inCustomer.name;
    lcd::DisplayText(1, 0, Cust_Name.c_str(), 0);
    printf("%s\n", Cust_Name.c_str());

    std::string Cust_Bal = "Balance Amt:" + inCustomer.cur_amt;
    lcd::DisplayText(2, 0, Cust_Bal.c_str(), 0);
    printf("%s\n", Cust_Bal.c_str());

    gCustomerName = inCustomer.name;
    gCustomerBalance = inCustomer.cur_amt;
    gCustomerContact = inCustomer.contact;
    lcd::DisplayText(4, 0, "Press Enter once data have been confirmed", 0);

    int x = lk_getkey();
    lk_dispclr();

    if(x == klok::pc::KEYS::KEY_ENTER)
    {
        printf("pressed enter while display_customer_details\n");

    }
    else if(x == klok::pc::KEYS::KEY_CANCEL)
    {
        printf("pressed cancel while display_customer_details\n");
    }
}


void CustomerWiseReport()
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

        klok::pc::display_sub_range(allCustomers, 4, res, &getPosCustomerDisplayName);

        if(!res.wasCancelled)
        {
            gCustomerId = allCustomers[res.selectedIndex].id;
            display_customer_report(allCustomers[res.selectedIndex]);
        }
    }
    else
    {
        printf("failed to GetAllFromDatabase -> getCustomerDetails \n");
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
        menu.maxEntries = 3;
        strcpy(menu.menu[0],"Daily Collection Report");
        strcpy(menu.menu[1],"Consolidated Report");
        strcpy(menu.menu[2],"Customer Wise Report");

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
                }
                break;
            }
        }
    }
}

void Customer()
{
    printf("Customer Activity\n");
}

void Item()
{
    printf("Item Activity\n");
}

void Master()
{
    printf("Master\n");

    MENU_T menu;
    int opt = 0;
    int selItem = 0;
    int acceptKbdEvents = 0;

    while(1)
    {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 2;
        strcpy(menu.menu[0],"Customer");
        strcpy(menu.menu[1],"Item");

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
                    Customer();
                    break;

                case 2:
                    Item();
                    break;
                }
                break;
            }
        }
    }
}

void UserRights()
{
    printf("UserRights Activity\n");
}

void Download()
{
	lk_dispclr();
	lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
	printf("Download Activity\n");

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
				sleep(6);
				
					if (ret == 0)
					{
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
				sleep(6);
				
					if (ret == 0)
					{
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

void Upload()
{
	lk_dispclr();
	lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
    printf("Upload Activity\n");

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
				sleep(6);
				
					if (ret == 0)
					{
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
				sleep(6);
				
					if (ret == 0)
					{
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
        menu.maxEntries = 3;
        strcpy(menu.menu[0],"User Rights" );
        strcpy(menu.menu[1],"Download");
        strcpy(menu.menu[2],"Upload");

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
                    UserRights();
                    break;

                case 2:
                    Download();
                    break;

                case 3:
                    Upload();
                    break;
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
        menu.maxEntries = 4;
        strcpy(menu.menu[0],"Billing");
        strcpy(menu.menu[1],"Reports");
        strcpy(menu.menu[2],"Master");
        strcpy(menu.menu[3],"Settings");

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
                    Master();
                    break;

                case 4:
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
