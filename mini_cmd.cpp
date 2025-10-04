#include <iostream>
#include <string>
#include <set>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <direct.h>
#include <windows.h>
#include <io.h>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

//couleur des erreurs
void error(const string &msg) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY);
    cout << msg << endl;
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

//system() ameliore
bool launch(const std::string& program) {
    HINSTANCE result = ShellExecute(NULL, "open", program.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)result > 32); //>32 = success
}

//lecture du fichier sans ouvrir le cmd
vector<string> readScript(const string& filename) {
    vector<string> lines;
    ifstream file(filename);
    if (!file) {
        error("Error: cannot open script " + filename);
        return lines;
    }
    string line;
    while (getline(file, line)) {
        //ignore les lignes vides et les commentaires
        if (!line.empty() && line[0] != '@' && line[0] != ' ') {
            lines.push_back(line);
        }
    }
    file.close();
    return lines;
}

//couleur des infos
void info(const string &msg) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << msg << endl;
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

//fonction qui contient toutes les commandes
bool exe(const string &input, float version) {

    //liste des commandes valides
    static const set<string> valid_commands = {
        "cd", "help", "version", "dir", "echo", "launch", "cls", "open", "credit", "copy", "sysinfo", "move",
        "create", "del", "add", "sub", "mul", "div", "pow", "srq", "exit", "type", "time", "date", "find"
    };

    //commande exit
    if (input.rfind("exit", 0) == 0) return false;

    //commande version
    if (input == "version") {
        ostringstream oss;
        oss << fixed << setprecision(1) << version;
        info("version: " + oss.str());
    }

    //commande cd
    if (input.rfind("cd", 0) == 0) {
        if (input.size() > 3) {
            string path = input.substr(3);

            //operateur
            for (char &c : path) if (c == '/') c = '\\';
            if (_chdir(path.c_str()) == 0) {
                char* cwd = _getcwd(nullptr, 0);
                info(string("path>") + string(cwd));
                free(cwd);
            } else {
                error(string("Error: unknown path>" + path));
            }
        } else {
            // cd sans argument = affiche répertoire courant
            char* cwd = _getcwd(nullptr, 0);
            info(cwd);
            free(cwd);
        }
    }

    //commande dir
    if (input == "dir") {
        struct _finddata_t fileinfo;
        intptr_t handle;
        handle = _findfirst("*", &fileinfo);
        if (handle != -1) {
            info("[DIR]:");
            do {
                info(fileinfo.name);
            } while (_findnext(handle, &fileinfo) == 0);
            _findclose(handle); //ferme la recherche
        }
    }

    //command echo
    if (input.rfind("echo", 0) == 0) {
        string message = input.substr(5);
        info(message);
    }

    //commande pour nettoyer l'ecran
    if (input == "cls") {
        system("cls");
    }

    //commande pour lancer des script
    if (input.rfind("launch", 0) == 0) {
        string program = input.substr(6);
        size_t start = program.find_first_not_of(" ");
        if (start != string::npos) program = program.substr(start);
        if (!program.empty() && program.front() == '"' && program.back() == '"') {
            program = program.substr(1, program.size() - 2);
        }
        //verifie si c'est un fichier script (.bat, .cmd)
        if (program.size() >= 4) {
            string ext = program.substr(program.size() - 4);
            if (ext == ".bat" || ext == ".cmd") {
                //lit et execute le script dans le mini-CMD
                vector<string> scriptLines = readScript(program);
                for (const string& cmd : scriptLines) {
                    cout << "> " << cmd << endl; //affiche la commande (optionnel)
                    if (!exe(cmd, version)) {
                        break; //si "exit" est dans le script
                    }
                }
                return true;
            }
        }
        //sinon utilise shellexecute() comme avant
        if (!launch(program)) {
            error("Error: cannot launch " + program);
        }
    }

    //commande pour ouvrir et lire un fichier
    if (input.rfind("open", 0) == 0) {
        istringstream iss(input);
        string cmd;
        string file;
        iss >> cmd;
        getline(iss, file);

        //trim les espaces au début
        if (!file.empty() && file[0] == ' ') file.erase(0, 1);

        //supprimer guillemets si présents
        if (!file.empty() && file.front() == '"' && file.back() == '"') {
            file = file.substr(1, file.size() - 2);
        }

        if (file.empty()) {
            error("Error: missing file name");
        } else {
            //verifier si le fichier existe avant d'essayer de l'ouvrir
            if (_access(file.c_str(), 0) != 0) {
                error("Error: file not found -> " + file);
            } else if (!launch(file)) {  //utilise ta fonction ShellExecute existante
                error("Error: cannot open " + file);
            } else {
                info("Opened: " + file);
            }
        }
    }

    //commande pour afficher le contenu d'un fichier dans la console
    if (input.rfind("type", 0) == 0) {
        istringstream iss(input);
        string cmd, file;
        iss >> cmd;
        getline(iss, file);

        if (!file.empty() && file[0] == ' ') file.erase(0, 1); //suprime les espaces
        if (!file.empty() && file.front() == '"' && file.back() == '"') {
            file = file.substr(1, file.size() - 2);
        }
        if (file.empty()) {
            error("Error: file empty");
        } else {
            ifstream in(file);
            if (!in) {
                error("Error: cannot open " + file);
            } else {
                string line;
                while (getline(in, line)) {
                    info(line);
                }
                in.close();
            }
        }
    }

    //copie un fichier vers une destination
    if (input.rfind("copy", 0) == 0) {
        istringstream iss(input);
        string cmd, source, dest;
        iss >> cmd >> source >> dest;

        if (source.empty() || dest.empty()) {
            error("Error: missing arguments for copy");
        } else if (!CopyFile(source.c_str(), dest.c_str(), FALSE)) {
            DWORD errCode = GetLastError();
            error("Error: cannot copy " + source + " (code: " + to_string(errCode) + ")");
        } else {
            info("Copied: " + source + " -> " + dest);
        }
    }

    //commande pour deplacer un fichier
    if (input.rfind("move", 0) == 0) {
        istringstream iss(input);
        string cmd, src, dest;
        iss >> cmd >> src >> dest; //extraction correcte

        //suppression des guillemets si presents
        if (!src.empty() && src.front() == '"' && src.back() == '"') {
            src = src.substr(1, src.size() - 2);
        }
        if (!dest.empty() && dest.front() == '"' && dest.back() == '"') {
            dest = dest.substr(1, dest.size() - 2);
        }

        if (src.empty() || dest.empty()) {
            error("Error: usage -> move <source> <destination>");
        } else {
            //verifie si le fichier source existe
            ifstream check(src);
            if (!check) {
                error("Error: source file not found");
            } else {
                check.close();

                //deplacement
                if (rename(src.c_str(), dest.c_str()) == 0) {
                    info("Moved successfully: " + src + " -> " + dest);
                } else {
                    error("Error: cannot move " + src);
                }
            }
        }
    }

    //crée un nouveau fichier
    if (input.rfind("create", 0) == 0) {
        istringstream iss(input);
        string cmd, folder;
        iss >> cmd >> folder;

        if (folder.empty()) {
            error("Error: missing folder name");
        } else if (_mkdir(folder.c_str()) != 0) {
            error("Error: cannot create " + folder);
        } else {
            info("Created: " + folder);
        }
    }

    //commande qui cherche un mot dans un fichier
    if (input.rfind("find", 0) == 0) {
        istringstream iss(input);
        string cmd, word, file;
        iss >> cmd >> word;
        getline(iss, file);

        //trim espace au début
        if (!file.empty() && file[0] == ' ') file.erase(0, 1);
        //supprimer guillemets si présents
        if (!word.empty() && word.front() == '"' && word.back() == '"') word = word.substr(1, word.size() - 2);
        if (!file.empty() && file.front() == '"' && file.back() == '"') file = file.substr(1, file.size() - 2);
        while (!word.empty() && word.front() == ' ') word.erase(0, 1);
        while (!file.empty() && file.front() == ' ') file.erase(0, 1);

        //operateur de recherche
        if (word.empty() || file.empty()) {
            error("Error: usage -> find <word> <file>");
        } else {
            ifstream in(file);
            if (!in) error("Error: cannot open " + file);
            else {
                string line;
                int lineNumber = 0;
                bool found = false;
                while (getline(in, line)) {
                    lineNumber++;
                    if (line.find(word) != string::npos) {
                        info(to_string(lineNumber) + ": " + line);
                        found = true;
                    }
                }
                if (!found) info("Word not found in file");
                in.close();
            }
        }
    }

    //suprime un fichier
    if (input.rfind("del", 0) == 0) {
        istringstream iss(input);
        string cmd;
        string target;
        iss >> cmd;
        getline(iss, target);

        //trim espaces
        if (!target.empty() && target[0] == ' ') target.erase(0, 1);
        if (!target.empty() && target.front() == '"' && target.back() == '"') {
            target = target.substr(1, target.size() - 2);
        }

        //operateur
        if (target.empty()) {
            error("Error: missing file or folder name");
        } else if (!fs::exists(target)) {
            error("Error: " + target + " does not exist");
        } else {
            try {
                if (fs::is_directory(target)) {
                    fs::remove_all(target); //supprime le dossier + contenu
                } else {
                    fs::remove(target);
                }
                info("Deleted: " + target);
            } catch (const fs::filesystem_error& e) {
                error("Error: " + string(e.what()));
            }
        }
    }

    //comande d'addition
    if (input.rfind("add", 0) == 0) {
        istringstream iss(input);
        string cmd;
        iss >> cmd;
        float val;

        //opérateur de calcul
        if (!(iss >> val)) {
            error("Error: invalid number");
        } else {
            double result = val;
            while (iss >> val) result += val;
            info(to_string(result));
        }
    }

    //commande de soustraction
    if (input.rfind("sub", 0) == 0) {
        istringstream iss(input);
        string cmd;
        iss >> cmd;
        float val;

        //opérateur de calcul
        if (!(iss >> val)) {
            error("Error: invalid number");
        } else {
            double result2 = val;
            while (iss >> val) result2 -= val;
            info(to_string(result2));
        }
    }

    //comande de multiplication
    if (input.rfind("mul", 0) == 0) {
        istringstream iss(input);
        string cmd;
        iss >> cmd;
        float val;

        //operateur de calcul
        if (!(iss >> val)) {
            error("Error: missing numbers");
        } else {
            double result3 = val;
            while (iss >> val) result3 *= val;
            info(to_string(result3));
        }
    }

    //comande de division
    if (input.rfind("div", 0) == 0) {
        istringstream iss(input);
        string cmd;
        float a4, b4;
        iss >> cmd >> a4 >> b4;

        //opérateur de calcul
        if (iss.fail()) error("Error: invalid number");
        else if (b4 == 0) error("Error: division by 0");
        else info(to_string(a4 / b4));
    }

    //comande de puissance
    if (input.rfind("pow", 0) == 0) {
        istringstream iss(input);
        string cmd;
        float a5, b5;
        iss >> cmd >> a5 >> b5;

        //opérateur de calcul
        if (iss.fail()) error("Error: invalid number");
        else info(to_string(pow(a5, b5)));
    }

    //comande de racine carre
    if (input.rfind("srq", 0) == 0) {
        istringstream iss(input);
        string cmd;
        float a6;
        iss >> cmd >> a6;

        //opérateur de calcul
        if (iss.fail()) error("Error: invalid number");
        else info(to_string(sqrt(a6)));
    }

    //credit
    if (input == "credit") {
        info("developeur: Herle");
    }

    //commande qui donne l'heure
    if (input == "time") {
        time_t now_time = time(0);
        tm *ltm = localtime(&now_time);
        info("Current time: " + to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + ":" + to_string(ltm->tm_sec));
    }

    //commande qui donne la date
    if (input == "date") {
        time_t now_time = time(0);
        tm *ltm = localtime(&now_time);
        info("Current date: " + to_string(ltm->tm_mday) + "/" + to_string(1 + ltm->tm_mon) + "/" + to_string(1900 + ltm->tm_year));
    }

    //commande qui donne toutes les infos techniques de l'ordi
    if (input == "sysinfo") {
        //nom de l'ordinateur
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) info("Computer Name: " + string(computerName));

        //nom de l'utilisateur
        char userName[256];
        DWORD userSize = sizeof(userName);
        if (GetUserNameA(userName, &userSize)) info("User Name: " + string(userName));

        //version windows
        OSVERSIONINFOEXA osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        if (GetVersionExA((OSVERSIONINFOA*)&osvi)) {
           info("Windows Version: " + to_string(osvi.dwMajorVersion) + "."
           + to_string(osvi.dwMinorVersion) + " (Build " + to_string(osvi.dwBuildNumber) + ")");
        }

        //memoire physique
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        if (GlobalMemoryStatusEx(&statex)) {
            info("Total Physical Memory: " + to_string(statex.ullTotalPhys / (1024 * 1024)) + " MB");
            info("Available Physical Memory: " + to_string(statex.ullAvailPhys / (1024 * 1024)) + " MB");
        }

        //processeur
        SYSTEM_INFO siSysInfo;
        GetSystemInfo(&siSysInfo);
        info("Number of Cores: " + to_string(siSysInfo.dwNumberOfProcessors));

        //nom du cpu via registre
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char cpuName[256];
            DWORD bufSize = sizeof(cpuName);
            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &bufSize) == ERROR_SUCCESS) {
                info("CPU Name: " + string(cpuName));
            }
            RegCloseKey(hKey);
        }

        //espace disque
        ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
        if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
            info("Disk Total Space (C:): " + to_string(totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024)) + " GB");
            info("Disk Free Space (C:): " + to_string(totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024)) + " GB");
        }
    }

    //commande d'aide
    if (input == "help") {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        cout << "===================================" << endl;
        cout << "list of commands:" << endl;
        cout << "help       = list of commands" << endl;
        cout << "version    = show cmd's version" << endl;
        cout << "cd xxxx    = use defined path" << endl;
        cout << "dir        = show all docs in the path" << endl;
        cout << "echo xxx   = print text" << endl;
        cout << "launch x   = launch program" << endl;
        cout << "open xxxx  = open and write txt folder or other" << endl;
        cout << "type xxxx  = write folder in the cmd" << endl;
        cout << "copy xx xx = copy a folder to another directory" << endl;
        cout << "create xxx = create a folder" << endl;
        cout << "move xx xx = copy a folder in an other directory or rename it" << endl;
        cout << "find xx xx = search a word in the specified folder" << endl;
        cout << "del xxxx   = delete a folder" << endl;
        cout << "add x+x    = add numbers" << endl;
        cout << "sub x-x    = substract numbers" << endl;
        cout << "mul x*x    = multiply numbers" << endl;
        cout << "div x/x    = divide numbers" << endl;
        cout << "pow x^x    = make a power" << endl;
        cout << "srq vx     = make a square root" << endl;
        cout << "cls        = clear the window" << endl;
        cout << "credit     = the developer's name" << endl;
        cout << "time       = show the hour" << endl;
        cout << "date       = show the date" << endl;
        cout << "sysinfo    = show all computer infos" << endl;
        cout << "exit       = quit" << endl;
        cout << "===================================" << endl;
        SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    //si l'input est invalide
    istringstream iss(input);
    string cmd;
    iss >> cmd;
    if (valid_commands.find(cmd) == valid_commands.end()) {
        if (input.empty()) {
            error("Error: please enter a command");
        } else {
            error("Error: invalid command");
        }
        info("enter 'help' to get the command list");
    }
    return true;
}

//boucle principale
int main() {
    float version = 1.0;
    string input;
    cout << "copiright; falseMicrosoftTM" << endl; //pour la blague
    ostringstream oss;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "version:" << version << endl;
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    info("enter 'help' to get the commands list");

    //programme
    while (true) {
        cout << ">";
        getline(cin, input);
        if(exe(input, version) == false) break;
    }
    return 0;
}