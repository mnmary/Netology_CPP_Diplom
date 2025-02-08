#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <regex>
#include <map>

#include <./boost/locale.hpp>


enum class TProtocol
{
	HTTP = 0,
	HTTPS = 1
};

struct URL
{
	TProtocol protocol;
	std::string host;
	std::string query;

	bool operator==(const URL& l) const
	{
		return protocol == l.protocol
			&& host == l.host
			&& query == l.query;
	}
};

//trim
void trim(std::string& s)
{
	//ltrim
	s.erase(0, s.find_first_not_of(" \t"));
	//rtrim
	s.erase(s.find_last_not_of(" \t") + 1);
}


std::map<std::string, unsigned int> clear_html_tag(const std::string& html)
// отчистка строки от тэгов. строка анализируется посимвольно все что заключено между <> пропускается. буквы заключенные между >< остаются и разделяются пробелами.
//одержимое тэгов style script пропускается.
{
	// Create system default locale
	boost::locale::generator gen;
	std::locale loc = gen("");
	std::locale::global(loc);
	std::cout.imbue(loc);

	const std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	//const std::string no_letters = " .,:;{}[]()!?@#$%^&*\n\t'\"-+=_<>~\\|/";
	const int  size_wold = 3;//размер пропускаумых слов
	std::string word;
	std::map<std::string, unsigned int> result;

	for (int i = 0; i < html.length(); ++i) {
		if (html.at(i) == '<')
		{ // часть удаления тэга
			if (word.length() > size_wold) {//если было слово то добавим его (случай когда слово в плотную к ><)
				word = boost::locale::to_lower(word);
				result[word]++;
			};
			word.erase();

			std::string tag_name = "";
			while (html.at(i + 1) != ' ' && html.at(i + 1) != '>') {
				tag_name += html.at(i + 1);
				++i;
			}
			while (html.at(i) != '>') {
				++i;
			}
			//пропускаем содежимое в заданных тэгах
			if (tag_name == "script" || tag_name == "style") {
				while (html.at(i + 1) != '<') {
					++i;
				}
			}
		}
		else
		{//часть формирования слова
			//if (no_letters.find_first_of(html.at(i)) == std::string::npos)//оставляем все кроме no_letters (вариант для разных языков)
			if (letters.find_first_of(html.at(i)) != std::string::npos)//оставляем только символы из letters
			{
				word += html.at(i);
			}
			else {
				if (word.length() > size_wold) {
					word = boost::locale::to_lower(word);
					result[word]++;
				};
				word.erase();
			}
		}
	}
	return result;
}

URL First_URL_to_Link(const std::string& url)
{
	std::regex ur("(https?)?(:?\/\/)?([[:alnum:]-_]+\..*?)?(\/.*)");
	// \1 = протокол (http, https или пусто) \2 =не нужно \3 = адрес \4 = страница
	std::smatch sm;
	std::regex_search(url, sm, ur);

	URL tmp_link;

	//заполняем протокол
	if (sm[1].length() != 0) //протокол найден
	{
		if (sm[1].str() == "http") {
			tmp_link.protocol = TProtocol::HTTP;
		}
		else if (sm[1].str() == "https") {
			tmp_link.protocol = TProtocol::HTTPS;
		}
	}
	else //протокол не найден
	{
		throw "Bad first URL: not find protocol.";
	};
	//заполняем хост
	if (sm[3].length() != 0)
	{
		tmp_link.host = sm[3].str();
	}
	else
	{
		throw "Bad first URL: not find host.";
	};
	//заполняем страницу
	if (sm[4].length() != 0) {
		tmp_link.query = sm[4].str();
	}
	else {
		tmp_link.query = '/';
	};
	return tmp_link;
}

URL URL_to_Link(const std::string& url, const URL& current_link)
{
	std::regex ur("(https?)?(:?\/\/)?([[:alnum:]_-]+\.[^\/]+)?(\/.*(#[^\/]+$)?)");
	// \1 = протокол (http, https или пусто) \2 =не нужно \3 = адрес \4 = страница \5 = ссылка на странице
	std::smatch sm;
	std::regex_search(url, sm, ur);

	URL tmp_link;

	//заполняем паротокол
	if (sm[1].length() != 0) //протокол найден
	{
		if (sm[1].str() == "http") {
			tmp_link.protocol = TProtocol::HTTP;
		}
		else if (sm[1].str() == "https") {
			tmp_link.protocol = TProtocol::HTTPS;
		}
	}
	else //протокол не найден
	{
		tmp_link.protocol = current_link.protocol;
	};
	//заполняем хост
	if (sm[3].length() != 0)
	{
		tmp_link.host = sm[3].str();
	}
	else
	{
		tmp_link.host = current_link.host;
	};
	//заполняем страницу
	if (sm[4].length() != 0) {
		if (sm[5].length() == 0)
		{
			tmp_link.query = sm[4].str();
		}
		else
		{
			tmp_link.query = sm[4].str().substr(0, sm[4].length() - sm[5].length()); //страница без ссылки на ней
		}
	}
	else {
		tmp_link.query = '/';
	}
	return tmp_link;
}


std::vector<URL> get_link(const std::string& html, const URL& current_link)
{
	std::regex html_link("<a href=\"(.*?)\"");
	std::vector<URL> links_result;

	auto links_begin = std::sregex_iterator(html.begin(), html.end(), html_link);
	auto links_end = std::sregex_iterator();

	for (std::sregex_iterator i = links_begin; i != links_end; ++i) {
		std::smatch sm = *i;
		if (sm[1].str().at(0) != '#')//не обрабатываем якоря на странице (даёт кртный прирост)
		{
			URL tmp_link = URL_to_Link(sm[1].str(), current_link);
			if (std::find(links_result.begin(), links_result.end(), tmp_link) == links_result.end()) // проверка на повторы
			{
				links_result.push_back(tmp_link);
			}
		}
	}
	return links_result;
}

std::string link_to_string(const URL& link) {
	if (link.protocol == TProtocol::HTTP) {
		return("http://" + link.host + link.query);
	}
	else {
		return("https://" + link.host + link.query);
	}
}

std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str);
	return url_decoded;
}
