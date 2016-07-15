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
	                                                                              
	strcpy(menu.title,"MAIN MENU");       
	    strcpy(menu.menu[0],"LCD");
	    strcpy(menu.menu[1],"Keypad");
	    strcpy(menu.menu[2],"Printer");
	    strcpy(menu.menu[3],"Date & Time");
	    strcpy(menu.menu[4],"Battery");
	    strcpy(menu.menu[5],"USB Host");
	    strcpy(menu.menu[6],"Machine Id");
	    strcpy(menu.menu[7],"JFFS2");
	    strcpy(menu.menu[8],"Software Details");
	    strcpy(menu.menu[9],"RFID");
	strcpy(menu.menu[10],"IFD");
        strcpy(menu.menu[11],"SAM");
        strcpy(menu.menu[12],"Magnetic Head" );
	strcpy(menu.menu[13],"Audio");
	strcpy(menu.menu[14],"Communication");
	strcpy(menu.menu[15],"MicroSD Card");
	strcpy(menu.menu[16],"Download");
	strcpy(menu.menu[17],"console");
	strcpy(menu.menu[18],"Shutdown");
	strcpy(menu.menu[19],"Bash");
	while(1){
          
        	lk_dispclr();
               	opt = scroll_menu(&menu,&selItem,acceptKbdEvents);
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
}


