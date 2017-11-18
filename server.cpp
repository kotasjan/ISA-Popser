/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *  License: MIT (More info in LICENSE.md)
*/

#include "server.h"

/*
 * Server.cpp je nejdůležitějším souborem celého programu. Obsahuje
 * všechny funkce, které jsou implementací protokolu POP3. Součástí
 * jsou i funkce pro vytvoření spojení mezi klientem a serverem.
*/

// Vektor, který obsahuje instance připojených klientů
vector<Client> Server::clients;

/*
 * Konstruktor třídy Server sloužící k nastavení parametrů serveru.
 * Nejprve se inicializují mutexy (obecný a transakční), ty slouží
 * k zajištění bezpečnosti při práci s globálními (statickými)
 * proměnnými, ale i pro zajištění bezpečného přístupu k datům Maildiru.
 * Součástí vytvoření instance je i vytvoření schránky (socketu), pro
 * navázání spojení na konkrétním portu.
*/

Server::Server() {
  Thread::InitMutex();
  Thread::InitTransaction();

  int yes = 1;

  memset(&serverAddr, 0, sizeof(serverAddr));

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cerr << "ERROR: socket." << endl;
    exit(EXIT_FAILURE);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(Server::port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serverAddr.sin_zero), 8);

  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (bind(server_sock, (struct sockaddr*)&serverAddr,
           sizeof(struct sockaddr)) == -1) {
    cerr << "ERROR: bind" << endl;
    exit(EXIT_FAILURE);
  }

  if ((listen(server_sock, MAX_CLIENTS - 1)) == -1) {
    cerr << "ERROR: listen" << endl;
    exit(EXIT_FAILURE);
  }
}

/*
 * Funkce sloužící k vyhledání instance klienta ve vektoru klientů
 * vector<Client>. Funkce vrací pozici (index) klienta v tomto vektoru.
*/

int Server::FindClientIndex(Client* c) {
  for (size_t i = 0; i < clients.size(); i++) {
    if ((Server::clients[i].getID()) == c->getID()) return (int)i;
  }
  // cerr << "ERROR: Client id not found." << endl;
  return -1;
}

/*
 * Funkce AcceptAndDispatch() vytváří hlavní nekonečnou smyčku
 * programu. Funkce naslouchá, zda se k serveru nepřipojil klient.
 * Pokud se klient připojí, je vytvořena nová instance Client, které
 * se přidělí samostatné vlákno a proces se opakuje.
*/

void Server::AcceptAndDispatch() {
  Client* c;
  Thread* t;

  socklen_t cliSize = sizeof(sockaddr_in);

  while (true) {
    c = new Client(this->clientID);
    t = new Thread();

    this->clientID++;

    c->socket = accept(server_sock, (struct sockaddr*)&clientAddr, &cliSize);

    if (c->socket < 0) {
      cerr << "ERROR: accept";
    } else {
      t->Create((void*)Server::HandleClient, c);
    }
  }
}

/*
 * Funkce HandleClient() obsluhuje klienta. Stará se o vzájemnou komunikaci
 * mezi klientem a serverem založené na pravidlech protokolu POP3.
*/

void* Server::HandleClient(void* args) {
  Client* c = (Client*)args;
  char buffer[BUF_SIZE];
  int len;

  Thread::LockMutex();

  Server::clients.push_back(*c);

  Thread::UnlockMutex();

  if (Server::cFlag) {
    Server::SendData(c, "+OK POP3 server ready\r\n");
  } else {
    time_t seconds;

    seconds = time(NULL);

    stringstream myString;
    myString << "<" << getpid() << "." << seconds << "@" << Server::hostname
             << ">";

    c->SetHash(myString.str(), Server::right_password);

    Server::SendData(
        c, ("+OK POP3 server ready " + myString.str() + "\r\n").c_str());
  }

  while (1) {
    memset(buffer, 0, sizeof(buffer));
    len = recv(c->socket, buffer, sizeof buffer, 0);

    if (len <= 0) {
      Server::CloseClient(c);

      close(c->socket);

      break;
    } else {
      Server::Respond(c, string(buffer));
    }
  }

  return NULL;
}

/*
 * SendData() je prostá data, která pouze abstrahuje odeslání dat klientovi.
*/

