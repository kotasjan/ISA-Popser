/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *  License: MIT (More info in LICENSE.md)
*/

#include "popser.h"

/*
 * Popser.cpp je řídící kód celého programu. Slouží ke zpracování argumentů,
 * výpisu nápovědy, výpisu chybových hlášek, zachytávání řídících signálů,
 * spuštění samotného serveru. Kromě toho také kontroluje existenci Maildiru
 * a autorizačního souboru.
*/

// Statické proměnné třídy Server.

uint16_t Server::port;
int Server::clientID;
int Server::cFlag;
int Server::resetFlag;
char Server::hostname[128];
string Server::right_username;
string Server::right_password;
string Server::maildirPath;

/*
 * Fukce help() slouží k vypsání nápovědy programu.
*/

void help() {
  cout << "NAME" << endl;
  cout << "\tpopser" << endl;
  cout << "\n" << endl;
  cout << "SYNOPSIS" << endl;
  cout << "\t./popser -h" << endl;
  cout << "\t./popser -r" << endl;
  cout << "\t./popser [-a PATH] [-d PATH] [-p PORT] [-c] [-h] [-r]" << endl;
  cout << "\n" << endl;
  cout << "DESCRIPTION" << endl;
  cout << "\t-h\tshow help" << endl;
  cout << "\t-r\treset data in maildir directory" << endl;
  cout << "\t-a\tset path to authentication file with user data" << endl;
  cout << "\t-d\tset path to maildir" << endl;
  cout << "\t-p\tset port number" << endl;
  cout << "\t-c\tuse this if you want to use nonencrypted method of "
          "authentication"
       << endl;
  cout << "\n" << endl;
  cout << "AUTHOR" << endl;
  cout << "\tWritten by Jan Kotas as a student project for ISA." << endl;
  cout << "\n" << endl;
  cout << "COPYRIGHT" << endl;
  cout << "\tCopyright © 2017 MIT (for more info visit LICENSE.md)\n\tThis  is "
          "free software: you are free to change and redistribute it.  There "
          "is NO WARRANTY, to the extent permitted by law."
       << endl;
  cout << "\n" << endl;

  exit(0);
}

/*
 * V případě, že se vyskytne vážná chyba ve vykonávání programu nebo jsou
 * například špatně zadané argumenty apod. je volána funkce error(), která
 * vypíše chybovou hlášku na stderr a program bezpečně ukončí.
*/

void error(string errorMessage) {
  cerr << errorMessage << endl;
  signal_handler(EXIT_FAILURE);
}

/*
 * Fuknce signal_handler() je volána v případě, že byl server předčasně ukončen.
 * V takovém případě je potřeba zajistit, že nejdojde k nekonzistenci dat náhlým
 * ukončením a všechny změny dokončené klienty budou úspěšně dokončeny. Z toho
 * důvodu jsem využil mutex pro případ, že by se klient nacházel ve fázi, kdy
 * přistupuje k datům.
*/

void signal_handler(int signal_value) {
  Thread::LockMutex();
  if (signal_value) exit(signal_value);
  Thread::UnlockMutex();
}

/*
 * loadParams() je řídící funkce pro zpracování vstupních argumentů. Argumenty
 * validuje a předává k dalšímu zpracování.
*/

void loadParams(int argc, char* argv[]) {
  int option;
  int count = 0;
  while ((option = getopt(argc, argv, "a:d:p:chr")) != -1) {
    switch (option) {
      case 'h':
        help();
        exit(0);
        break;
      case 'a':
        count += 2;
        if (string(optarg).at(0) == '-')
          error("ERROR: Option -a request value");
        par.authFile = optarg;
        break;
      case 'c':
        count++;
        par.cFlag = 1;
        break;
      case 'p':
        count += 2;
        par.port = atoi(optarg);
        break;
      case 'd':
        count += 2;
        if (string(optarg).at(0) == '-')
          error("ERROR: Option -d request value");
        par.directoryPath = optarg;
        break;
      case 'r':
        count++;
        par.resetFlag = 1;
        break;
      case '?':
        error(
            "ERROR: Some option is not a valid option.\nRun program with -h if "
            "you want to see help\n");
        break;
      default:
        error("ERROR: Invalid argument \"" + string(optarg) +
              "\"!\nRun program with -h if you want to see help\n");
    }
  }

  if (count != (argc - 1)) error("ERROR: Invalid argument.");

  if (argc == 2 && par.resetFlag == 1)
    par.runMode = 0;
  else
    par.runMode = 1;

  if (par.runMode) {
    if (par.directoryPath.size() == 0 || par.authFile.size() == 0) {
      if (par.directoryPath.size() == 0 && par.authFile.size() == 0)
        error(
            "ERROR: Directory path and authentication file path are "
            "missing\n");
      else if (par.directoryPath.size() == 0)
        error("ERROR: Directory path is missing\n");
      else
        error("ERROR: Authentication file path is missipar.directoryPathng\n");
    }

    if (argc > 9)
      error(
          "ERROR: Invalid count of arguments!\nRun program with -h if you want "
          "to see help.\n");

    load_right_user();
    check_path_to_files();
  }

  if (par.port == 0) error("ERROR: Missing -p (port) argument");

  if (par.resetFlag) reset();

  if (par.runMode == 0) exit(0);
}

