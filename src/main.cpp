#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <SQLiteCpp/SQLiteCpp.h> 

int main(int argc, const char* argv[])
{
   try
{
    // Open a database file
    SQLite::Database    db("PayCollect.db");

    // Compile a SQL query, containing one parameter (index 1)
    SQLite::Statement   query(db, "SELECT * FROM pay_coll_user ");

    // Bind the integer value 6 to the first parameter of the SQL query
    // Loop to execute the query step by step, to get rows of result
    while (query.executeStep())
    {
        // Demonstrate how to get some typed column value
        int         id      = query.getColumn(0);
        const char* value   = query.getColumn(1);

        std::cout << "row: " << id << ", " << value << ", "<< std::endl;
    }
}
catch (std::exception& e)
{
    std::cout << "exception: " << e.what() << std::endl;
}
}