void Server::SendData(Client* c, const char* data) {
  if (data != NULL) {
    send(c->socket, data, strlen(data), 0);
  }
}

/*
 * Respond() slouží ke zpracování požadavku zaslaného klientem a k rozhodnutí,
 * zda je zaslaný příkaz validní. V případě, že je příkaz validní, přepošle jej
 * k dalšímu zpracování konkrétní funkci zaměřující se na daný příkaz. Naopak
 * v případě, že požadavek není validní, je klientovi zaslána chybová hláška.
*/

void Server::Respond(Client* c, string request) {
  string pom_request = request;

  transform(pom_request.begin(), pom_request.end(), pom_request.begin(),
            ::toupper);

  if (!c->isInTransactionPhase()) {
    if (strncmp(pom_request.c_str(), "USER", 4) == 0)
      Server::USER(c, request);
    else if (strncmp(pom_request.c_str(), "PASS", 4) == 0)
      Server::PASS(c, request);
    else if (strncmp(pom_request.c_str(), "APOP", 4) == 0)
      Server::APOP(c, request);
    else if (strncmp(pom_request.c_str(), "QUIT", 4) == 0)
      Server::QUIT(c, request);
    else
      Server::SendData(c, "-ERR bad command\r\n");
  } else {
    if (strncmp(pom_request.c_str(), "DELE", 4) == 0)
      Server::DELE(c, request);
    else if (strncmp(pom_request.c_str(), "LIST", 4) == 0)
      Server::LIST(c, request);
    else if (strncmp(pom_request.c_str(), "NOOP", 4) == 0)
      Server::NOOP(c, request);
    else if (strncmp(pom_request.c_str(), "RETR", 4) == 0)
      Server::RETR(c, request);
    else if (strncmp(pom_request.c_str(), "RSET", 4) == 0)
      Server::RSET(c, request);
    else if (strncmp(pom_request.c_str(), "STAT", 4) == 0)
      Server::STAT(c);
    else if (strncmp(pom_request.c_str(), "TOP", 3) == 0)
      Server::TOP(c, request);
    else if (strncmp(pom_request.c_str(), "UIDL", 4) == 0)
      Server::UIDL(c, request);
    else if (strncmp(pom_request.c_str(), "QUIT", 4) == 0)
      Server::QUIT(c, request);
    else
      Server::SendData(c, "-ERR bad command\r\n");
  }
}

/*
 * Po odpojení klienta od serveru se musí instance Client,
 * patřící odpojenému klientovi odstranit z vektoru. K tomu
 * slouží právě funkce CloseClient().
*/

void Server::CloseClient(Client* c) {
  if (c->isInTransactionPhase()) Thread::UnlockTransaction();

  Thread::LockMutex();

  int index = Server::FindClientIndex(c);

  if (index != -1) {
    Server::clients.erase(Server::clients.begin() + index);
  } else {
    // cerr << "ERROR: Cannot find user to erasing him!" << endl;
  }

  Thread::UnlockMutex();
}

/*
 * Update() je funkce, která se volá v případě, že klient plánovaně
 * opustil server a pomocí POP3 příkazu QUIT. Musí být ale splněn
 * předpoklad, že byl v tu dobu v transakční fázi.
*/

void Server::Update(Client* c) {
  Thread::LockMutex();

  for (auto& email : c->emails) {
    if (email.deleteFlag)
      remove((Server::maildirPath + string("cur/") + email.name).c_str());
  }

  Thread::UnlockMutex();
}

/*
 * Funkce AddNewEmails() zkontroluje obsah složky "new" v Maildiru
 * a v případě, že se v ní nacházejí nějaké nové emaily, tak je přesune
 * do složky "cur".
*/

void Server::AddNewEmails(Client* c) {
  DIR* dir;
  struct dirent* ent;
  string newDir;
  string curDir;

  char ch = Server::maildirPath.back();

  if (ch == '/') {
    newDir = Server::maildirPath + "new/";
    curDir = Server::maildirPath + "cur/";
  } else {
    newDir = Server::maildirPath + "/new/";
    curDir = Server::maildirPath + "/cur/";
  }

  if ((dir = opendir((newDir).c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] != '.')

        MoveFile(newDir + string(ent->d_name), curDir + string(ent->d_name));
    }

    closedir(dir);
  }
}

