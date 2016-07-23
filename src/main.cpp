#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <SQLiteCpp/SQLiteCpp.h> 

#include "PosDataStructures.hpp"
#include "Visiontek.hpp"
#include <vector>

extern "C"{
	#include<X6x8.h>
	#include<0202lcd.h>
	#include<V91magswipe.h>
	#include <header.h>
	#include<printer.h>
}

namespace {
	std::string gUserId = "",gTransId="", gCustomerId="";
	SQLite::Database * gDatabasePtr = NULL;
}

void prompt_shutdown(){
	lcd::DisplayText(3,0,"press power to shutdown",1);
	while(true);
}


void display_fatal_error(const char * userError,std::exception & exc){
	// call DisplayText 
	lk_dispclr();
	lcd::DisplayText(1,0,userError,1);

	// Display exc.what()
	std::string err = exc.what();
	lcd::DisplayText(2,0,err.c_str(),1);
}



static void closeDatabase(){

	if(gDatabasePtr != NULL)
	{
		delete gDatabasePtr;
		gDatabasePtr = NULL;
	}

}


static SQLite::Database & getDatabase(){
	if(gDatabasePtr == NULL){
		try
		{
			gDatabasePtr = new SQLite::Database("PayCollect.db", SQLite::OPEN_READWRITE, SQLite::OPEN_CREATE);
		}
		catch(std::exception & e)
		{
	
			display_fatal_error("Database Open Failed",e);
			prompt_shutdown();
		}
	}
	else
	{
		return *gDatabasePtr;
	}
}

const char * getPosCustomerDisplayName(const klok::pc::Customer & inCustomer)
{
	return inCustomer.id.c_str();
}

void insertAndPrint(float principleAmt, float addLess, float netAmt, std::string principleAmtString, std::string addLessString, std::string netAmtString, std::string customerId, std::string userId, std::string transId){

	klok::pc::Transaction toInsert;
	toInsert.cust_id = customerId;
	toInsert.user_id = userId;
	toInsert.gross_amt = principleAmtString;
	toInsert.add_less = addLessString;
	toInsert.net_amt = netAmtString;
	// toInsert.date_time = 
	if(klok::pc::Transaction::InsertIntoTable(getDatabase(),toInsert)==0)
	{
		int status = 0;
		lk_dispclr();
		lcd::DisplayText(1,0,"Continue printing",0);
		int x = lk_getkey();
		if(x == klok::pc::KEYS::KEY_ENTER)
		{
			if(prn_open()!=-1)
			{
				if(prn_paperstatus()!=0)
	        	{
	                lk_dispclr();
	                lcd::DisplayText(3,5,"No Paper !",1);
	                lk_getkey();

	                return;
	        	}
	    		status=printer::WriteText("\n",1,1);
	    		if(status != 0)
	    		{
	    			lk_dispclr();
	    			lcd::DisplayText(1,0,"Printing Error",0);
	    			lk_getkey();
	    			return;
	    		}
	    		status=printer::WriteText("Klok Innovations",16,1);
	    		if(status != 0)
	    		{
	    			lk_dispclr();
	    			lcd::DisplayText(1,0,"Printing Error",0);
	    			lk_getkey();
	    			return;
	    		}
	    		status=printer::WriteText("1234567890",1,1);
	    		if(status != 0)
	    		{
	    			lk_dispclr();
	    			lcd::DisplayText(1,0,"Printing Error",0);
	    			lk_getkey();
	    			return;
	    		}
	    		status=printer::WriteText(customerId.c_str(),1,1);
	    		if(status != 0)
	    		{
	    			lk_dispclr();
	    			lcd::DisplayText(1,0,"Printing Error",0);
	    			lk_getkey();
	    			return;
	    		}
	    		status=printer::WriteText(userId.c_str(),1,1);
	    		if(status != 0)
	    		{
	    			lk_dispclr();
	    			lcd::DisplayText(1,0,"Printing Error",0);
	    			lk_getkey();
	    			return;
	    		}
			}
		}else
			{

			}
	}else{
		printf("failed to InsertIntoTable\n");
	}

}
void net_amt(float principleAmt,float addLess)
{
	lk_getkey();
	lk_dispclr();
	float netAmt = principleAmt + addLess;	
	char principleAmtString [30]={0};
	char addLessString [30]={0};
	char netAmtString [30]={0};
	sprintf(principleAmtString,"%f",principleAmt);
	sprintf(addLessString,"%f",addLess);
	sprintf(netAmtString,"%f",netAmt);
	lcd::DisplayText(1,0,"Net Amount",0);
	lcd::DisplayText(3,0,netAmtString,0);


	
	int x = lk_getkey();
		if(x == klok::pc::KEYS::KEY_ENTER){
			insertAndPrint(principleAmt,addLess,netAmt,principleAmtString,addLessString,netAmtString,gCustomerId,gUserId,gTransId);
		}
}

