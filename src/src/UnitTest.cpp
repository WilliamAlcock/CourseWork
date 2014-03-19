#include "UnitTest.h"
#include "bitFunctions.cpp"
#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include "Directory.h"
#include "HDD.h"
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>

int main() {
	UnitTest myUnitTest;
//	ofstream out("out.txt");
//	streambuf *coutbuf = std::cout.rdbuf(); //save old buf
//	cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
	myUnitTest.runSuperBlockTest();
	myUnitTest.runFreeBlockListTest();
	myUnitTest.runINodeListTest();
	myUnitTest.runDirectoryTest();
//	std::cout.rdbuf(coutbuf); //reset to standard output again

}

void UnitTest::runDirectoryTest() {
	const int driveSizeLength = 3;
	int driveSize[driveSizeLength] = {
		1024 * 1024 * 128, 	// 128MB
		1024 * 1024 * 64,	// 64MB
		1024 * 1024 * 32	// 32MB
	};

	const int blockSizeLength = 3;
	int blockSize[blockSizeLength] = {
		1024,				// 1KB
		512,				// 512bytes
		256					// 256bytes
	};

	HDD* hdd[driveSizeLength * blockSizeLength];
	SuperBlock* superBlock[driveSizeLength * blockSizeLength];
	FreeBlockList* freeBlockList[driveSizeLength * blockSizeLength];
	INodeList* iNodeList[driveSizeLength * blockSizeLength];
	Directory* directory[driveSizeLength * blockSizeLength];

	for (int i=0;i<driveSizeLength;i++) {
		for (int j=0;j<blockSizeLength;j++) {
			cout << "****** TESTING DIRECTORY " << ((i*3) + j) << " *****" << endl;
			cout << "Creating HDD of size " << driveSize[i] << endl;
			hdd[(i*3) + j] = new HDD(driveSize[i]);
			hdd[(i*3) + j]->setDebug(false);
			assert(hdd[(i*3) + j]->capacity() == driveSize[i]);
			cout << "Creating Super Block with HDD" << endl;
			superBlock[(i*3) + j] = new SuperBlock(hdd[(i*3) + j]);
			cout << "Writing Super Block to HDD with blockSize " << blockSize[j] << endl;
			superBlock[(i*3) + j]->write(blockSize[j]);
			cout << "Creating FreeBlockList" << endl;
			freeBlockList[(i*3) + j] = new FreeBlockList(hdd[(i*3) + j], superBlock[(i*3) + j]->getNumberOfBlocks(), superBlock[(i*3) + j]->getBlockSize(), superBlock[(i*3) + j]->getFreeBlockStart());
			freeBlockList[(i*3) + j]->write();
			cout << "Creating INodeList" << endl;
			iNodeList[(i*3) + j] = new INodeList(hdd[(i*3) + j], superBlock[(i*3) + j], freeBlockList[(i*3) + j]);
			int rootBlock = freeBlockList[(i*3) + j]->getBlock();
			iNodeList[(i*3) + j]->write(rootBlock);
			cout << "Creating Directory" << endl;
			directory[(i*3) + j] = new Directory(hdd[(i*3) + j], superBlock[(i*3) + j], freeBlockList[(i*3) + j], iNodeList[(i*3) + j], 0);
			directory[(i*3) + j]->write();
			cout << "Testing - create 50 directorys in the root directory" << endl;
			string directoryNames[50];
			for (int loop=0;loop<50;loop++) {
				directoryNames[loop] = "MyDIR" + to_string(loop);
				cout << "creating directory - '" << directoryNames[loop] << "'" << endl;
				assert (directory[(i*3) + j]->createDir("",directoryNames[loop]) == true);
			}
			cout << "Testing - creating 5 directorys in each of those directorys" << endl;
			string secondLevelDirectoryNames[50][5];
			for (int loop=0;loop<50;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					secondLevelDirectoryNames[loop][loop2] = "SecondDirectory" + to_string(loop2);
					cout << "creating directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << endl;
					assert (directory[(i*3) + j]->createDir(directoryNames[loop]+"/",secondLevelDirectoryNames[loop][loop2]) == true);
				}
			}
			cout << "Testing - creating 5 directorys in each of those directorys" << endl;
			string thirdLevelDirectoryNames[50][5][5];
			for (int loop=0;loop<50;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						thirdLevelDirectoryNames[loop][loop2][loop3] = "ThirdDirectory" + to_string(loop3);
						cout << "creating directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->createDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3]) == true);
					}
				}
			}
			cout << "Testing - Trying to delete the second level directorys (this should be impossible as they have a contents)" << endl;
			for (int loop=0;loop<50;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					cout << "deleting directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << endl;
					assert (directory[(i*3) + j]->deleteDir(directoryNames[loop]+"/",secondLevelDirectoryNames[loop][loop2]) == false);
				}
			}
			cout << "Testing - Deleting the third level directorys " << endl;
			for (int loop=0;loop<50;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "deleting directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->deleteDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3]) == true);
					}
				}
			}
			cout << "Testing - Recreating half the third level directorys" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "creating directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->createDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3]) == true);
					}
				}
			}
			cout << "Testing - Moving the first half to the second half" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "moving directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						cout << "              to - '" << directoryNames[25+loop] << "/" << secondLevelDirectoryNames[loop+25][loop2] << "/" << thirdLevelDirectoryNames[loop+25][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->moveDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3],
																directoryNames[25+loop]+"/"+secondLevelDirectoryNames[loop+25][loop2]+"/",thirdLevelDirectoryNames[loop+25][loop2][loop3] ) == true);
					}
				}
			}
			cout << "Testing - Deleting the directorys" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "deleting directory - '" << directoryNames[25+loop] << "/" << secondLevelDirectoryNames[loop+25][loop2] << "/" << thirdLevelDirectoryNames[loop+25][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->deleteDir(directoryNames[25+loop]+"/"+secondLevelDirectoryNames[loop+25][loop2]+"/",thirdLevelDirectoryNames[loop+25][loop2][loop3] ) == true);
					}
				}
			}
			cout << "Testing - Trying to delete the earlier directorys (This should be impossible)" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "deleting directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->deleteDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3] ) == false);
					}
				}
			}
			int numberOfFreeNodesBef = iNodeList[(i*3) + j]->getNumberOfFreeINodes();
			cout << "Testing - Recreating half the third level directorys" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "creating directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->createDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3]) == true);
					}
				}
			}
			int numberOfFreeNodesAft = iNodeList[(i*3) + j]->getNumberOfFreeINodes();
			cout << "Testing - creating hard links from the first half to the second half" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "creating link from - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						cout << "                to - '" << directoryNames[25+loop] << "/" << secondLevelDirectoryNames[loop+25][loop2] << "/" << thirdLevelDirectoryNames[loop+25][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->makeHardLinkDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3],
																directoryNames[25+loop]+"/"+secondLevelDirectoryNames[loop+25][loop2]+"/",thirdLevelDirectoryNames[loop+25][loop2][loop3] ) == true);
					}
				}
			}
			assert (iNodeList[(i*3) + j]->getNumberOfFreeINodes() == numberOfFreeNodesAft);

			cout << "Testing - Deleting the first half of directorys" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "deleting directory - '" << directoryNames[loop] << "/" << secondLevelDirectoryNames[loop][loop2] << "/" << thirdLevelDirectoryNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->deleteDir(directoryNames[loop]+"/"+secondLevelDirectoryNames[loop][loop2]+"/",thirdLevelDirectoryNames[loop][loop2][loop3] ) == true);
					}
				}
			}

			assert (iNodeList[(i*3) + j]->getNumberOfFreeINodes() == numberOfFreeNodesAft);
			cout << "Testing - Deleting the second half of directorys" << endl;
			for (int loop=0;loop<25;loop++) {
				for (int loop2=0;loop2<5;loop2++) {
					for (int loop3=0;loop3<5;loop3++) {
						cout << "deleting directory - '" << directoryNames[loop+25] << "/" << secondLevelDirectoryNames[loop+25][loop2] << "/" << thirdLevelDirectoryNames[loop+25][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->deleteDir(directoryNames[loop+25]+"/"+secondLevelDirectoryNames[loop+25][loop2]+"/",thirdLevelDirectoryNames[loop+25][loop2][loop3] ) == true);
					}
				}
			}
			assert (iNodeList[(i*3) + j]->getNumberOfFreeINodes() == numberOfFreeNodesBef);

			string level1dirNames[3];
			string level1fileNames[3][6];
			string level1fileData[3][6];

			string level2dirNames[3][3];
			string level2fileNames[3][3][6];
			string level2fileData[3][3][6];

			string level3dirNames[3][3][3];
			string level3fileNames[3][3][3][6];
			string level3fileData[3][3][3][6];
			cout << "Creating 3 directorys each with 3 directorys each with 3 directorys creating 6 files in each" << endl;
			for (int loop=0;loop<3;loop++){
				level1dirNames[loop] = "dir" + to_string(loop);
				cout << "Creating level1 dir " << level1dirNames[loop] << endl;
				assert (directory[(i*3) + j]->createDir("",level1dirNames[loop]) == true);
				for (int fileLoop=0;fileLoop<6;fileLoop++) {
					level1fileNames[loop][fileLoop] = "file" + to_string(fileLoop);
					level1fileData[loop][fileLoop] = createRandomFile();
					cout << "Creating level1 files " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->createFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],level1fileData[loop][fileLoop]) == true);
				}
				for (int loop2=0;loop2<3;loop2++){
					level2dirNames[loop][loop2] = "dir" + to_string(loop2);
					cout << "Creating level2 dir " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << endl;
					assert (directory[(i*3) + j]->createDir(level1dirNames[loop]+"/",level2dirNames[loop][loop2]) == true);
					for (int fileLoop=0;fileLoop<6;fileLoop++) {
						level2fileNames[loop][loop2][fileLoop] = "file" + to_string(fileLoop);
						level2fileData[loop][loop2][fileLoop] = createRandomFile();
						cout << "Creating level2 files " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->createFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],level2fileData[loop][loop2][fileLoop]) == true);
					}
					for (int loop3=0;loop3<3;loop3++){
						level3dirNames[loop][loop2][loop3] = "dir" + to_string(loop3);
						cout << "Creating level3 dir " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << endl;
						assert (directory[(i*3) + j]->createDir(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3dirNames[loop][loop2][loop3]) == true);
						for (int fileLoop=0;fileLoop<6;fileLoop++) {
							level3fileNames[loop][loop2][loop3][fileLoop] = "file" + to_string(fileLoop);
							level3fileData[loop][loop2][loop3][fileLoop] = createRandomFile();
							cout << "Creating level3 files " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->createFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],level3fileData[loop][loop2][loop3][fileLoop]) == true);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Reading the files from the disk to make sure they are the same" << endl;
			string file;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop++) {
					cout << "reading file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],file);
					assert (file == level1fileData[loop][fileLoop]);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop++) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],file);
						assert (file == level2fileData[loop][loop2][fileLoop]);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop++) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file);
							assert (file == level3fileData[loop][loop2][loop3][fileLoop]);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Deleting half the files from the disk" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "deleting file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->deleteFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop]) == true);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "deleting file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->deleteFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop]) == true);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "deleting file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->deleteFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop]) == true);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Trying to read those files from the disk - this should not be possible" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "reading file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],file) == false);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],file) == false);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file) == false);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Reading the remaining files" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=1;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "reading file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],file);
					assert (file == level1fileData[loop][fileLoop]);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=1;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],file);
						assert (file == level2fileData[loop][loop2][fileLoop]);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=1;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file);
							assert (file == level3fileData[loop][loop2][loop3][fileLoop]);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Recreating the deleted files" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "creating file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->createFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],level1fileData[loop][fileLoop]) == true);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "creating file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->createFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],level2fileData[loop][loop2][fileLoop]) == true);

					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "creating file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->createFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],level3fileData[loop][loop2][loop3][fileLoop]) == true);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Renaming the files" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					string newName = "renamedFile" + to_string(fileLoop);
					cout << "renaming file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << " to " << newName << endl;
					assert (directory[(i*3) + j]->renameFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],newName) == true);
					level1fileNames[loop][fileLoop] = newName;
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						string newName = "renamedFile" + to_string(fileLoop);
						cout << "renaming file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << " to " << newName << endl;
						assert (directory[(i*3) + j]->renameFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],newName) == true);
						level2fileNames[loop][loop2][fileLoop] = newName;
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							string newName = "renamedFile" + to_string(fileLoop);
							cout << "renaming file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << " to " << newName << endl;
							assert (directory[(i*3) + j]->renameFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],newName) == true);
							level3fileNames[loop][loop2][loop3][fileLoop] = newName;
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Reading the renamed files to check they are still ok" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "reading file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],file) == true);
					assert (file == level1fileData[loop][fileLoop]);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],file) == true);
						assert (file == level2fileData[loop][loop2][fileLoop]);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file) == true);
							assert (file == level3fileData[loop][loop2][loop3][fileLoop]);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Trying to read the old files - this should cause an error" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					string newFile = "file" + to_string(fileLoop);
					cout << "reading file " << level1dirNames[loop] << "/" << newFile << endl;
					assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",newFile,file) == false);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						string newFile = "file" + to_string(fileLoop);
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << newFile << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",newFile,file) == false);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							string newFile = "file" + to_string(fileLoop);
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << newFile << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",newFile,file) == false);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Trying to read some directorys - this should cause an error" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					string newFile = "file" + to_string(fileLoop);
					cout << "reading file " << level1dirNames[loop] << endl;
					assert (directory[(i*3) + j]->readFile("",level1dirNames[loop],file) == false);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						string newFile = "file" + to_string(fileLoop);
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level2dirNames[loop][loop2],file) == false);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							string newFile = "file" + to_string(fileLoop);
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3dirNames[loop][loop2][loop3],file) == false);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Moving all files to their parent directories" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "moving file " << level1dirNames[loop] << "/" << level1fileNames[loop][fileLoop];
					string newFile = level1fileNames[loop][fileLoop] + "Moved" + level1dirNames[loop];
					cout << " to " << "/" << newFile << endl;
					assert (directory[(i*3) + j]->moveFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],"",newFile) == true);
					cout << "Checking old file does not exist" << endl;
					assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level1fileNames[loop][fileLoop],file) == false);
					level1fileNames[loop][fileLoop] = newFile;
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "moving file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level2fileNames[loop][loop2][fileLoop];
						string newFile = level2fileNames[loop][loop2][fileLoop] + "Moved" + level2dirNames[loop][loop2];
						cout << " to " << level1dirNames[loop] << "/" << newFile << endl;
						assert (directory[(i*3) + j]->moveFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],level1dirNames[loop]+"/",newFile) == true);
						cout << "Checking old file does not exist" << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level2fileNames[loop][loop2][fileLoop],file) == false);
						level2fileNames[loop][loop2][fileLoop] = newFile;
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "moving file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << level3fileNames[loop][loop2][loop3][fileLoop];
							string newFile = level3fileNames[loop][loop2][loop3][fileLoop] + "Moved" + level3dirNames[loop][loop2][loop3];
							cout << " to " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << newFile << endl;
							assert (directory[(i*3) + j]->moveFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",newFile) == true);
							cout << "Checking old file does not exist" << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file) == false);
							level3fileNames[loop][loop2][loop3][fileLoop] = newFile;
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Checking moved files can be read" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "reading file " << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->readFile("",level1fileNames[loop][fileLoop],file) == true);
					assert (level1fileData[loop][fileLoop] == file);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level2fileNames[loop][loop2][fileLoop],file) == true);
						assert (level2fileData[loop][loop2][fileLoop] == file);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file) == true);
							assert (level3fileData[loop][loop2][loop3][fileLoop] == file);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Testing rewriting files " << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "rewriting file " << level1fileNames[loop][fileLoop] << endl;
					level1fileData[loop][fileLoop] = createRandomFile();
					assert (directory[(i*3) + j]->rewriteFile("",level1fileNames[loop][fileLoop],level1fileData[loop][fileLoop]) == true);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "rewriting file " << level1dirNames[loop] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						level2fileData[loop][loop2][fileLoop] = createRandomFile();
						assert (directory[(i*3) + j]->rewriteFile(level1dirNames[loop]+"/",level2fileNames[loop][loop2][fileLoop],level2fileData[loop][loop2][fileLoop]) == true);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "rewriting file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							level3fileData[loop][loop2][loop3][fileLoop] = createRandomFile();
							assert (directory[(i*3) + j]->rewriteFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3fileNames[loop][loop2][loop3][fileLoop],level3fileData[loop][loop2][loop3][fileLoop]) == true);
						}
					}
				}
			}
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Reading rewriten files " << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "reading file " << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->readFile("",level1fileNames[loop][fileLoop],file) == true);
					assert (level1fileData[loop][fileLoop] == file);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level2fileNames[loop][loop2][fileLoop],file) == true);
						assert (level2fileData[loop][loop2][fileLoop] == file);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file) == true);
							assert (level3fileData[loop][loop2][loop3][fileLoop] == file);
						}
					}
				}
			}
