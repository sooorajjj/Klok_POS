#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <SQLiteCpp/SQLiteCpp.h> 

#include "Visiontek.hpp"

extern "C"{
	#include<X6x8.h>
	#include<0202lcd.h>
	#include<V91magswipe.h>
	#include <header.h>
	#include<printer.h>
}
void PayCollection(){
	printf("PayCollection Activity\n");
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
	
	lcd::DisplayText(0,0,"Klok Innovations",1);
	lcd::DisplayText(4,0,"  F1   F2   F3   F4",0);   
	lk_dispbutton((unsigned char*)"Home",(unsigned char*)"Up  ",(unsigned char*)"Down",(unsigned char*)"End ");
	lk_buzzer(2);

	sprintf(autobuf,"%s-%02d%02d%02d%02d%02d%04d.txt",buff,intim.tm_hour,intim.tm_min,intim.tm_sec,intim.tm_mday,intim.tm_mon+1,intim.tm_year+1900);

	while(1)
	{	
	int key1=lk_getkey();
		if(key1!=0xff)
			break;
	}

		
	lk_dispclr();

        menu.start                      = 0;
        menu.maxEntries                 = 20;
	                                                                              
	strcpy(menu.title,"Login");       
	    
	int res=0;
	prn_open();

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
			if(strcmp(user,"0123")==0)
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
						if(strcmp(pwd,"0123")==0)
						{
							main_menu(user,pwd);
							printf("main_menu\n");
						}
					}
				}
			}
		}
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

// 			lk_dispclr();
//     	strcpy(menu.menu[1],"Billing");
//     		strcpy(menu.menu[2],"Pay Collection");
//     		strcpy(menu.menu[3],"POS");
//     	strcpy(menu.menu[4],"Reports");
//     		strcpy(menu.menu[5],"Daily Collection Report");
//     		strcpy(menu.menu[6],"Consolidated Report");
//     		strcpy(menu.menu[7],"Customer Wise Report");
// 		strcpy(menu.menu[8],"Master");
//     		strcpy(menu.menu[9],"Customer");
// 			strcpy(menu.menu[10],"Item");
//     	strcpy(menu.menu[11],"Settings");
//     		strcpy(menu.menu[12],"User Rights" );
// 			strcpy(menu.menu[13],"Download");
// 			strcpy(menu.menu[14],"Upload");
// }