/*
 * Funkce CurrentEmails provádí analýzu složky "cur" a vytváří
 * instance třídy Email, které představují reálné emaily, které
 * se nacházejí v této složce.
*/

void Server::CurrentEmails(Client* c) {
  DIR* dir;
  Email* e;
  struct dirent* ent;
  struct stat sb;
  string curDir;

  char ch = Server::maildirPath.back();

  if (ch == '/')
    curDir = Server::maildirPath + "cur/";
  else
    curDir = Server::maildirPath + "/cur/";

  string content;

  if ((dir = opendir((curDir).c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] != '.') {
        content = "";

        ifstream infile((curDir + string(ent->d_name)).c_str());

        for (int i = 0; infile.eof() != true; i++) content += infile.get();

        content.erase(content.end() - 1);

        e = new Email(c->curEmails + 1);
        e->content = content;
        e->name = ent->d_name;
        e->uniqueName = md5(to_string(e->getID()));

        if (stat((curDir + string(ent->d_name)).c_str(), &sb) == -1) {
          cerr << "ERROR: stat" << endl;
          return;
        } else {
          c->overalSize += sb.st_size;
          e->size = sb.st_size;
        }

        c->curEmails++;
        c->emails.push_back(*e);
      }
    }

    closedir(dir);
  } else {
    cerr << "ERROR: Could not open directory" << endl;
  }
}

/*
 * Tato funkce slouží k přesunu souboru z jedné složky do druhé.
*/

void Server::MoveFile(string sourceFile, string destinationFile) {
  ifstream infile((sourceFile).c_str());
  ofstream outfile((destinationFile).c_str());
  string content;

  for (int i = 0; infile.eof() != true; i++)
    content += infile.get();

  content.erase(content.end() - 1);

  infile.close();

  outfile << content; 
  outfile.close();

  remove(sourceFile.c_str());
}

/*
 * Funkce USER() slouží ke zpracování uživatelského jména získaného
 * z příkazu, který uživatel zaslal serveru. Uživatelské jméno je
 * uloženo do atributu klienta a k jeho ověření dochází až po získání
 * hesla. Tato funkce je přístupná pouze za předppokladu, že je na serveru
 * povoleno nezabezpečené (nešifrované) přihlašování.
*/

void Server::USER(Client* c, string request) {
  if (Server::cFlag) {
    try {
      c->username = request.substr(5, request.length() - 7);
    } catch (const std::out_of_range& oor) {
      SendData(c,
               "-ERR Your USER command has bad format. Use -h switch to show "
               "help\r\n");
      return;
    }

    SendData(c, "+OK Send your password\r\n");
  } else
    SendData(c, "-ERR USER command is not allowed. Use APOP instead\r\n");
}

/*
 * Funkce PASS() slouží ke zpracování klientova hesla. V této funkci se
 * rozhodne, zda jsou přihlašovací údaje validní a zda se je možné uživatelovi
 * poskytnout výlučný přístup do transakční fáze. V případě, že výlučný přístup
 * klient nezíská, musí se, i v případě zadání platných údajů, opětovně
 * autorizovat.
 * Stejně jako funkce USER i tato funkce je zpřístupněna pouze v případě, když
 * je
 * na serveru povoleno nezabezpečené přihlašování.
*/

void Server::PASS(Client* c, string request) {
  if (Server::cFlag && (c->username.length() > 0)) {
    string userpass;
    try {
      userpass = request.substr(5, request.length() - 7);
    } catch (const std::out_of_range& oor) {
      SendData(c,
               "-ERR Your PASS command has bad format. Use -h switch to show "
               "help\r\n");
      return;
    }
    if (userpass == Server::right_password &&
        c->username == Server::right_username) {
      if (!Thread::LockTransaction()) {
        Thread::LockMutex();

        c->SetTransactionPhase(true);
        AddNewEmails(c);
        CurrentEmails(c);
        SendData(
            c, (string("+OK username's maildrop has ") +
                to_string(c->curEmails) + string(" messages (") +
                to_string(c->overalSize) + string(" octets)") + string("\r\n"))
                   .c_str());

        Thread::UnlockMutex();
      } else {
        c->username = "";
        c->SetTransactionPhase(false);
        SendData(c, "-ERR cannot get access to Maildir\r\n");
      }
    } else {
      c->username = "";
      SendData(c, "-ERR bad command\r\n");
    }
  } else {
    c->username = "";
    if (!Server::cFlag)
      SendData(c, "-ERR PASS command is not allowed. Use APOP instead\r\n");
    else
      SendData(c, "-ERR You have to use USER command first\r\n");
  }
}