//			string copylevel1fileNames[3][6];
//			string copylevel1fileData[3][6];

//			string copylevel2fileNames[3][3][6];
//			string copylevel2fileData[3][3][6];

//			string copylevel3fileNames[3][3][3][6];
//			string copylevel3fileData[3][3][3][6];
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Make a hard link to the original file " << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "making hard link to file " << level1fileNames[loop][fileLoop];
					cout << " in - " << level1dirNames[loop] << "/" << "hard"+to_string(fileLoop) << endl;
					assert (directory[(i*3) + j]->makeHardLinkToFile("",level1fileNames[loop][fileLoop],level1dirNames[loop]+"/","hard"+to_string(fileLoop)) == true);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "making hard link to file " << level1dirNames[loop] << "/" << level2fileNames[loop][loop2][fileLoop];
						cout << " in - " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << "hard"+to_string(loop)+to_string(fileLoop) << endl;
						assert (directory[(i*3) + j]->makeHardLinkToFile(level1dirNames[loop]+"/",level2fileNames[loop][loop2][fileLoop],level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/","hard"+to_string(loop)+to_string(fileLoop)) == true);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "making hard link to file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3fileNames[loop][loop2][loop3][fileLoop];
							cout << " in - " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << "hard"+to_string(loop)+to_string(loop2)+to_string(fileLoop) << endl;
							assert (directory[(i*3) + j]->makeHardLinkToFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3fileNames[loop][loop2][loop3][fileLoop],level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/","hard"+to_string(loop)+to_string(loop2)+to_string(fileLoop)) == true);
						}
					}
				}
			}
			string file2;
			cout << "--------------------------------------------------------------------------------------" << endl;
			cout << "Reading the originals and the links checking they are the same" << endl;
			for (int loop=0;loop<3;loop++){
				for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
					cout << "reading file " << level1fileNames[loop][fileLoop] << endl;
					assert (directory[(i*3) + j]->readFile("",level1fileNames[loop][fileLoop],file) == true);
					assert (level1fileData[loop][fileLoop] == file);
					cout << "reading file " << level1dirNames[loop] << "/" << "hard"+to_string(fileLoop) << endl;
					assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/","hard"+to_string(fileLoop),file2) == true);
					assert (level1fileData[loop][fileLoop] == file2);
				}
				for (int loop2=0;loop2<3;loop2++){
					for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
						cout << "reading file " << level1dirNames[loop] << "/" << level2fileNames[loop][loop2][fileLoop] << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/",level2fileNames[loop][loop2][fileLoop],file) == true);
						assert (level2fileData[loop][loop2][fileLoop] == file);
						cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << "hard"+to_string(loop)+to_string(fileLoop) << endl;
						assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/","hard"+to_string(loop)+to_string(fileLoop),file2) == true);
						assert (level2fileData[loop][loop2][fileLoop] == file2);
					}
					for (int loop3=0;loop3<3;loop3++){
						for (int fileLoop=0;fileLoop<6;fileLoop = fileLoop+2) {
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3fileNames[loop][loop2][loop3][fileLoop] << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/",level3fileNames[loop][loop2][loop3][fileLoop],file) == true);
							assert (level3fileData[loop][loop2][loop3][fileLoop] == file);
							cout << "reading file " << level1dirNames[loop] << "/" << level2dirNames[loop][loop2] << "/" << level3dirNames[loop][loop2][loop3] << "/" << "hard"+to_string(loop)+to_string(loop2)+to_string(fileLoop) << endl;
							assert (directory[(i*3) + j]->readFile(level1dirNames[loop]+"/"+level2dirNames[loop][loop2]+"/"+level3dirNames[loop][loop2][loop3]+"/","hard"+to_string(loop)+to_string(loop2)+to_string(fileLoop),file2) == true);
							assert (level3fileData[loop][loop2][loop3][fileLoop] == file2);
						}
					}
				}
			}
		}
	}
}

