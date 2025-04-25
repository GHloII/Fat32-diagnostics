#include "pch.h"
#include "CppUnitTest.h"
#define private public  // Даем доступ к приватным членам
#include "../fat32-checker/main.cpp"
#include"../fat32-checker/fat32.cpp"
#undef private

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest

{
    void writeFilesToFile(const std::vector<File>& files, const std::string& outputPath) {
        // Открываем файл для записи (перезапишется, если файл существует)
        std::ofstream outFile("./HHHHHHHHHHHHHHHHHHHHHHH.txt");

        if (!outFile.is_open()) {
            std::cerr << "Ошибка при открытии файла для записи." << std::endl;
            return;
        }

        // Проходим по всем файлам и записываем их в файл
        for (const auto& file : files) {
            // Записываем данные каждого файла в файл
            outFile << "Имя файла: " << file.name << std::endl;
            outFile << "Первый кластер: " << file.firstClaster << std::endl;
            outFile << "Размер файла: " << file.fileSize << std::endl;
            outFile << "-----------------------------------" << std::endl;
        }

        // Закрываем файл
        outFile.close();
    }
    void writeBrokenFilesToFile(const std::vector<BrokenFileInfo>& brokenFiles, const std::string& filename) {
        std::ofstream outFile("./ffffffffffffffffff.txt");  // Открываем файл для записи
        if (outFile.is_open()) {
            for (const auto& broken : brokenFiles) {
                outFile << broken.filename << " --- " << broken.errorMessage << "\n";  // Записываем каждую строку
            }
            outFile.close();  // Закрываем файл после записи
        }
        else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void writeLostClustersToFile(const std::vector<uint32_t>& fatTable, const std::vector<File>& lostFiles) {
        std::ofstream outFile("PPPPPPPPPPPPPPPP.txt");
        if (!outFile.is_open()) {
            std::cerr << "Error opening file for writing.\n";
            return;
        }

        for (const auto& lostFile : lostFiles) {
            uint32_t currentCluster = lostFile.firstClaster;
            outFile << "Lost file starting with cluster " << currentCluster << " has clusters: ";

            while (currentCluster < fatTable.size() && fatTable[currentCluster] != 0x00000000) {
                outFile << currentCluster << " ";
                currentCluster = fatTable[currentCluster];
            }

            outFile << ".\n";
        }

        outFile.close();
    }

    TEST_CLASS(MyClassTest)
    {
    protected:
        Fat32* fs;

        // Инициализация данных для теста
        TEST_METHOD_INITIALIZE(SetUp)
        {
            fs = new Fat32("42.img", true);  // Инициализация объекта Fat32
        }

        // Очистка после теста
        TEST_METHOD_CLEANUP(TearDown)
        {
            delete fs;  // Удаление объекта после теста
        }

    public:
        TEST_METHOD(UpdateFatHelperTableWorks)
        {
            // Подставим тестовые значения в fatTable_
            fs->fatTable_ = {
                0, 0, 0, 0,      // 0–3
                7, 6, 3, 9,      // 4–7
                5, 8, 4, 0,      // 8–11
                0x0FFFFFF7,      // 12 — bad cluster
                0x0FFFFFFF,      // 13 — EOF
                13, 14, 15,      // 14–16
                0, 0,            // 17–18
                20, 21, 22, 8,   // 19–22
                0,               // 23
                25, 26, 27, 28,  // 24–27
                26,              // 28
                0,               // 29
                0x0FFFFFFF,      // 30 — EOF
                0, 0             // 31–32
            };

            fs->files_ = {
                File{ "A.txt", "A.txt", "", 0, 10 },
                File{ "C.doc", "C.doc", "", 0, 19 },
                File{ "D.bin", "D.bin", "", 0, 24 }
            };

            // Вызовем метод, который хотим протестировать
            fs->updateFatHelperTable();
            fs->findLostFiles(fs->fatTable_);
            writeFilesToFile(fs->getFiles(), "output.txt");
            writeBrokenFilesToFile(fs->getBrokenFiles(), "./ooooo.txt");
            writeLostClustersToFile(fs->getFATTable(), fs->getLostFiles());
            // Проверка: можно добавить дополнительные проверки с ASSERT или EXPECT
            // Например:
            // Assert::IsTrue(fs->fatHelperTable_.size() > 0d);  // Проверка, что таблица не пуста
        }
    };
}
