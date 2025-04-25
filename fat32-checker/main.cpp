#include <iostream>

#include "structs.h"
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

 std::string File::toString() const {
	std::ostringstream oss; // (поток вывода в строку)
	oss << *this;  // Используем перегруженный оператор
	return oss.str();
}

 std::string File::formatFatDateTime(uint16_t date, uint16_t time) {
	// Разбор даты 
	int year = 1980 + ((date >> 9) & 0x7F);
	int month = (date >> 5) & 0x0F;
	int day = date & 0x1F;

	// Разбор времени 
	int hour = (time >> 11) & 0x1F;
	int minute = (time >> 5) & 0x3F;
	int second = (time & 0x1F) * 2;

	std::ostringstream oss;
	oss << std::setfill('0')
		<< std::setw(4) << year << "-"
		<< std::setw(2) << month << "-"
		<< std::setw(2) << day << " "
		<< std::setw(2) << hour << ":"
		<< std::setw(2) << minute << ":"
		<< std::setw(2) << second;

	return oss.str();
}