string UnitTest::createRandomFile() {
	stringstream returnString;
	for (int i=0;i<rand()%1500000;i++) {
		int asciiVal = rand()%256;
		int asciiChar = asciiVal;
		returnString << asciiChar;
	}
	return returnString.str();
}

void UnitTest::runINodeListTest() {
	const int driveSizeLength = 3;
	int driveSize[driveSizeLength] = {
		1024 * 1024 * 128, 	// 128MB
		1024 * 1024 * 64,	// 64MB
		1024 * 1024 * 32	// 32MB
	};

	const int blockSizeLength = 3;
	int blockSize[blockSizeLength] = {
		1024,				// 1KB
		1024 * 2,			// 2KB
		1024 * 4			// 4KB
	};

	HDD* hdd[driveSizeLength * blockSizeLength];
	SuperBlock* superBlock[driveSizeLength * blockSizeLength];
	FreeBlockList* freeBlockList[driveSizeLength * blockSizeLength];
	INodeList* iNodeList[driveSizeLength * blockSizeLength];

	for (int i=0;i<driveSizeLength;i++) {
		for (int j=0;j<blockSizeLength;j++) {
			cout << "****** TESTING INODELIST " << ((i*3) + j) << " ******" << endl;
			cout << "Creating HDD of size " << driveSize[i] << endl;
			hdd[(i*3) + j] = new HDD(driveSize[i]);
			hdd[(i*3) + j]->setDebug(false);
			assert(hdd[(i*3) + j]->capacity() == driveSize[i]);
			cout << "Creating Super Block with HDD" << endl;
			superBlock[(i*3) + j] = new SuperBlock(hdd[(i*3) + j]);
			cout << "Writing Super Block to HDD with blockSize " << blockSize[j] << endl;
			superBlock[(i*3) + j]->write(blockSize[j]);
			cout << "Creating FreeBlockList" << endl;
			freeBlockList[(i*3) + j] = new FreeBlockList(hdd[(i*3) + j], superBlock[(i*3) + j]->getNumberOfBlocks(), superBlock[(i*3) + j]->getBlockSize(), superBlock[(i*3) + j]->getFreeBlockStart());
			freeBlockList[(i*3) + j]->write();
			cout << "Creating INodeList" << endl;
			iNodeList[(i*3) + j] = new INodeList(hdd[(i*3) + j], superBlock[(i*3) + j], freeBlockList[(i*3) + j]);
			iNodeList[(i*3) + j]->write(freeBlockList[(i*3) + j]->getBlock());
			cout << "Testing writeNewINode" << endl;
			int startingNumberOfINodes = iNodeList[(i*3) + j]->getNumberOfFreeINodes();
			int iNodes[20];
			for (int loop=0;loop<20;loop++) {
				iNodes[loop] = iNodeList[(i*3) +j]->writeNewINode(true,1000,freeBlockList[(i*3) + j]->getBlock());
				cout << "Written iNode # " << iNodes[loop] << " - " << loop << endl;
			}
			cout << "starting number of INodes = " << startingNumberOfINodes << " number now = " << iNodeList[(i*3) + j]->getNumberOfFreeINodes() << endl;
			assert (iNodeList[(i*3) + j]->getNumberOfFreeINodes() == startingNumberOfINodes - 20);
			cout << "Testing addLinkToINode" << endl;
			for (int loop=0;loop<20;loop=loop+2) {
				iNodeList[(i*3) + j]->addLinkToINode(iNodes[loop]);
				cout << "Added link to iNode # " << iNodes[loop] << " - " << loop << endl;
			}
			cout << "Testing removeLinkFromINode" << endl;
			for (int loop=0;loop<20;loop++) {
				iNodeList[(i*3) + j]->removeLinkFromINode(iNodes[loop]);
				cout << "Removed link from iNode # " << iNodes[loop] << " - " << loop << endl;
			}
			cout << "starting number of INodes = " << startingNumberOfINodes << " number now = " << iNodeList[(i*3) + j]->getNumberOfFreeINodes() << endl;
			assert (iNodeList[(i*3) + j]->getNumberOfFreeINodes() == startingNumberOfINodes - 10);
			cout << "Testing addBlockToINode, getNumberOfBlocks and getBlockNumber" << endl;
			int newBlock[20];
			for (int loop=0;loop<20;loop=loop+2) {
				newBlock[loop] = freeBlockList[(i*3) +j]->getBlock();
				iNodeList[(i*3) + j]->addBlockToINode(iNodes[loop],newBlock[loop]);
				cout << "Added Block " << newBlock[loop] << " to iNode # " << iNodes[loop] << " - " << loop << endl;
			}
			for (int loop=0;loop<20;loop=loop+2) {
				cout << "iNode # " << iNodes[loop] << " has " << iNodeList[(i*3) + j]->getNumberOfBlocks(iNodes[loop]) << " blocks" << endl;
				assert(iNodeList[(i*3) + j]->getNumberOfBlocks(iNodes[loop]) == 2);
			}
			cout << "Blocks are 0 indexed" << endl;
			for (int loop=0;loop<20;loop=loop+2) {
				cout << "iNode # " << iNodes[loop] << " block number 1 = " << iNodeList[(i*3) + j]->getBlockNumber(iNodes[loop],1) << " = " << newBlock[loop] << endl;
				assert(iNodeList[(i*3) + j]->getBlockNumber(iNodes[loop],1) == newBlock[loop]);
			}
			cout << "Testing reference block creation" << endl;
			int xtraBlocks[20][20];
			int startNumberBlocks[20];
			for (int loop=0;loop<20;loop=loop+2) {
				cout << "Adding 20 blocks to iNode " << iNodes[loop] << endl;
				startNumberBlocks[loop] = iNodeList[(i*3) +j]->getNumberOfBlocks(iNodes[loop]);
				for (int loop2=0;loop2<20;loop2++) {
					xtraBlocks[loop][loop2] = freeBlockList[(i*3) + j]->getBlock();
					iNodeList[(i*3) + j]->addBlockToINode(iNodes[loop],xtraBlocks[loop][loop2]);
				}
				assert(iNodeList[(i*3) +j]->getNumberOfBlocks(iNodes[loop]) == startNumberBlocks[loop]+20);
			}
			for (int loop=0;loop<20;loop=loop+2) {
				cout << "iNode # " << iNodes[loop] << endl;
				for (int loop2=0;loop2<20;loop2++) {
					cout << "block number " << startNumberBlocks[loop] + loop2 << " = " << iNodeList[(i*3) + j]->getBlockNumber(iNodes[loop],startNumberBlocks[loop] + loop2) << " = " << xtraBlocks[loop][loop2] << endl;
					assert(iNodeList[(i*3) + j]->getBlockNumber(iNodes[loop],startNumberBlocks[loop] + loop2)  == xtraBlocks[loop][loop2]);
				}
			}
			cout << "Testing removeBlockFromINode" << endl;
			for (int loop=0;loop<20;loop=loop+2) {
				cout << "removing 22 blocks from iNode " << iNodes[loop] << " - this should throw an error to stop me removing last block " << endl;
				cout << iNodeList[(i*3) +j]->getNumberOfBlocks(iNodes[loop]) << endl;
				startNumberBlocks[loop] = iNodeList[(i*3) +j]->getNumberOfBlocks(iNodes[loop]);
				int loop2 = 22;
				int numberOfFreeBlocks = freeBlockList[(i*3) +j]->getNumberOfFreeBlocks();
				try {
					while (loop2>0) {
						iNodeList[(i*3) +j]->removeBlockFromINode(iNodes[loop]);
						loop2--;
					}
				} catch (const runtime_error e) {
					cout << "iNodeList has correctly thrown an error " << e.what() << endl;
				}
				cout << "Checking iNode has only 1 block" << endl;
				assert (loop2 == 1);
				assert (iNodeList[(i*3) +j]->getNumberOfBlocks(iNodes[loop]) == 1);
				cout << "Checking all the blocks including the reference block has been freed" << endl;
				cout << "number of free blocks to start " << numberOfFreeBlocks << " ,number now " <<freeBlockList[(i*3) +j]->getNumberOfFreeBlocks() << endl;
				assert (freeBlockList[(i*3) +j]->getNumberOfFreeBlocks() == numberOfFreeBlocks + 22);
			}
			cout << "Testing that the reference block and all the blocks in it are freed when an INode has all its links removed" << endl;
			int freeBlocks[20];
			for (int loop=0;loop<20;loop=loop+2) {
				cout << "Adding 22 blocks to iNode " << iNodes[loop] << endl;
				for (int loop2=0;loop2<20;loop2++) {
					xtraBlocks[loop][loop2] = freeBlockList[(i*3) + j]->getBlock();
					iNodeList[(i*3) + j]->addBlockToINode(iNodes[loop],xtraBlocks[loop][loop2]);
				}
				freeBlocks[loop] = freeBlockList[(i*3) + j]->getNumberOfFreeBlocks();
				startNumberBlocks[loop] = iNodeList[(i*3) + j]->getNumberOfBlocks(iNodes[loop]);
				cout << "Remove all links from INode # " << iNodes[loop] << endl;
				iNodeList[(i*3) +j]->removeLinkFromINode(iNodes[loop]);
				assert(freeBlockList[(i*3) + j]->getNumberOfFreeBlocks() == startNumberBlocks[loop] + freeBlocks[loop] + 1);
			}
			cout << "Testing INodeList will stop me from creating too many INodes" << endl;
			int numberOfNodesLeft = iNodeList[(i*3) + j]->getNumberOfFreeINodes();
			int loop = 0;
			try {
				while (loop<numberOfNodesLeft+1) {
					iNodeList[(i*3) + j]->writeNewINode(true,1000,freeBlockList[(i*3) + j]->getBlock());
					loop++;
				}
			} catch (const runtime_error e) {
				cout << "iNodeList has correctly thrown an error " << e.what() << endl;
			}
			assert(loop == numberOfNodesLeft-1);

			// destroy variables
			delete hdd[(i*3) + j];
			delete superBlock[(i*3) + j];
			delete freeBlockList[(i*3) + j];
			delete iNodeList[(i*3) + j];
		}
	}
	cout << "SUCCESS - INodeList !!!!" << endl;
}