void add_less(float principleAmt)
{
	int x = lk_getkey();
	lk_dispclr();
	if(x == klok::pc::KEYS::KEY_ENTER){
		int res = 0;
		char addLess[10]={0};
		lcd::DisplayText(1,0,"Add/Less",0);
		res=lk_getnumeric(4,0,(unsigned char *)addLess,10,strlen(addLess));
		float scanned =0;
		if (sscanf(addLess,"%f",&scanned)==1&&res>0)
		{
			net_amt(principleAmt,scanned);
			lcd::DisplayText(4,0,"Press Enter once data have been confirmed",0);
		}else{
			lk_dispclr();
			lcd::DisplayText(4,0,"Enter correct Amt",0);
			lk_getkey();
		}
	}
}

void display_customer_details(const klok::pc::Customer & inCustomer){
	lk_dispclr();
	std::string Cust_Name = "Name :" + inCustomer.name;
	lcd::DisplayText(1,0,Cust_Name.c_str(),0);
	printf("%s\n",Cust_Name.c_str());
	std::string Cust_Bal = "Balance Amt:" + inCustomer.cur_amt;
	lcd::DisplayText(2,0,Cust_Bal.c_str(),0);
	printf("%s\n",Cust_Bal.c_str());
	lcd::DisplayText(4,0,"Press Enter once data have been confirmed",0);

	int x = lk_getkey();
	lk_dispclr();
	if(x == klok::pc::KEYS::KEY_ENTER){
		int res = 0;
		char grossAmt[10]={0};
		lcd::DisplayText(1,0,"Gross Amount",0);
		res=lk_getnumeric(4,0,(unsigned char *)grossAmt,10,strlen(grossAmt));
		float scanned =0;
		if (sscanf(grossAmt,"%f",&scanned)==1&&res>0)
		{
			add_less(scanned);
			lcd::DisplayText(4,0,"Press Enter once data have been confirmed",0);
		}else{
			lk_dispclr();
			lcd::DisplayText(4,0,"Enter correct Amt",0);
			lk_getkey();
		}
	}else if(x == klok::pc::KEYS::KEY_CANCEL){
		printf("pressed cancel while display_customer_details\n");
		
	}
}

void getCustomerDetails(){

	std::vector<klok::pc::Customer> allCustomers;
	if(klok::pc::Customer::GetAllFromDatabase(getDatabase(),allCustomers,10)== 0)
		{
		//got all users


			for(int i = 0; i != allCustomers.size(); i++)
			{
				printf("Userid :%s\n",allCustomers[i].id.c_str() );
				printf("Username :%s\n",allCustomers[i].name.c_str() );
			}	

			klok::pc::MenuResult res;
			res.wasCancelled = false;
			res.selectedIndex = -1;
			klok::pc::display_sub_range(allCustomers,4,res,&getPosCustomerDisplayName);
			if(!res.wasCancelled)
			{
				gCustomerId = allCustomers[res.selectedIndex].id;
				display_customer_details(allCustomers[res.selectedIndex]);
			}
		}else{
		printf("failed to GetAllFromDatabase \n");
		// failed
		}
}

void PayCollection()
{
	printf("PayCollection Activity\n");

	lk_bkl_timeout(20);
	lk_dispclr();
	lcd::DisplayText(1,0,"1.PayCollection Menu ",0);
	lcd::DisplayText(4,0,"Press any key",0);
	lk_getkey();

	std::string Trans_ID = "";
	if(klok::pc::User::GetNextTransactionIDForUser(getDatabase(),gUserId.c_str(),Trans_ID) !=0 )
	{
		printf("Getting Trans_ID for user %s failed \n",gUserId.c_str());
		return;
	}
	else if (Trans_ID == "")
	{
		printf("User name  %s is not available\n",gUserId.c_str());
		return;
	}
	gTransId = Trans_ID;

		
	lk_dispclr();
	std::string display_transid = "TransNo :";
	display_transid+=Trans_ID;
	lcd::DisplayText(2,0,display_transid.c_str(),0);
	printf("Trans_ID :%s\n",Trans_ID.c_str());

	lcd::DisplayText(4,0,"Press Enter key to continue",0);
	int x = lk_getkey();
	lk_dispclr();
	if(x == klok::pc::KEYS::KEY_ENTER)
	{
	getCustomerDetails();

	}else if(x == klok::pc::KEYS::KEY_CANCEL){
		printf("pressed cancel after TransNo screen\n");
	}
}