/*
 * Funkce APOP() slouží k autorizaci uživatele pomocí šifrovaného přihlašování.
 * Šifrování je prováděno za pomoci knihovny md5, která ze vstupního řětězce
 * vygeneruje 32 znaků dlouhý hash kód. Stejně jako u funkce PASS platí, že
 * pokud uživatel nedostane přístup do transakční fáze, není autorizace platná
 * a uživatel ji musí opakovat až do doby, kdy výlučný přístup získá.
*/

void Server::APOP(Client* c, string request) {
  string hash;
  if (!Server::cFlag) {
    try {
      c->username = request.substr(5, request.length() - 40);
      hash = request.substr(request.length() - 34, 32);
    } catch (const std::out_of_range& oor) {
      SendData(c,
               "-ERR Your APOP command has bad format. Use -h switch to show "
               "help\r\n");
      return;
    }

    if ((c->username == Server::right_username) && (hash == c->getHash())) {
      if (!Thread::LockTransaction()) {
        Thread::LockMutex();

        c->SetTransactionPhase(true);
        AddNewEmails(c);
        CurrentEmails(c);
        SendData(
            c, (string("+OK username's maildrop has ") +
                to_string(c->curEmails) + string(" messages (") +
                to_string(c->overalSize) + string(" octets)") + string("\r\n"))
                   .c_str());

        Thread::UnlockMutex();
      } else {
        c->SetTransactionPhase(false);
        SendData(c, "-ERR cannot get access to Maildir\r\n");
      }
    } else
      SendData(c, "-ERR wrong username or password\r\n");
  } else {
    SendData(
        c,
        "-ERR APOP command is not supported - use USER and PASS instead\r\n");
  }
}

/*
 * Funkce DELE() slouží ke smazání konkrétného emailu v adresáři "cur". Email se
 * fyzicky nesmaže okamžitě, ale až po update fázi. Do té doby je pouze označen
 * ke smazání. Podmínkou pro použití této funkce je, že se klient nachází
 * v transakční fázi.
*/

void Server::DELE(Client* c, string request) {
  int emailNumber;
  try {
    emailNumber = stoi(request.substr(5, request.length() - 7));
  } catch (const std::out_of_range& oor) {
    cerr << "ERROR: parsing email number." << endl;
    return;
  }

  if (c->emails[emailNumber - 1].deleteFlag == true)
    SendData(c, (string("-ERR message ") + to_string(emailNumber) +
                 string(" already deleted\r\n"))
                    .c_str());
  else {
    c->emails[emailNumber - 1].deleteFlag = true;

    SendData(c, (string("+OK message ") + to_string(emailNumber) +
                 string(" deleted\r\n"))
                    .c_str());
  }
}

/*
 * Funkce LIST() slouží buď k vypsání seznamu všech emailů ze složky "cur" nebo
 * k vypsání pouze konkrétního emailu. Tímto vypsáním se myslí pouze záznam
 * skládající se z ID emailu a jeho velikosti v Bytech. Podmínkou pro použití
 * této funkce je, že se klient nachází v transakční fázi.
*/

