
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#ifdef WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
//#include <Windows.h>
#include <map>

#include "../Include/INI_File.h"
#include "../Include/DB_Client.h"
#include "../Include/HTTP_Server.h"
#include "../Include/Utils.h"


// "Loop" forever accepting new connections.
void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, DB_Client& db)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HTTP_Server>(std::move(socket), &db)->start();
			httpServer(acceptor, socket, db);
		});
}


int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try
	{
		INI_file ini_file("config.ini");

		DB_Client db(ini_file.get_value("DataBase.bd_name"), ini_file.get_value("DataBase.bd_user"), ini_file.get_value("DataBase.bd_pass"));

		unsigned short port = static_cast<unsigned short>(std::stoi(ini_file.get_value("Server.server_port")));
		auto const address = net::ip::make_address("0.0.0.0");

		net::io_context ioc{1};

		tcp::acceptor acceptor{ioc, { address, port }};
		tcp::socket socket{ioc};
		httpServer(acceptor, socket, db);

		std::cout << "Open browser and connect to http://localhost:" <<  port << " to see the web server operating" << std::endl;

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}