/*
 * Funkce reset() slouží k navrácení serveru do původního stavu pře spuštěním.
 * To představuje navrácení všech emailů ze složky "cur" do složky "new".
*/

void reset() {
  string path;

  ifstream pathfile("path.txt");

  if (pathfile.is_open()) {
    if (!getline(pathfile, path)) error("ERROR: cannot read path file\n");

    DIR* dir;
    struct dirent* ent;
    string newDir;
    string curDir;

    char ch = path.back();

    if (ch == '/') {
      newDir = path + "new/";
      curDir = path + "cur/";
    } else {
      newDir = path + "/new/";
      curDir = path + "/cur/";
    }

    if ((dir = opendir((curDir).c_str())) != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] != '.') {
          ifstream infile((curDir + string(ent->d_name)).c_str());
          ofstream outfile((newDir + string(ent->d_name)).c_str());
          string content;

          for (int i = 0; infile.eof() != true; i++) content += infile.get();

          content.erase(content.end() - 1);

          infile.close();

          outfile << content;
          outfile.close();

          remove((curDir + string(ent->d_name)).c_str());
        }
      }

      closedir(dir);
    } else {
      /* could not open directory */
      cerr << "ERROR: Could not open directory" << endl;
    }
  } else {
    error(
        "ERROR: Cannot get path of Maildir from path.txt. You need to run "
        "server once at least");
  }
}

/*
 * Funkce load_right_user() slouží k parsování souboru s přihlašovacími údaji
 * uživatele. Výstupem je poté přihlašovací jméno klienta a jeho heslo.
*/

void load_right_user() {
  string line;

  ifstream auth_file(par.authFile);

  if (auth_file.is_open()) {
    if (!getline(auth_file, line)) {
      error("ERROR: cannot read authentication file\n");
    }

    line.erase(0, 11);

    right_username = line;

    // cout << "DEBUG: Right username: " << right_username << "\n" << endl;

    if (!getline(auth_file, line)) {
      error("ERROR: cannot read authentication file\n");
    }

    line.erase(0, 11);

    right_password = line;

    // cout << "DEBUG: Right password: " << right_password << "\n" << endl;

    auth_file.close();

  } else {
    error("ERROR: Unable to open authentication file\n");
  }
}

/*
 * Fukce kontrolující cestu k Maildiru. Při úspěchu zapisuje cestu do souboru
 * path.txt.
*/

void check_path_to_files() {
  struct stat sb;

  if ((stat((par.directoryPath).c_str(), &sb) == 0) && S_ISDIR(sb.st_mode)) {
    fstream f;

    f.open("path.txt", std::fstream::out | std::fstream::trunc);

    f << par.directoryPath;

    f.close();

  } else {
    error("ERROR: Unable to find mail directory");
  }
}

/*
 * Řídící funkce celého programu. Vytváří instanci třídy Server a inicializuje
 * statické atributy.
*/

int main(int argc, char* argv[]) {
  signal(SIGINT, signal_handler);

  loadParams(argc, argv);

  Server* server;

  Server::port = par.port;
  Server::clientID = 0;
  Server::right_username = right_username;
  Server::right_password = right_password;
  Server::maildirPath = par.directoryPath;

  server = new Server();

  server->cFlag = par.cFlag;
  server->resetFlag = par.resetFlag;

  if (gethostname(Server::hostname, 128)) {
    cerr << "ERROR: Cannot get hostname - Hostname set as \"unknown\"" << endl;
    strcpy(Server::hostname, "unknown");
  }

  server->AcceptAndDispatch();

  return 0;
}