void UnitTest::runFreeBlockListTest() {
	const int driveSizeLength = 3;
	int driveSize[driveSizeLength] = {
		1024 * 1024 * 128, 	// 128MB
		1024 * 1024 * 64,	// 64MB
		1024 * 1024 * 32	// 32MB
	};

	const int blockSizeLength = 3;
	int blockSize[blockSizeLength] = {
		1024,				// 1KB
		1024 * 2,			// 2KB
		1024 * 4			// 4KB
	};

	HDD* hdd[driveSizeLength * blockSizeLength];
	SuperBlock* superBlock[driveSizeLength * blockSizeLength];
	FreeBlockList* freeBlockList[driveSizeLength * blockSizeLength];

	for (int i=0;i<driveSizeLength;i++) {
		for (int j=0;j<blockSizeLength;j++) {
			cout << "****** TESTING FREEBLOCKLIST " << ((i*3) + j) << " *****" << endl;
			cout << "Creating HDD of size " << driveSize[i] << endl;
			hdd[(i*3) + j] = new HDD(driveSize[i]);
			hdd[(i*3) + j]->setDebug(false);
			assert(hdd[(i*3) + j]->capacity() == driveSize[i]);
			cout << "Creating Super Block with HDD" << endl;
			superBlock[(i*3) + j] = new SuperBlock(hdd[(i*3) + j]);
			cout << "Writing Super Block to HDD with blockSize " << blockSize[j] << endl;
			superBlock[(i*3) + j]->write(blockSize[j]);
			cout << "Creating FreeBlockList" << endl;
			freeBlockList[(i*3) + j] = new FreeBlockList(hdd[(i*3) + j], superBlock[(i*3) + j]->getNumberOfBlocks(), superBlock[(i*3) + j]->getBlockSize(), superBlock[(i*3) + j]->getFreeBlockStart());
			freeBlockList[(i*3) + j]->write();
			cout << "Using 20 blocks" << endl;
			// check the first free block pointer is 0
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() ) == 0 );
			// check the free block counter is equal to max number of nodes
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() + 4) == superBlock[(i*3) + j]->getNumberOfBlocks() );
			for (int loop=0;loop<20;loop++) {
				freeBlockList[(i*3) + j]->getBlock();
			}
			// check the first free block pointer is 20
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() ) == 20 );
			// check the free block counter is equal to max number of nodes - 20
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() + 4) == superBlock[(i*3) + j]->getNumberOfBlocks() - 20);

			cout << "Freeing 20 blocks" << endl;
			for (int loop=19;loop>-1;loop--) {
				freeBlockList[(i*3) + j]->freeBlock(loop);
			}
			// check the first free block pointer is 0
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() ) == 0 );
			// check the free block counter is equal to max number of nodes
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() + 4) == superBlock[(i*3) + j]->getNumberOfBlocks() );

			// use all the blocks and 1 extra and force an error
			cout << "Use every block +1 check for error " << superBlock[(i*3) + j]->getNumberOfBlocks() << endl;
			int loop = superBlock[(i*3) + j]->getNumberOfBlocks()+1;
			try {
			   while (loop>0) {
				   freeBlockList[(i*3) + j]->getBlock();
				   loop--;
			   }
			} catch (const runtime_error e) {
			    cout << "FreeBlockList has correctly thrown an error " << e.what() << endl;
			}
			assert (loop == 1);
			// check the free block counter is equal to max number of nodes
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() + 4) == 0);
			// free all the blocks
			for (int loop = 0; loop<superBlock[(i*3) + j]->getNumberOfBlocks();loop++) {
				freeBlockList[(i*3) + j]->freeBlock(loop);
			}
			// check the first free block pointer is 0
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() ) == 0 );
			// check the free block counter is equal to max number of nodes
			assert (readInt(hdd[(i*3) + j], superBlock[(i*3) + j]->getFreeBlockStart() + 4) == superBlock[(i*3) + j]->getNumberOfBlocks() );

			// use 50 blocks then free 30 of them at random then check the blocks are still marked as used

			// destroy variables
			delete hdd[(i*3) + j];
			delete superBlock[(i*3) + j];
			delete freeBlockList[(i*3) + j];
		}
	}
	cout << "SUCCESS - FreeBlockList !!!!" << endl;
}

