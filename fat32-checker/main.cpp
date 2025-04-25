#include <iostream>

#include "Fat32.h"


// валидация: одна латинская буква
bool isValidDriveLetter(const std::string& input) {
	return input.length() == 1 && std::isalpha(input[0]);
}

//  ввод с подтверждением
std::string getDiskPath() {
	std::string letter;
	while (true) {
		std::cout << "Enter drive letter (e.g. F): ";
		std::getline(std::cin, letter);

		// Удалим пробелы
		letter.erase(remove_if(letter.begin(), letter.end(), ::isspace), letter.end());

		if (!isValidDriveLetter(letter)) {
			std::cout << "Invalid input. Please enter a single drive letter.\n\n";
			continue;
		}

		// Приводим к верхнему регистру
		char drive = std::toupper(letter[0]);
		std::string path = R"(\\.\)" + std::string(1, drive) + ":";

		std::cout << "Use disk path: " << path << "? (y/n): ";
		std::string confirm;
		std::getline(std::cin, confirm);
		if (!confirm.empty() && (confirm[0] == 'y' || confirm[0] == 'Y')) {
			return path;
		}

		std::cout << "Let's try again.\n\n";
	}
}



int main() {

	std::locale::global(std::locale("en_US.UTF-8"));                    
	std::string filename = getDiskPath();  
	Fat32* disk;
	try
	{
		disk = new Fat32(filename);
	}
	catch (const std::exception& e) {

		std::cerr  << e.what() << '\n';
	}


	std::cin.get();                                           // Ожидает ввода, чтобы окно консоли не закрывалось сразу
	return 0;
}

// Метод для преобразования в строку

