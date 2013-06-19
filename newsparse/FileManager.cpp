#include "FileManager.h"
#include "RssParser.h"
#include "md5.h"


string FileManager::FilenameFromSource(string source) {
	return md5(source);
}


vector< pair<string,string> > FileManager::GetRegisteredSources() {
	vector< pair<string,string> > registry;

	fstream freg;
	if (!OpenFile(freg, "registry", ios::in, false)) {
		return registry;
	}

	string filename, source;
	while (freg) {
		getline(freg, filename, '\n');
		getline(freg, source, '\n');

		if (filename.length() && source.length()) {
			registry.push_back(pair<string,string>(filename, source));
		}
	}

	return registry;
}


bool FileManager::AddSource(string source) {
	if (IsSourceRegistered(source)) {
		cout << "Feed is already being tracked.\n";
		return false;
	}

	vector< pair<string,string> > regs = GetRegisteredSources();
	regs.push_back( pair<string,string>(FilenameFromSource(source), source) );

	WriteRegistry(regs);

	return true;
}


bool FileManager::RemoveSource(string source) {
	vector< pair<string,string> > regs = GetRegisteredSources();

	int bestIdx = -1;
	int partialMatches = 0;


	for (int i=0; i<regs.size(); i++) {
		if (regs[i].second == source) {
			partialMatches = -1;
			bestIdx = i;
		} else if (partialMatches >= 0) {
			if (regs[i].second.find(source) != string::npos) {
				partialMatches++;
				bestIdx = i;
			}
		}
	}

	if (bestIdx >= 0) {
		if (partialMatches > 1) {
			cout << partialMatches << " matches found, none has been removed.\n";
			cout << "Be more specific, please.\n";
			return false;
		} else {
			cout << "Removed source:\n\t" << regs[bestIdx].second << "\n";

			remove(regs[bestIdx].first.c_str());	// Delete the cache-file

			regs.erase(regs.begin() + bestIdx);
			WriteRegistry(regs);
		}
	} else {
		cout << "No feed containing '" << source << "' found.\n";
		return false;
	}

	return true;
}


FileManager::FileManager(void) {

}


FileManager::~FileManager(void) {

}


void FileManager::WriteToFile(vector<RssItem> *items, string source) {
	string filename = FilenameFromSource(source);
	RegisterSource(source, filename);

	fstream file;
	if (!OpenFile(file, filename, ios::trunc | ios::out, true)) {
		cout << "Failed to open file: '" << filename <<"'.\n";
		return;
	}

	for (int i=0; i<items->size(); i++) {
		(*items)[i].WriteToFile(file);
	}
}


void FileManager::ReadFromFile(vector<RssItem> *items, string source) {
	if (IsSourceRegistered(source)) {
		string filename = GetRegisteredSourceFile(source);

		if (filename.length()) {
			fstream file;
			if (!OpenFile(file, filename, ios::in, false)) {
				return;
			}

			while (file) {
				RssItem item;
				if (item.ReadFromFile(file)) {
					items->push_back(item);
				}
			}
		}
	}
}





bool FileManager::OpenFile(fstream &file, string filename, int mode, bool create) {
	if (file.is_open()) {
		return true;
	}

	file.open(filename, (std::ios_base::openmode)mode);

	if (!create) {
		return file.is_open();
	}

	if (!file.is_open()) {
		CreateDummyFile(filename);
		return OpenFile(file, filename, (ios_base::openmode)mode, false);
	}

	return true;
}


void FileManager::CreateDummyFile(string filename) {
	ofstream dummy(filename);
}


void FileManager::WriteRegistry(vector< pair<string,string> > regs) {
	fstream freg;
	if (!OpenFile(freg, "registry", ios::out | ios::trunc, true)) {
		cout << "Failed to open file 'registry'!\n";
		return;
	}

	for (int i=0; i<regs.size(); i++) {
		freg << regs[i].first << "\n"
			 << regs[i].second << "\n";
	}
}


void FileManager::RegisterSource(string source, string file) {
	if (!IsSourceRegistered(source)) {
		fstream freg;
		OpenFile(freg, "registry", ios::app, true);

		freg << file << "\n"
			 << source << "\n";
	}
}


bool FileManager::IsSourceRegistered(string source) {
	fstream freg;
	if (!OpenFile(freg, "registry", ios::in, true)) {
		return false;
	}

	string line;
	do {
		getline(freg, line, '\n');	// Ignore the filename
		getline(freg, line, '\n');

		if (line == source) {
			return true;
		}
	} while (freg);

	return false;
}


string FileManager::GetRegisteredSourceFile(string source) {
	fstream freg;
	if (!OpenFile(freg, "registry", ios::in, false)) {
		cout << "Failed to open file 'registry'!\n";
		return "";
	}

	string fsource;
	string filename;
	do {
		getline(freg, filename, '\n');
		getline(freg, fsource, '\n');

		if (fsource == source) {
			return filename;
		}
	} while (freg);

	return "";
}


void FileManager::ResetHandle(fstream &file) {
	file.clear();
	file.seekg(0, file.beg);
}