void UnitTest::runSuperBlockTest() {
	const int driveSizeLength = 5;
	int driveSize[driveSizeLength] = {
		1024 * 1024 * 512, 	// 512MB
		1024 * 1024 * 256,	// 256MB
		1024 * 1024 * 128,	// 128MB
		1024 * 1024 * 64,	// 64MB
		1024 * 1024 * 32	// 32MB
	};

	const int blockSizeLength = 4;
	int blockSize[blockSizeLength] = {
		1024,				// 1KB
		1024 * 2,			// 2KB
		1024 * 4,			// 4KB
		1024 * 8			// 8KB
	};

	HDD* hdd[driveSizeLength * blockSizeLength];
	SuperBlock* superBlock[driveSizeLength * blockSizeLength];
	SuperBlock* superBlock2[driveSizeLength * blockSizeLength];

	int numberOfBlocks;
	// 1 bit for every block (rounded up) + the free block list header
	int freeBlockListSizeBytes;
	// calculate the iNodeList size so it pads out to the end of the nearest block
	int iNodeListBytes;
	int systemBlocks;
	int finalINodeListBytes;
	int numberOfINodes;

	for (int i=0;i<driveSizeLength;i++) {
		for (int j=0;j<blockSizeLength;j++) {
			numberOfBlocks = driveSize[i] / blockSize[j];
			freeBlockListSizeBytes = ((numberOfBlocks + 7) /8) + freeBlockHeaderSize;
			iNodeListBytes = ((numberOfBlocks / 8) * iNodeSize) + iNodeHeaderSize + iNodeFooterSize;
			systemBlocks = ((superBlockSize + iNodeListBytes + freeBlockListSizeBytes) + blockSize[j] - 1) / blockSize[j];
			finalINodeListBytes = (systemBlocks * blockSize[j]) - superBlockSize - freeBlockListSizeBytes;
			numberOfINodes = (finalINodeListBytes - iNodeHeaderSize - iNodeFooterSize) / iNodeSize;

			cout << "***** TESTING SUPERBLOCK " << ((i*4) + j) << " *****" << endl;
			cout << "Creating HDD of size " << driveSize[i] << endl;
			hdd[(i*4) + j] = new HDD(driveSize[i]);
			hdd[(i*4) + j]->setDebug(false);
			assert(hdd[(i*4) + j]->capacity() == driveSize[i]);
			cout << "Creating Super Block with HDD" << endl;
			superBlock[(i*4) + j] = new SuperBlock(hdd[(i*4) + j]);
			cout << "Writing Super Block to HDD with blockSize " << blockSize[j] << endl;
			superBlock[(i*4) + j]->write(blockSize[j]);
			hdd[(i*4) + j]->reset();
			cout << "** Testing data written to HDD ..." << endl;
			cout << "number of blocks" << endl;
			assert(readInt(hdd[(i*4) + j]) == driveSize[i] / blockSize[j]);
			cout << "block size " << endl;
			assert(readInt(hdd[(i*4) + j]) == blockSize[j]);
			cout << "freeBockList size in bytes" << endl;
			assert(readInt(hdd[(i*4) + j]) == freeBlockListSizeBytes);
			cout << "number of System blocks" << endl;
			assert(readInt(hdd[(i*4) + j]) == systemBlocks);
			cout << "INodeList size in bytes" << endl;
			assert(readInt(hdd[(i*4) + j]) == finalINodeListBytes);
			cout << "number of INodes" << endl;
			assert(readInt(hdd[(i*4) + j]) == ((finalINodeListBytes - iNodeHeaderSize - iNodeFooterSize) / iNodeSize));
			cout << "** Testing getters ..." << endl;
			testSuperBlockGetters(superBlock[(i*4) + j], numberOfBlocks, systemBlocks, blockSize[j], freeBlockListSizeBytes, iNodeListBytes, finalINodeListBytes, numberOfINodes);
			cout << "Creating new Super Block with HDD" << endl;
			superBlock2[(i*4) + j] = new SuperBlock(hdd[(i*4) + j]);
			cout << "Reading Super Block from HDD" << endl;
			superBlock2[(i*4) + j]->read();
			cout << "** Testing getters ..." << endl;
			testSuperBlockGetters(superBlock2[(i*4) + j], numberOfBlocks, systemBlocks, blockSize[j], freeBlockListSizeBytes, iNodeListBytes, finalINodeListBytes, numberOfINodes);

			// destroy variables
			delete hdd[(i*4) + j];
			delete superBlock[(i*4) + j];
			delete superBlock2[(i*4) + j];

		}
	}
	cout << "SUCCESS - Super Block !!!!" << endl;
}