void Server::LIST(Client* c, string request) {
  string number = "";

  try {
    number = request.substr(4, request.length() - 6);
  } catch (const std::out_of_range& oor) {
    cerr << "ERROR: substr()" << endl;
    return;
  }

  int sizeAfterDel = c->overalSize;
  int emailsAfterDel = c->curEmails;

  for (auto& email : c->emails) {
    if (email.deleteFlag) {
      sizeAfterDel -= email.size;
      emailsAfterDel--;
    }
  }

  if (number.length() == 0) {
    SendData(
        c, (string("+OK ") + to_string(emailsAfterDel) + string(" messages (") +
            to_string(sizeAfterDel) + string(" octets)\r\n"))
               .c_str());

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        SendData(c, (to_string(email.getID()) + string(" ") +
                     to_string(email.size) + string("\r\n"))
                        .c_str());
      }
    }
    SendData(c, ".\r\n");

  } else {
    int a;

    try {
      a = stoi(number);
    } catch (const std::invalid_argument& ia) {
      SendData(c, "-ERR bad command\r\n");
      return;
    }

    if (a < 1) {
      SendData(c, (string("-ERR email with number ") + to_string(a) +
                   string(" does not exist\r\n"))
                      .c_str());
      return;
    }

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        if (a == email.getID()) {
          SendData(c, (string("+OK ") + to_string(a) + string(" ") +
                       to_string(email.size) + string("\r\n"))
                          .c_str());
          return;
        }
      }
    }

    SendData(c, (string("-ERR email with number ") + to_string(a) +
                 string(" does not exist\r\n"))
                    .c_str());
  }
}

/*
 * Funkce NOOP je primitivní POP3 funkce, která má pouze otestovat komunikaci
 * se serverem. Odpovědí je vždy pouze prosté "+OK".
*/

void Server::NOOP(Client* c, string request) { SendData(c, "+OK\r\n"); }

/*
 * Funkce QUIT() slouží k uzavření socketu klienta. V případě, že byl klient
 * v transakční fázi, vstoupí se do fáze update, kde se provedou klientem
 * požadované změny. Podmínkou pro použití této funkce je, že se klient nachází
 * v transakční fázi.
*/

void Server::QUIT(Client* c, string request) {
  SendData(c, "+OK Bye\r\n");

  if (c->isInTransactionPhase()) Update(c);

  Server::CloseClient(c);

  close(c->socket);
}

/*
 * Funkce RETR() slouží k odeslání obsahu emailu klientovi. Funkce nejprve
 * zkontroluje, zda nebyl požadovaný email označen ke smazání (zda existuje)
 * a v případě nalezení shody obsah emailu odešle klientovi. Podmínkou pro
 * použití této funkce je, že se klient nachází v transakční fázi.
*/

void Server::RETR(Client* c, string request) {
  string number = "";

  try {
    number = request.substr(4, request.length() - 6);
  } catch (const std::out_of_range& oor) {
    cerr << "ERROR: substr()" << endl;
    return;
  }

  if (number.length() != 0) {
    int a;

    try {
      a = stoi(number);
    } catch (const std::invalid_argument& ia) {
      SendData(c, "-ERR bad command\r\n");
      return;
    }

    if (a < 1) {
      SendData(c, (string("-ERR email with number ") + to_string(a) +
                   string(" does not exist\r\n"))
                      .c_str());
      return;
    }

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        if (a == email.getID()) {
          SendData(c, (string("+OK ") + to_string(email.size) +
                       string(" octets\r\n"))
                          .c_str());
          SendData(c, (email.content + string(".\r\n")).c_str());
          return;
        }
      }
    }

    SendData(c, (string("-ERR email with number ") + to_string(a) +
                 string(" does not exist\r\n"))
                    .c_str());

  } else {
    SendData(c, "-ERR bad command\r\n");
    return;
  }
}
/*
 * Funkce RSET() projde všechny instance třídy Email a ty, které jsou
 * označeny ke smazání, odznačí. Podmínkou pro použití této funkce je,
 * že se klient nachází v transakční fázi.
*/

void Server::RSET(Client* c, string request) {
  if (request.length() == 6) {
    for (auto& email : c->emails) {
      if (email.deleteFlag) email.deleteFlag = false;
    }

    SendData(c, (string("+OK maildrop has ") + to_string(c->curEmails) +
                 string(" messages (") + to_string(c->overalSize) +
                 string(" octets)\r\n"))
                    .c_str());

  } else
    SendData(c, "-ERR bad command\r\n");
}

/*
 * Cílem funkce STAT() je vypsat celkový počet emailů a jejich celkovou
 * velikost. Podmínkou pro použití této funkce je, že se klient nachází
 * v transakční fázi.
*/

