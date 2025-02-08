#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "../Include/root_certificates.hpp"
#include "../Include/DB_Client.h"
#include "../Include/HTTP_HTTPs_Client.h"
#include "../Include/INI_file.h"
#include "../Include/Utils.h"
#include "../Include/Safe_Queue.h"

std::vector<std::thread> arrThread;
Safe_Queue<std::function <void()> > queue;
std::mutex mutex;

bool stop{ false };

void doWork()
{
	std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
	while (!stop)
	{
		std::function<void()> work;
		std::unique_lock<std::mutex> lock(mutex);

		if (!queue.empty())
		{
			work = queue.pop();
			std::cout << "POP\n";
			work();			
		}
		lock.unlock();
	}
}


void parseLink(const URL& link, int depth, DB_Client& db)
{
	std::cout << "parse link: " << link.host << " " << link.query << " depth = " << depth << std::endl;
	try 
	{
		std::string html = getContent(link);

		if (html.size() == 0)
		{
			std::cout << ("Failed to get HTML Content from: " + link.host + link.query) << std::endl;
			return;
		}

		// TODO: Parse HTML code here on your own
		std::map<std::string, unsigned int> word_and_count(clear_html_tag(html)); //ключ слово, значение количество его повторений
		if (word_and_count.size() > 0) 
		{
			db.addLink(word_and_count, link_to_string(link));
		}

		// TODO: Collect more links from HTML code and add them to the parser like that:

		if (depth > 0) 
		{
			std::vector<URL> links(get_link(html, link)); // получаем ссылки если будем их использовать

			size_t count = links.size();
			size_t index = 0;
			for (auto& subLink : links)
			{
				if (!db.findUrl(subLink.host + subLink.query))
				{
					queue.push([subLink, depth, &db]() { parseLink(subLink, depth - 1, db); });
					std::cout << "PUSH\n";

					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

}

int main()
{
	setlocale(LC_ALL, "ru_RU.utf-8");
	try
	{
		INI_file ini_file("config.ini");

		std::string start_page = ini_file.get_value("Client.start_page");
		int recursion_depth = std::stoi(ini_file.get_value("Client.recursion_depth"));

		std::string host = ini_file.get_value("DataBase.bd_host");
		std::string port = ini_file.get_value("DataBase.bd_port");
		std::string name = ini_file.get_value("DataBase.bd_name");
		std::string user = ini_file.get_value("DataBase.bd_user");
		std::string password = ini_file.get_value("DataBase.bd_pass");

		DB_Client db(host, port, name, user, password);

		//create
		int cntPool = std::thread::hardware_concurrency();
		for (int i = 0; i < cntPool; i++)
		{
			arrThread.push_back(std::thread(&doWork));
		}

		//work
		URL link = First_URL_to_Link(start_page);

		queue.push([link, recursion_depth, &db]() {parseLink(link, recursion_depth, db);});
		std::cout << "PUSH\n";

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		//destroy		
		std::unique_lock<std::mutex> lock(mutex);
		stop = true;
		lock.unlock();

		for (auto& t : arrThread)
		{
			t.join();
		}


	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	system("pause");
	return 0;
}