void UnitTest::testSuperBlockGetters(SuperBlock* superBlock, int numberOfBlocks, int systemBlocks, int blockSize, int freeBlockListSizeBytes, int iNodeListBytes, int finalINodeListBytes, int numberOfINodes) {
	cout << "getNumberOfBlocks" << endl;
	assert (superBlock->getNumberOfBlocks() == (numberOfBlocks - systemBlocks));
	assert (superBlock->getNumberOfBlocks() == (numberOfBlocks - (((superBlockSize + freeBlockListSizeBytes + iNodeListBytes) + blockSize - 1)/ blockSize)));
	cout << "getBlockSize" << endl;
	assert (superBlock->getBlockSize() == blockSize);
	cout << "getFreeBlockStart" << endl;
	assert (superBlock->getFreeBlockStart() == superBlockSize);
	cout << "getINodeStart" << endl;
	assert (superBlock->getINodeStart() == (superBlockSize + freeBlockListSizeBytes));
	cout << "getNumberOfINodes" << endl;
	assert (superBlock->getNumberOfINodes() == numberOfINodes);
	cout << "getFirstBlockStart" << endl;
	assert (superBlock->getFirstBlockStart() == (superBlockSize + freeBlockListSizeBytes + finalINodeListBytes));
	assert (superBlock->getFirstBlockStart() == (systemBlocks * blockSize));
}

int UnitTest::readInt(HDD* hdd) {
	charToInt returnInt;
	for (int i=0;i<4;i++) {
		returnInt.c[i] = hdd->read();
	}
	return returnInt.i;
}

int UnitTest::readInt(HDD* hdd, int index) {
	hdd->seek(index);
	return readInt(hdd);
}
