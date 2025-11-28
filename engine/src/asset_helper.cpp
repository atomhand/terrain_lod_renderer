#include "asset_helper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

/* Read a text file into a string*/
std::string Engine::AssetHelper::readFile(const char *filePath)
{
    std::filesystem::path path = std::filesystem::current_path();
	path += "/assets/";
	path += filePath;

	std::string content;
	std::ifstream fileStream(path, std::ios::in);

	if (!fileStream.is_open()) {
		std::cerr << "Could not read file " << path.string() << ". File does not exist." << std::endl;
		return "";
	}
	std::cout << "Reading file " << path.string() << std::endl;

	std::string line = "";
	while (!fileStream.eof()) {
		getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();
	return content;
}