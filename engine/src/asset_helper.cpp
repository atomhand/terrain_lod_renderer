#include "asset_helper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

/* Read a text file into a string*/
string Engine::AssetHelper::readFile(const char *filePath)
{
    filesystem::path path = filesystem::current_path();
	path += "/assets/";
	path += filePath;

	string content;
	ifstream fileStream(path, ios::in);

	if (!fileStream.is_open()) {
		cerr << "Could not read file " << path.string() << ". File does not exist." << endl;
		return "";
	}
	cout << "Reading file " << path.string() << endl;

	string line = "";
	while (!fileStream.eof()) {
		getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();
	return content;
}