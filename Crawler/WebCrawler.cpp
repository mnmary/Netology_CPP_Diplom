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
//очередь состоит из ссылок на скачивание
Safe_Queue <TqueueItem> queue;
std::mutex mutex;

bool stop{ false };


//пул потоков выполняет скачивание страницы по ссылке
void doWork(DB_Client& db)
{
	std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
	while (!stop)
	{
		TqueueItem work;
		std::unique_lock<std::mutex> lock(mutex);

		if (!queue.empty())
		{
			work = queue.pop();
			std::cout << std::this_thread::get_id() << " POP " << work.link.host << work.link.query << " depth = " << work.dept << " " << "\n";
			std::vector<URL> links;
			std::string html = getContent(work.link);

			if (html.size() > 0)
			{
				std::map<std::string, unsigned int> word_and_count(clear_html_tag(html)); //ключ слово, значение количество его повторений
				if (word_and_count.size() > 0)
				{
					db.addLink(word_and_count, link_to_string(work.link));
				}

				links = get_link(html, work.link);
				std::cout << "link = " << work.link.host << work.link.query << " links count " << links.size() << std::endl;
			}

			if (work.dept > 0)
			{
				std::cout << "depth > 0 links quan = " << links.size() << std::endl;
				for (auto& subLink : links)
				{
					if (!db.findUrl(subLink.host + subLink.query))
					{
						TqueueItem qSubLink;
						qSubLink.link = subLink;
						qSubLink.dept = work.dept - 1;
						queue.push(qSubLink);
						std::cout << std::this_thread::get_id() << " PUSH " << qSubLink.link.host << qSubLink.link.query << " depth = " << qSubLink.dept << "\n";
					}
				}
			}
			
		}
		lock.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
			arrThread.push_back(std::thread(&doWork, db));
		}

		//work
		URL link = First_URL_to_Link(start_page);

		TqueueItem qFirstLink;
		qFirstLink.link = link;
		qFirstLink.dept = recursion_depth;
		
		queue.push(qFirstLink);
		std::cout << std::this_thread::get_id() << " PUSH " << qFirstLink.link.host << qFirstLink.link.query << " depth = " << qFirstLink.dept << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));
		
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