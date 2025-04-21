#include "Fat32.h"

 Fat32::Fat32(const std::string& filename, bool skipInit)
	: filename_(filename)
{
	if (!skipInit) {
		diskOpener();
		readBootSector();
		readDirectory(bs_, fatTable_);
		//findBadClusters(fatTable_);
		updateFatHelperTable();
		findLostFiles(fatTable_);

		printBrokenFiles();
	}
}

 Fat32::~Fat32() {
	if (disk_.is_open()) {
		disk_.close();
	}
}

inline void Fat32::diskOpener() {
	disk_.open(filename_, std::ios::binary);
	if (!disk_) {
		// вызвать деструктор
		throw std::runtime_error("file is not open\n");
	}
	std::cout << "file is open!\n";
}

inline void Fat32::readBootSector() {

	disk_.read(reinterpret_cast<char*>(&bs_), sizeof(BootSector));             // reinterpret_cast<char*>(&bs) преобразует указатель на структуру bs в указатель на массив байтов, то есть на char*. Почему именно char? Потому что в C++ char всегда представляет собой 1 байт данных.
	if (!disk_) {                                                              // После этого ты получаешь указатель на первый байт структуры BootSector, и таким образом можешь записать в эту структуру данные, прочитанные из файла.
		throw std::runtime_error("Error: Failed to read boot sector\n");
		return;
	}
	if (std::string(bs_.fsType, 8) != "FAT32   ") {
		throw std::runtime_error("Error: File system is not FAT 32 \n");
		return;
	}
	std::cout << "File system: " << std::string(bs_.fsType, 8) << "\n";
	std::cout << "Cluster size: " << (bs_.sectorsPerCluster * bs_.bytesPerSector) << " bytes\n";
	std::cout << "Root directory's first cluster: " << bs_.rootCluster << "\n";
	std::cout << "FAT size: " << (bs_.sectorsPerFAT32 * bs_.bytesPerSector) << " bytes\n";

	fatTable_ = readFAT(bs_);
}

inline std::vector<uint32_t> Fat32::readFAT(const BootSector& bs) {
	uint32_t fatSize = bs.sectorsPerFAT32 * bs.bytesPerSector;
	std::vector<uint32_t> fatTable(fatSize / sizeof(uint32_t));

	uint32_t fatOffset = bs.reservedSectors * bs.bytesPerSector;                // Адрес начала FAT
	disk_.seekg(fatOffset, std::ios::beg);                                      // Функция seekg перемещает указатель чтения в файл на заданное смещение. std::ios::beg означает, что отсчёт идет от начала файла (то есть, указатель будет перемещен на fatOffset байтов от начала файла). Это гарантирует, что мы начинаем читать таблицу FAT именно с того места, где она начинается на диске.
	disk_.read(reinterpret_cast<char*>(fatTable.data()), fatSize);              // преобразует указатель на вектор в указатель на массив байтов (массив символов char). Это необходимо, потому что функция read работает с потоком байтов, а не с типизированными данными.

	if (!disk_) {
		throw std::runtime_error("Error: Failed to read FAT\n");
	}
	return fatTable;
}

inline void Fat32::findClusterCount(const BootSector& bs) {
	uint32_t totalSectors = bs.totalSectors32;
	uint32_t fatSectors = bs.sectorsPerFAT32 * bs.numFATs;
	uint32_t dataSectors = totalSectors - (bs.reservedSectors + fatSectors);
	uint32_t totalClusters = dataSectors / bs.sectorsPerCluster;

	std::cout << "Total clusters: " << totalClusters << "\n";
}

inline bool Fat32::isLFNEntry(const DirEntry& entry) {
	return (entry.attr & 0x0F) == 0x0F;
}

inline std::string Fat32::decodeLFN(const std::vector<LFNEntry>& entries) {
	std::u16string name;
	for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
		const LFNEntry& e = *it;
		for (char16_t ch : e.name1) if (ch != 0xFFFF && ch != 0) name += ch;
		for (char16_t ch : e.name2) if (ch != 0xFFFF && ch != 0) name += ch;
		for (char16_t ch : e.name3) if (ch != 0xFFFF && ch != 0) name += ch;
	}

	// Конвертация из UTF-16 в UTF-8
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(name);
}

inline std::string Fat32::decodeShortName(const uint8_t name[11]) {
	std::string shortName;
	for (int i = 0; i < 8 && name[i] != ' '; ++i)
		shortName += static_cast<char>(name[i]);
	if (name[8] != ' ')
		shortName += '.' + std::string(reinterpret_cast<const char*>(&name[8]), 3);
	return shortName;
}

