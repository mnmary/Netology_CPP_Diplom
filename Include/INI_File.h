#pragma once
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <sstream>
#include "Utils.h"

using myINI = std::map<std::string, std::map<std::string, std::string> >;
using myINI_item = std::map<std::string, std::string>;

class INI_file
{
private:
	myINI _values;
	std::string _fileName;

public:
	INI_file(const std::string& fileName) : _fileName{ fileName }
	{
		int strNum{ 0 };
		std::string str{ "" };
		std::string sectionName{ "" };

		//std::cout << "create a INI parser, file: " << fileName << std::endl;
		//read file
		std::ifstream input(_fileName);
		if (!input)
		{
			std::string a = "������ �������� ����� ";
			a.append(_fileName);
			throw (a.data());
		}

		while (std::getline(input, str)) // ��� ����� ��������� �� ��� ��� ���� �����������.
		{
			strNum++;
			trim(str);
			if (!str.empty())
			{
				//*���� ���� �������� ������������ ���������, ����� ������� � ����������, �� ����� ������ ����������� ��������.

				//������ �� ������
				trim(str);

				//if comment					
				if (str[0] == ';')
				{
					//*������ ������������.
					//����� ������ : `; ������ ������������ `. <br / > ������ ������ ������������ ����� ������, � ���� ������ ������ �� �����.
				}
				else if (str[0] == '[')
				{
					//*������ ����� ������ � ���������� ��� ����� ������ ����������.
					//����� ������ : `[���_������] `. <br / > ������� � ������� ���������, ������� �� ����������� � ����� ����������� ������, ������������.
					if (str.back() == ']')
					{
						//section
						sectionName = str.substr(1, str.size() - 2);
						_values[sectionName];	//������ ��� �� ������ ������ ������(��� ������)
					}
					else
					{
						std::string a = "Section error in line ";
						a.append(std::to_string(strNum));
						throw (a.data());
					}
				}
				else
				{
					//*������ ������� ���������� � ����� �������� ����������.
					//����� ������ : ` ��������_���������� = ��������_���������� `. <br / > ���������� �������� ����� ��������� ��������� ����� ���� ������������.
					//��� �������� ����� �������, ��� �������� ����� ���� ���� �������, ���� ������.��������� �������� �� �����������.						
					std::istringstream iss(str);
					std::string item{ "" }, value{ "" };

					std::getline(iss, item, '=');
					std::getline(iss, value);

					trim(item);
					if (item == "")
					{
						std::string a = "Item error in line ";
						a.append(std::to_string(strNum));
						throw (a.data());
					}

					trim(value);

					//���� ����������� � ����� ������
					size_t flag = value.find(';');
					if (flag != std::string::npos)
					{
						value = value.substr(0, flag - 1);
						trim(value);
					}

					//���������� �������� � ��������� �������
					_values[sectionName][item] = value;

				}
			}
		}
		input.close();

	}
		
	~INI_file() = default;

	std::string get_value(const std::string& sectionAndItem) const
	{
		//��� ����� ����������� ����� ������� INI - ������, ������� ������������� ��� ������������ ���� ��������� �������, ����������� �������� �������� ���������� � ����������� ������.
		size_t flag = sectionAndItem.find('.');
		if (flag == std::string::npos)
		{
			std::string a = "Incorrect format of Section.Item: ";
			a.append(sectionAndItem);
			throw (a.data());
		}
		//������ ����������� ���������� ������
		std::string section = sectionAndItem.substr(0, flag);
		std::string key = sectionAndItem.substr(flag + 1);

		//���� ������
		auto it = _values.find(section);
		if (it == _values.end())
		{
			std::string a = "Section do not exists: ";
			a.append(section);
			throw (a.data());
		}
		//���� ���� � ������
		auto item_it = it->second.find(key);
		if (item_it == it->second.end())
		{
			//*���� �������� �������� ���������� ���, ����� ������� ��������� ��� ������������ � �������� ������ ���������� �� ���� ������.��������, ������������ ����������.
			std::string result{ "\nThis is available items:\n" };
			auto it = _values.find(section);

			for (myINI_item::const_iterator eptr = it->second.begin(); eptr != it->second.end(); eptr++)
			{
				result = result + "- " + eptr->first + "\n";
			}
			std::string a = "Item do not exists: ";
			a.append(sectionAndItem);
			a.append(result);
			throw (a.data());
		}


		if (_values.at(section).at(key) == "")
		{
			//�������� ������
			std::string a = "Value do not exists: ";
			a.append(_values.at(section).at(key).data());
			throw (a.data());
		}
		//������������ � ������ ��� ����� �����
		std::istringstream iss(_values.at(section).at(key));
		std::string value;
		if (!(iss >> value))
		{
			std::string a = "Error of convert value: ";
			a.append(_values.at(section).at(key).data());
			throw (a.data());
		}
		return value;
	}

};

