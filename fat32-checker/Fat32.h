#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <codecvt>
#include <map>

#include "structs.h"


class Fat32 {

public:


	Fat32(const std::string& filename, bool skipInit = false);

	~Fat32();

private:

	struct HelpTable {
		const File* file = nullptr;
		uint32_t nextCluster = 0;
		bool isLost = false;
	};


	void diskOpener();

	void readBootSector();

	std::vector<uint32_t> readFAT(const BootSector& bs);

	void findClusterCount(const BootSector& bs);

	bool isLFNEntry(const DirEntry& entry);
	std::string decodeLFN(const std::vector<LFNEntry>& entries);
	std::string decodeShortName(const uint8_t name[11]);

	void readDirectory(const BootSector& bs, const std::vector<uint32_t>& fatTable, uint32_t cluster = 0, const std::string& path = "/");

	void findBadClusters(const std::vector<uint32_t>& fatTable);

	void updateFatHelperTable();

	void findLostFiles(const std::vector<uint32_t>& fatTable);

	void FixCyclicFile(const File& file, const std::vector<uint32_t>& fatTable, std::vector<uint32_t>& curedFatTable, uint32_t bytesPerSector, uint32_t sectorsPerCluster);

	BootSector bs_;
	std::vector<uint32_t> fatTable_;                                    // Вектор с таблицей FAT
	std::vector<HelpTable> fatHelperTable_;                             // Вектор с таблицей 
	std::vector<File> lostFiles_;                                       // Вектор для хранения потерянных файлов

	std::vector<uint32_t> curedFatTable_;								// восстановленная FAT
	std::vector<File> curedFiles_;										// восстановленные файлы

	const std::string filename_;                                        // Диск с FAT32
	std::ifstream disk_;
	std::vector<uint32_t> bad_clusters_;
	std::vector<File> files_;

	std::vector<File> cyclicFiles_;										// вектор если зациклились
	std::map<File*, std::vector<uint32_t>> intersectingClusters_;		// вектор если пересечение
	std::vector<File> noEOFfiles_;										// вектор если не нашли EOF

	std::vector<BrokenFileInfo> brokenFiles_;
	friend class MyClassTest;

public:


	void HealCyclicFiles(std::vector<File>& cyclicFiles, const std::vector<uint32_t>& fatTable, std::vector<uint32_t>& curedFatTable, const BootSector& bs);


	void printFatTable(const std::vector<uint32_t>& fatTable);

	void printFiles(const std::vector<File>& files);

	void printBrokenFiles();

	void printLostClusters(const std::vector<uint32_t>& fatTable);


	const std::vector<uint32_t>& getFATTable() const;

	const std::vector<File>& getLostFiles() const;

	const std::vector<File>& getFiles() const;

	const BootSector& getBootSector() const;

	const std::vector<BrokenFileInfo>& getBrokenFiles() const;
};