inline void Fat32::readDirectory(const BootSector& bs, const std::vector<uint32_t>& fatTable, uint32_t cluster, const std::string& path) { //кластер, с которого начинается чтение (по умолчанию 0)

	uint32_t dataBegin = bs.reservedSectors + (bs.numFATs * bs.sectorsPerFAT32);                        // определяет начало данных на диске, который включает все зарезервированные сектора и сектора, занятые таблицами FAT. Если cluster равен 0 (по умолчанию), это означает, что нужно начать с корневого кластера, указанный в rootCluster.
	if (cluster == 0) {
		cluster = bs.rootCluster;
	}

	const uint32_t bytesPerCluster = bs.bytesPerSector * bs.sectorsPerCluster;
	std::vector<LFNEntry> lfnBuffer;                                                                    // Вектор lfnBuffer используется для хранения записей длинных имён файлов (LFN, Long File Name).

	while (cluster < 0x0FFFFFF8 && cluster >= 2) {                                                      // Пока значение cluster не указывает на специальный маркер окончания цепочки кластеров (0x0FFFFFF8) и не меньше 2 (так как кластеры с номерами 0 и 1 зарезервированы).
		if (cluster >= fatTable.size()) {
			std::cerr << "Ошибка: кластер " << cluster << " выходит за границы FAT-таблицы.\n";
			return;
		}

		uint32_t sector = dataBegin + (cluster - 2) * bs.sectorsPerCluster;
		uint32_t offset = sector * bs.bytesPerSector;

		disk_.seekg(offset);
		std::vector<uint8_t> clusterData(bytesPerCluster);
		disk_.read(reinterpret_cast<char*>(clusterData.data()), bytesPerCluster);

		for (size_t i = 0; i < bytesPerCluster; i += 32) {
			DirEntry* entry = reinterpret_cast<DirEntry*>(&clusterData[i]); // лишь указатель на область байтов не новая структура

			if (entry->name[0] == 0x00) return;
			if (entry->name[0] == 0xE5) continue;

			if (isLFNEntry(*entry)) {
				lfnBuffer.push_back(*reinterpret_cast<LFNEntry*>(entry));
				continue;
			}

			std::string fileEntryName = lfnBuffer.empty() ? decodeShortName(entry->name)
				: decodeLFN(lfnBuffer);
			lfnBuffer.clear();

			uint32_t firstCluster = (entry->fstClusHi << 16) | entry->fstClusLo;

			if ((entry->attr & 0x10) && (fileEntryName == "." || fileEntryName == "..")) continue;

			std::string fullPath = path + fileEntryName;


			File file;
			file.attr_int = entry->attr;
			entry->attr & 0x10 ? file.attr = "[DIR] " : file.attr = "[FILE]";
			file.filename = fullPath;
			file.name = fileEntryName;
			file.fileSize = entry->fileSize;
			file.firstClaster = firstCluster;
			file.wrtDate = entry->wrtDate;
			if (fileEntryName == "OLD") {
				std::cout << "p";
			}
			file.wrtTime = entry->wrtTime;

			files_.push_back(file);

			if ((entry->attr & 0x10) && firstCluster >= 2)
				readDirectory(bs, fatTable, firstCluster, fullPath + "/");
		}

		cluster = fatTable[cluster];
	}
}

inline void Fat32::findBadClusters(const std::vector<uint32_t>& fatTable) {
	for (size_t i = 0; i < fatTable.size(); ++i) {
		if (fatTable[i] == 0x0FFFFFF7) {
			bad_clusters_.push_back(i);
		}

	}
}

inline void Fat32::updateFatHelperTable() {
	fatHelperTable_.resize(fatTable_.size());	// переместить вызов кудато

	for (const auto& file : files_) {                       // берем ФАЙл 
		uint32_t cluster = file.firstClaster;

		std::unordered_set<uint32_t> visited;
		bool isBroken = false;
		std::string errorReason;
		bool isLoop = false;
		bool isNoEOF = false;

		while (cluster < fatTable_.size()) {
			errorReason = "";
			if (visited.count(cluster)) {
				errorReason += "Cluster loop detected |";
				isBroken = true;
				isLoop = true;
				break;
			}
			visited.insert(cluster);

			if (fatHelperTable_[cluster].file != nullptr) {
				isBroken = true;
				errorReason += "Cluster is already in use by another file:\t  "
					+ fatHelperTable_[cluster].file->filename + "  "
					+ fatHelperTable_[cluster].file->attr + "|";

				intersectingClusters_[const_cast<File*>(&file)].push_back(cluster);
			}

			uint32_t nextCluster = fatTable_[cluster];
			fatHelperTable_[cluster] = HelpTable{ &file, nextCluster };

			if (nextCluster >= 0x0FFFFFF8 && nextCluster <= 0x0FFFFFFF) {
				break; // EOF
			}

			if (nextCluster == 0x00000000) {
				errorReason += "File chain ends on a free cluster |";
				isBroken = true;
				isNoEOF = true;
				break;
			}

			if (nextCluster >= fatTable_.size()) {
				errorReason += "Next cluster index is out of FAT table bounds |";
				isBroken = true;
				break;
			}

			cluster = nextCluster;
		}

		if (isBroken) {
			brokenFiles_.push_back(BrokenFileInfo{ file.filename, errorReason });

			if (isLoop) {
				cyclicFiles_.push_back(file);
			}

			if (isNoEOF) {
				noEOFfiles_.push_back(file);
			}
		}
	}
}

