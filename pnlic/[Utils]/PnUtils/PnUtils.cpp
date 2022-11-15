// PnUtils.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "..\..\..\version\PNVersion.h"

#if defined(_WIN32)
#  define PATH_DELIMITER  '\\'
#else
#  define PATH_DELIMITER  '/'
#endif

std::vector<std::string> splitPath(const std::string& path);
std::string joinPath(const std::vector<std::string> path, int count);
void writeImageInlFile(const std::string &sourceFilePath, const std::string &targetFilePath);
void printUsage();

int main(int argc, char* argv[]) {
	std::cout << "Hello World!\n";

	std::string command = "--bin2inl";
	if (argc < 3 || argv[1] != command) {
		printUsage();
		return 0;
	}

	std::string sourceFilePath = argv[2];
	std::string targetFilePath = (argc == 4) ? argv[3] : "";
	if (sourceFilePath == targetFilePath || sourceFilePath.empty() /*== true*/) {
		printUsage();
		return 0;
	}

	if (targetFilePath.empty() /*== true*/) {
		std::vector<std::string> pathPieces = splitPath(sourceFilePath);

		std::string path = joinPath(pathPieces, (int) pathPieces.size() - 1);

		std::string fullFileName = pathPieces[pathPieces.size() - 1];
		int index = (int) fullFileName.find_last_of('.');
		if (index >= 0) {
			fullFileName.erase(index);
		}

		targetFilePath = path + "\\" + fullFileName + "Image.inl";
	}

	std::cout << "src-file-path: " << sourceFilePath << std::endl;//222-삭제:
	std::cout << "tar-file-path: " << targetFilePath << std::endl;//222-삭제:
	writeImageInlFile(sourceFilePath, targetFilePath);
	return 0;
}

std::vector<std::string> splitPath(const std::string& path) {
	std::vector<std::string> result;

	std::istringstream iss(path);
	std::string buffer;

	while (std::getline(iss, buffer, PATH_DELIMITER)) {
		result.push_back(buffer);
	}

	return result;
}

std::string joinPath(const std::vector<std::string> path, int count) {
	std::ostringstream result;

	for (const auto& piece : path) {
		if (--count > 0) {
			result << piece << PATH_DELIMITER;
		}
		else {
			result << piece;
			break;
		}
	}

	return result.str();
}

void writeImageInlFile(const std::string& sourceFilePath, const std::string& targetFilePath) {
	// .exe 파일을 읽습니다.
	std::ifstream srcFile(sourceFilePath, std::ios::binary);
	std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(srcFile), {});

	// 타겟 파일명에서 변수명을 뽑아냅니다.
	std::vector<std::string> pathPieces = splitPath(targetFilePath);
	std::string variableName = pathPieces[pathPieces.size() - 1];
	int index = (int) variableName.find_last_of('.');
	variableName.erase(index);

	// 소스 파일을 .inl 형식으로 변환하여 타겟 파일에 저장합니다.
	std::ofstream tarFile(targetFilePath);
	std::ostringstream inlStream;

	inlStream << "#pragma once" << std::endl;
	inlStream << std::endl;
	inlStream << "const static unsigned char g_" << variableName << "[] = {" << std::endl;

	for (int i = 0; i < bytes.size(); i++) {
		inlStream << (int)bytes[i];

		if (i != bytes.size() - 1)
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
	std::cout << "    PnUtils.exe --bin2inl <source-file-path> <target-file-path>" << std::endl;
}
