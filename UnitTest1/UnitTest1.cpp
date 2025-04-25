#include "pch.h"
#include "CppUnitTest.h"
#define private public   
#include "../fat32-checker/main.cpp"
#include"../fat32-checker/fat32.cpp"
#include <set>
#undef private

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest

{

	TEST_CLASS(MyClassTest)
	{
	protected:
		Fat32* fs;

		// ������������� ������ 
		TEST_METHOD_INITIALIZE(SetUp)
		{
			BootSector bs{};
			bs.bytesPerSector = 512;
			bs.sectorsPerCluster = 32;

			std::vector<uint32_t> fatTable = {
				0, 0, 0, 0,
				7, 6, 3, 9,
				5, 8, 4, 0,
				0x0FFFFFF7,
				0x0FFFFFFF,
				13, 14, 15,
				0, 0,
				20, 21, 22, 8,
				0,
				25, 26, 27, 28,
				26,
				0,
				0x0FFFFFFF,
				0, 0
			};

			// ������ ������ ������
			std::vector<File> files = {
				File{ "A.txt", "A.txt", "", 0, 10 },       // ��� EOF
				//File{ "B.txt", "B.txt", "", 0, 16 },       //  �������
				File{ "C.txt", "C.txt", "", 0, 19 },       // ������������ � A
				File{ "D.txt", "D.txt", "", 0, 24 },       // ����������
				//File{ "E.txt", "E.txt", "", 0, 30 }        // �������
			};

			
			fs = new Fat32(bs, fatTable, files);
		}

		TEST_METHOD_CLEANUP(TearDown)
		{
			delete fs;
		}

	public:
		

		// ���� �� ����������� ����� (  D)
		TEST_METHOD(DetectsCyclicFiles)
		{
			fs->updateFatHelperTable();
			fs->findLostFiles(fs->fatTable_);

			auto cyclic = fs->getCyclicFiles();  

			Assert::AreEqual<size_t>(1, cyclic.size(), L"�������� 1 ����������� ����");
			Assert::AreEqual(std::string("D.txt"), cyclic[0].name, L"�������� D.txt ��� �����������");
		}

		// ���� �� �������������� ����� (  C)
		TEST_METHOD(DetectsIntersectingFiles)
		{
			fs->updateFatHelperTable();
			fs->findLostFiles(fs->fatTable_);

			auto intersecting = fs->getIntersectingClusters();  

			Assert::AreEqual<size_t>(1, intersecting.size(), L"�������� 1 ���� � ������������");
			auto* fileC = intersecting.begin()->first;
			Assert::AreEqual(std::string("C.txt"), fileC->name, L"�������� C.txt ��� ��������������");
		}

		// ���� �� ����� ��� EOF (  A � C)
		TEST_METHOD(DetectsNoEOFFiles)
		{
			fs->updateFatHelperTable();
			fs->findLostFiles(fs->fatTable_);

			auto noEOF = fs->getNoEOFfiles();  

			std::set<std::string> noEOFnames;
			for (auto& f : noEOF) noEOFnames.insert(f.name);

			Assert::IsTrue(noEOFnames.count("A.txt") == 1, L"A.txt ������ ���� � ������ ��� EOF");
			Assert::IsTrue(noEOFnames.count("C.txt") == 1, L"C.txt ������ ���� � ������ ��� EOF");
		}

	};

}