void POS(){
	printf("POS Activity\n");
}

void Billing(){
	printf("Billing\n");

	MENU_T menu;
    int opt=0;
    int selItem  = 0;
    int acceptKbdEvents=0;

	while(1)
	{
		lk_dispclr();
        menu.start                      = 0;
        menu.maxEntries                 = 3;
		strcpy(menu.menu[0],"Pay Collection");
		strcpy(menu.menu[1],"POS");

		while(1)
		{
		    lk_dispclr();
		    opt = scroll_menu(&menu,&selItem,acceptKbdEvents);
		    	switch(opt)
		    	{
					case CANCEL:
					return;
		          
		           	case ENTER:

		        		switch (selItem+1)
		        		{
		           		        case 1: PayCollection();
		           		        break;
		                		case 2: POS();
		                		break;
		        		}
		    			break;
		    	}
        }
	}


}

void DailyCollectionReport(){
	printf("DailyCollectionReport Activity\n");
}

void ConsolidatedReport(){
	printf("ConsolidatedReport Activity\n");
}

void CustomerWiseReport(){
	printf("CustomerWiseReport Activity\n");
}

void Reports(){
	printf("Reports\n");

	MENU_T menu;
    int opt=0;
    int selItem  = 0;
    int acceptKbdEvents=0;

	while(1)
	{
		lk_dispclr();
        menu.start                      = 0;
        menu.maxEntries                 = 3;
		strcpy(menu.menu[0],"Daily Collection Report");
 		strcpy(menu.menu[1],"Consolidated Report");
		strcpy(menu.menu[2],"Customer Wise Report");

		while(1)
		{
		    lk_dispclr();
		    opt = scroll_menu(&menu,&selItem,acceptKbdEvents);
		    	switch(opt)
		    	{
					case CANCEL:
					return;
		          
		           	case ENTER:

		        		switch (selItem+1)
		        		{
		           		        case 1: DailyCollectionReport();
		           		        break;
		                		case 2: ConsolidatedReport();
		                		break;
		                		case 3: CustomerWiseReport();
		                		break;
		        		}
		    			break;
		    	}
        }
	}
}
void Customer(){
	printf("Customer Activity\n");
}
void Item(){
	printf("Item Activity\n");
}
void Master(){
	printf("Master\n");

	MENU_T menu;
    int opt=0;
    int selItem  = 0;
    int acceptKbdEvents=0;

	while(1)
	{
		lk_dispclr();
        menu.start                      = 0;
        menu.maxEntries                 = 2;
		strcpy(menu.menu[0],"Customer");
		strcpy(menu.menu[1],"Item");

		while(1)
		{
		    lk_dispclr();
		    opt = scroll_menu(&menu,&selItem,acceptKbdEvents);
		    	switch(opt)
		    	{
					case CANCEL:
					return;
		          
		           	case ENTER:

		        		switch (selItem+1)
		        		{
		           		        case 1: Customer();
		           		        break;
		                		case 2: Item();
		                		break;
		        		}
		    			break;
		    	}
        }
	}
}

void UserRights(){
	printf("UserRights Activity\n");
}

void Download(){
	printf("Download Activity\n");
}

void Upload(){
	printf("Upload Activity\n");
}