void Server::STAT(Client* c) {
  int sizeAfterDel = c->overalSize;
  int emailsAfterDel = c->curEmails;

  for (auto& email : c->emails) {
    if (email.deleteFlag) {
      sizeAfterDel -= email.size;
      emailsAfterDel--;
    }
  }

  SendData(c, (string("+OK ") + to_string(emailsAfterDel) + string(" ") +
               to_string(sizeAfterDel) + string("\r\n"))
                  .c_str());
}

/*
 * Funkce TOP() je bonusovou funkcí tohoto projektu. Jejím cílem je
 * vypsat konkrétní počet řádků z konkrétního emailu. Podmínkou pro
 * použití této funkce je, že se klient nachází v transakční fázi.
*/

void Server::TOP(Client* c, string request) {
  string numbers = "";

  try {
    numbers = request.substr(4, request.length() - 5);
  } catch (const std::out_of_range& oor) {
    cerr << "ERROR: substr()" << endl;
    return;
  }

  size_t n = count(numbers.begin(), numbers.end(), ' ');

  if (n != 1) {
    SendData(c, "-ERR bad command\r\n");
    return;
  }

  stringstream stream(numbers);

  int msgNum, countOfLines;

  stream.exceptions(std::ios::failbit);

  try {
    stream >> msgNum;
    stream >> countOfLines;

  } catch (std::ios::failure e) {
    SendData(c, "-ERR bad command\r\n");
    return;
  }

  if (msgNum < 0) {
    SendData(c, (string("-ERR email ") + to_string(msgNum) +
                 string(" was not found\r\n"))
                    .c_str());
    return;
  }

  if (countOfLines < 0) {
    SendData(c, (string("-ERR number of lines (") + to_string(countOfLines) +
                 string(") have to be a possitive number\r\n"))
                    .c_str());
    return;
  }

  for (auto& email : c->emails) {
    if (!email.deleteFlag) {
      if (msgNum == email.getID()) {
        istringstream f(email.content);
        string line;

        int j;
        string result = "";

        for (j = (countOfLines > 0 ? (countOfLines + 11) : 10);
             getline(f, line) && (j > 0); j--)
          result += line + "\n";

        if (j == 0) {
          SendData(c, "+OK\r\n");
          SendData(c, (result + string(".\r\n")).c_str());
        } else {
          SendData(c, "+OK\r\n");
          SendData(c, (email.content + string(".\r\n")).c_str());
        }
        return;
      }
    }
  }

  SendData(c, "-ERR no such message\r\n");
}

/*
 * Funkce UIDL() může být zadána s parametrem (číslem zprávy) nebo
 * bez parametru. V prvním případě funkce vypíše ID zprávy s unikátním
 * md5 hash názvem. V druhém případě funkce vypíše seznam všech zpráv.
 * Podmínkou pro použití této funkce je, že se klient nachází
 * v transakční fázi.
*/

void Server::UIDL(Client* c, string request) {
  string number = "";

  try {
    number = request.substr(5, request.length() - 6);
  } catch (const std::out_of_range& oor) {
    cerr << "ERROR: substr()" << endl;
    return;
  }

  int emailsAfterDel = c->curEmails;

  for (auto& email : c->emails) {
    if (email.deleteFlag) emailsAfterDel--;
  }

  if (number.length() == 0) {
    SendData(c, (string("+OK ") + to_string(emailsAfterDel) +
                 string(" messages in maildrop") + string("\r\n"))
                    .c_str());

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        SendData(c, (to_string(email.getID()) + string(" ") + email.uniqueName +
                     string("\r\n"))
                        .c_str());
      }
    }
    SendData(c, ".\r\n");

  } else {
    size_t n = count(number.begin(), number.end(), ' ');

    if (n != 0) {
      SendData(c, "-ERR bad command\r\n");
      return;
    }

    stringstream stream(number);

    int msgNum;

    stream.exceptions(std::ios::failbit);

    try {
      stream >> msgNum;

    } catch (std::ios::failure e) {
      SendData(c, "-ERR bad command\r\n");
      return;
    }

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        if (msgNum == email.getID()) {
          SendData(c, (string("+OK ") + to_string(msgNum) + string(" ") +
                       email.uniqueName + string("\r\n"))
                          .c_str());
          return;
        }
      }
    }

    SendData(c, "-ERR no such message\r\n");
  }
}
