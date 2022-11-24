// ImageGen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../../../version/PNVersion.h"

#if defined(_WIN32)
#  define PATH_DELIMITER  '\\'
#else
#  define PATH_DELIMITER  '/'
#endif

std::vector<std::string> splitPath(const std::string& path);
std::string joinPath(const std::vector<std::string> dirNameList, int count);
void writeImageInlFile(const std::string &inFilePath, const std::string &outFilePath);
void printUsage();

int main(int argc, char* argv[]) {
	std::string command = "--bin2inl";
	if (argc < 3 || argv[1] != command) {
		printUsage();
		return 0;
	}

	std::string inFilePath = argv[2];
	std::string outFilePath = (argc == 4) ? argv[3] : "";
	if (inFilePath.empty() /*== true*/ || inFilePath == outFilePath) {
		printUsage();
		return 0;
	}

	if (outFilePath.empty() /*== true*/) {
		std::vector<std::string> dirNameList = splitPath(inFilePath);

		std::string fullFileName = dirNameList[dirNameList.size() - 1];
		int index = (int) fullFileName.find_last_of('.');
		if (index >= 0) {
			fullFileName.erase(index);
		}

		std::string path = joinPath(dirNameList, (int) dirNameList.size() - 1);
		outFilePath = path + PATH_DELIMITER + fullFileName + "Image.inl";
	}

	std::cout << "input-file-path:  " << inFilePath << std::endl;
	std::cout << "output-file-path: " << outFilePath << std::endl;
	writeImageInlFile(inFilePath, outFilePath);
	return 0;
}

std::vector<std::string> splitPath(const std::string& path) {
	std::vector<std::string> result;
	std::istringstream iss(path);
	std::string buffer;

	while (std::getline(iss, buffer, PATH_DELIMITER)) {
		result.push_back(buffer);
	}

	if (result.size() == 1) {
		result.insert(result.begin(), ".");
	}
	return result;
}

std::string joinPath(const std::vector<std::string> dirNameList, int count) {
	std::ostringstream result;

	for (const auto& dirName : dirNameList) {
		if (--count > 0) {
			result << dirName << PATH_DELIMITER;
		}
		else {
			result << dirName;
			break;
		}
	}

	return result.str();
}

void writeImageInlFile(const std::string& inFilePath, const std::string& outFilePath) {
	// .exe 파일을 읽습니다.
	std::ifstream inFile(inFilePath, std::ios::binary);
	std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(inFile), {});

	// 타겟 파일명에서 변수명을 뽑아냅니다.
	std::vector<std::string> pathPieces = splitPath(outFilePath);
	std::string variableName = pathPieces[pathPieces.size() - 1];
	int index = (int) variableName.find_last_of('.');
	variableName.erase(index);

	// 소스 파일을 바이너리 형식으로 변환하여 .ini 파일에 저장합니다.
	std::ofstream tarFile(outFilePath);
	std::ostringstream inlStream;

	inlStream << "#pragma once" << std::endl;
	inlStream << std::endl;
	inlStream << "const static unsigned char g_" << variableName << "[] = {" << std::endl;

	for (int i = 0; i < (int) bytes.size(); i++) {
		inlStream << (int)bytes[i];

		if (i != (int) bytes.size() - 1)
			inlStream << ',';

		// 50개를 쓰고 줄바꿈합니다.
		if (i % 50 == 0) {
			inlStream << std::endl;
			tarFile << inlStream.str();
			inlStream.str(""); inlStream.clear();
		}
	}

	tarFile << std::endl << "};";
	tarFile.close();
}

void printUsage() {
	std::cout << ".exe to .inl Converter. - v" << Proud::g_versionText << std::endl;
	std::cout << "(c) Nettetion Inc. All rights reserved." << std::endl << std::endl;
	std::cout << "Usage:" << std::endl;
	std::cout << "    ImageGen.exe --bin2inl <input-file-path> <output-file-path>" << std::endl;
}