inline void Fat32::findLostFiles(const std::vector<uint32_t>& fatTable) {

	for (size_t cluster = 0; cluster < fatTable.size(); ++cluster) {
		if (fatTable[cluster] == 0x0FFFFFF7) {														// bad clusters
			continue;
		}

		// Проверяем, есть ли этот кластер в fatHelperTable_
		if (cluster >= fatHelperTable_.size() || !fatHelperTable_[cluster].file) {
			std::vector<uint32_t> lostChain;
			uint32_t currentCluster = cluster;

			// Восстанавливаем цепочку кластеров для этого потерянного файла
			while (currentCluster < fatTable.size() && fatTable[currentCluster] != 0x0FFFFFF7 &&
				fatTable[currentCluster] != 0x00000000 && fatTable[currentCluster] < 0x0FFFFFF8) {

				lostChain.push_back(currentCluster);

				uint32_t nextCluster = fatTable[currentCluster];
				fatHelperTable_[currentCluster] = HelpTable{										// обновляем fatHelperTable_
					nullptr,
					nextCluster
				};

				currentCluster = fatTable[currentCluster];
			}

			if (!lostChain.empty()) {
				File file;
				file.firstClaster = lostChain[0];
				lostFiles_.push_back(file);
				File* savedFile = &lostFiles_.back();

				for (uint32_t c : lostChain) {
					fatHelperTable_[c].file = savedFile;
					fatHelperTable_[c].isLost = true;
				}
			}
		}
	}

	for (size_t cluster = 0; cluster < fatTable.size(); ++cluster) {				//  цикл для потерянных однокластерных файлов которые указывают на еоф 
		if (fatTable[cluster] >= 0x0FFFFFF8 && fatTable[cluster] <= 0x0FFFFFFF) {	// EOF

			if (cluster >= fatHelperTable_.size() || !fatHelperTable_[cluster].file) {
				File file;
				file.firstClaster = static_cast<uint32_t>(cluster);
				lostFiles_.push_back(file);
				File* savedFile = &lostFiles_.back();

				fatHelperTable_[cluster].file = savedFile;
				fatHelperTable_[cluster].isLost = true;
			}
		}
	}
}

inline void Fat32::printFatTable(const std::vector<uint32_t>& fatTable) {

	for (size_t i = 0; i < 50 && i < fatTable.size(); ++i) {
		if (fatTable[i] == 0x0FFFFFF7 || fatTable[i] == 0x0FFFFFF8) {
			std::cout << "FAT[" << i << "] = " << fatTable[i] << " (BAD CLUSTER)";
			//bad_files.push_back(i);
		}
		else if (fatTable[i] == 0x0FFFFFFF)
		{
			std::cout << "FAT[" << i << "] = " << fatTable[i] << " (EOF)";
		}
		else
		{
			std::cout << "FAT[" << i << "] = " << fatTable[i];
		}
		std::cout << "\n";
	}
}

inline void Fat32::printFiles(const std::vector<File>& files) {

	for (auto file : files) {
		std::cout << file;
	}
}

inline void Fat32::printBrokenFiles() {
	for (const auto& broken : brokenFiles_) {
		std::cout << "Broken file: " << broken.filename
			<< " ---   " << broken.errorMessage << "\n";
	}
}

inline void Fat32::printLostClusters(const std::vector<uint32_t>& fatTable) {
	// Выводим потерянные файлы
	for (const auto& lostFile : lostFiles_) {
		uint32_t currentCluster = lostFile.firstClaster;
		std::cout << "Lost file starting with cluster " << currentCluster << " has clusters: ";

		// Восстанавливаем цепочку кластеров из таблицы FAT
		while (currentCluster < fatTable.size() && fatTable[currentCluster] != 0x00000000) {
			std::cout << currentCluster << " ";
			currentCluster = fatTable[currentCluster];
		}

		std::cout << ".\n";
	}
}

inline const std::vector<uint32_t>& Fat32::getFATTable() const {
	return fatTable_;
}

inline const std::vector<File>& Fat32::getLostFiles() const {
	return lostFiles_;
}

inline const std::vector<File>& Fat32::getFiles() const {
	return files_;
}

inline const BootSector& Fat32::getBootSector() const {
	return bs_;
}

inline const std::vector<BrokenFileInfo>& Fat32::getBrokenFiles() const {
	return brokenFiles_;
}
