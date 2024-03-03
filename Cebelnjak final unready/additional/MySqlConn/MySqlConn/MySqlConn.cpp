#include <iostream>

#include "mysql.h"
#include "mysqld_error.h"

#define log(x) std::cout << x << std::endl;
#define logn(x) std::cout << x;
#define debugerr(x) std::cerr << x << std::endl;

int main()
{
    HANDLE hPipe;
    char buffer[4097];
    DWORD dwRead;

    // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\mynamedpipe"), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 1024 * 16, 1024 * 16, NMPWAIT_USE_DEFAULT_WAIT, NULL);

    MYSQL* mysql;

    if (!(mysql = mysql_init(0)))
    {
        debugerr("mysql init failed");
        return -1;
    }

    if (!mysql_real_connect(mysql, "localhost", "root", "!Aoppq4913", "cpp_test", 3306, NULL, NULL))
    {
        debugerr("mysql connection failed");
        debugerr(mysql_error(mysql));
        return -1;
    }

    while (hPipe != INVALID_HANDLE_VALUE)
    {
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                /* add terminating zero */
                buffer[dwRead] = '\0';

                /* do something with data in buffer */
                std::cout << buffer << std::endl;

                if (mysql_query(mysql, buffer))//"INSERT INTO Test(polje) VALUES('text1');"
                {
                    debugerr("mysql query failed");
                    debugerr(mysql_error(mysql));
                    return -1;
                }

                mysql_store_result(mysql);
            }
        }

        DisconnectNamedPipe(hPipe);
    }

    //MYSQL_RES* res;
    //res = mysql_store_result(mysql);
    //https://cplusplus.com/forum/general/118160/
    //mysql_free_result(res);

    system("pause");

    mysql_close(mysql);
}