void Settings(){
	printf("Settings\n");

	MENU_T menu;
    int opt=0;
    int selItem  = 0;
    int acceptKbdEvents=0;

	while(1)
	{
		lk_dispclr();
        menu.start                      = 0;
        menu.maxEntries                 = 2;
		strcpy(menu.menu[0],"User Rights" );
		strcpy(menu.menu[1],"Download");
		strcpy(menu.menu[2],"Upload");

		while(1)
		{
		    lk_dispclr();
		    opt = scroll_menu(&menu,&selItem,acceptKbdEvents);
		    	switch(opt)
		    	{
					case CANCEL:
					return;
		          
		           	case ENTER:

		        		switch (selItem+1)
		        		{
		           		        case 1: UserRights();
		           		        break;
		                		case 2: Download();
		                		break;
		                		case 3: Upload();
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
    int opt=0;
    int selItem  = 0;
    int acceptKbdEvents=0;

	while(1)
	{
		lk_dispclr();
        menu.start                      = 0;
        menu.maxEntries                 = 4;
    	strcpy(menu.menu[0],"Billing");
    	strcpy(menu.menu[1],"Reports");
		strcpy(menu.menu[2],"Master");
    	strcpy(menu.menu[3],"Settings");
			while(1)
			{
	        	lk_dispclr();
	            opt = scroll_menu(&menu,&selItem,acceptKbdEvents);
	            	switch(opt)
	            	{
						case CANCEL:
						break;
	                  
	                   	case ENTER:

	                		switch (selItem+1)
	                		{
	                   		        case 1: Billing();
	                   		        break;
	                        		case 2: Reports();
	                        		break;
	                        		case 3: Master();
	                        		break;
	                        		case 4:	Settings();
	                        		break;
	                		}
	            			break;
            		}
        	}
	}
}




void checkDatabaseFile(){

}

int main(int argc, const char* argv[])
{


	char autobuf[80]={0};

 	struct tm intim;

	MENU_T menu;
    int opt=0;
    int selItem  = 0;
    int acceptKbdEvents=0;
	char buff[80]={0};

	lk_open();
	mscr_open();
	lk_dispclr();
	lk_dispfont(&(X6x8_bits[0]),6);
	lk_lcdintensity(24);


	SQLite::Database & db = getDatabase();
	
	lcd::DisplayText(0,0,"Klok Innovations",1);
	lcd::DisplayText(4,0,"  F1   F2   F3   F4",0);   
	lk_dispbutton((unsigned char*)"Home",(unsigned char*)"Up  ",(unsigned char*)"Down",(unsigned char*)"End ");
	lk_buzzer(2);

	sprintf(autobuf,"%s-%02d%02d%02d%02d%02d%04d.txt",buff,intim.tm_hour,intim.tm_min,intim.tm_sec,intim.tm_mday,intim.tm_mon+1,intim.tm_year+1900);
	printf("%s\n",autobuf );
	while(1)
	{	
	int key1=lk_getkey();
		if(key1!=0xff)
			break;
	}

		
	lk_dispclr();                                 
	strcpy(menu.title,"Login");

	int res=0;

	lk_bkl_timeout(20);
	lk_dispclr();
	lcd::DisplayText(1,0,"1.User Menu ",0);
	lcd::DisplayText(4,0,"Press any key",0);
	lk_getkey();

	while(1)
	{
		char user[10]={0};
		lk_dispclr();
		lcd::DisplayText(2,0,"Enter Username",0);
		res=lk_getalpha(4,0,(unsigned char *)user,9,strlen(user),0);
		if(res>0)
		{
			user[res]='\0';

			printf("Username is %s %d\n",user,res);
			klok::pc::User userObj;
			if(klok::pc::User::FromDatabase(getDatabase(),user,userObj) != 0){
				printf("No Such User %s\n", user);
				continue;
			}
			if(userObj.id == user)
			{
				while(1)
				{
					char pwd[10]={0};
					lk_dispclr();
					lcd::DisplayText(2,0,"Enter Password",0);
					res=lk_getpassword((unsigned char *)pwd,4,9);
					if (res>0)
					{
						pwd[res]='\0';
						printf("Password is %s %d\n",pwd,res);
						if(userObj.password == pwd )
						{
							gUserId = user;
							main_menu(user,pwd);
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
AGAIN_ASK_USER_DETAILS:
		while(false);

	}

LOGIN_SUCCESS:
	lk_dispclr();
	lcd::DisplayText(2,4,"Login Success",1);
	sleep(2);

	return 0;
}
	// try
	// {
 //    // Open a database file
 //    SQLite::Database    db("PayCollect.db");

 //    // Compile a SQL query, containing one parameter (index 1)
 //    SQLite::Statement   query(db, "SELECT * FROM pay_coll_user ");

 //    // Bind the integer value 6 to the first parameter of the SQL query
 //    // Loop to execute the query step by step, to get rows of result
	//     while (query.executeStep())
	//     {
	//         // Demonstrate how to get some typed column value
	//         int         id      = query.getColumn(0);
	//         const char* value   = query.getColumn(1);

	//         std::cout << "row: " << id << ", " << value << ", "<< std::endl;
	//     }
	// }
	// catch (std::exception& e)
	// {
 //    std::cout << "exception: " << e.what() << std::endl;
	// }

