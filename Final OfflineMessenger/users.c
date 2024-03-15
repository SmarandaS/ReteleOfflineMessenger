#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
int callback(void*, int, char**, char**);

int main(void) {
    
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("conturi.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }
    
    char *sql = "DROP TABLE IF EXISTS Users;" 
                "CREATE TABLE Users(Id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT NOT NULL, Online INT);" 
                "INSERT INTO Users(Name, Online) VALUES('Ana', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Alexandra', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Andreea', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Andrei', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Emil', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Madalina', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Smarandaa', 0);" 
                "INSERT INTO Users(Name, Online) VALUES('Veronica', 0);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 

    char *sql2 = "DROP TABLE IF EXISTS Mesaje;" 
                "CREATE TABLE Mesaje(Sursa INTEGER, Destinatie INTEGER, Mesaj TEXT, Trimis INTEGER);" ;

    rc = sqlite3_exec(db, sql2, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 

   
    sqlite3_close(db);
    
    return 0;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    NotUsed=0;

    for( int i=0 ; i< argc ; i++)
    {
        printf("%s\n", argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}



