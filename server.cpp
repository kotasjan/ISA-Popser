#include "server.h"

// Actually allocate clients
vector<Client> Server::clients;

Server::Server() {
  // Initialize static mutex from Thread
  Thread::InitMutex();
  Thread::InitTransaction();

  // For setsock opt (REUSEADDR)
  int yes = 1;

  memset(&serverAddr, 0, sizeof(serverAddr));

  // Init server_sock and start listen()'ing
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    cerr << "ERROR: socket." << endl;
    exit(EXIT_FAILURE);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(Server::port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serverAddr.sin_zero), 8);

  // Avoid bind error if the socket was not close()'d last time;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (bind(server_sock, (struct sockaddr*)&serverAddr,
           sizeof(struct sockaddr)) == -1){
    cerr << "ERROR: bind" << endl;
    exit(EXIT_FAILURE);
}

  if ((listen(server_sock, MAX_CLIENTS - 1)) == -1){
    cerr << "ERROR: listen" << endl;
    exit(EXIT_FAILURE);
  }
}

/*
  Should be called when vector<Client> clients is locked!
*/
int Server::FindClientIndex(Client* c) {
  for (size_t i = 0; i < clients.size(); i++) {
    if ((Server::clients[i].getID()) == c->getID()) return (int)i;
  }
  //cerr << "ERROR: Client id not found." << endl;
  return -1;
}

/*
  AcceptAndDispatch();
  Main loop:
    Blocks at accept(), until a new connection arrives.
    When it happens, create a new thread to handle the new client.
*/
void Server::AcceptAndDispatch() {
  Client* c;
  Thread* t;

  socklen_t cliSize = sizeof(sockaddr_in);

  while (true) {
    c = new Client(this->clientID);
    t = new Thread();

    this->clientID++;

    // Blocks here;
    c->socket = accept(server_sock, (struct sockaddr*)&clientAddr, &cliSize);

    if (c->socket < 0) {
      cerr << "ERROR: accept";
    } else {
      t->Create((void*)Server::HandleClient, c);
    }
  }
}

// Static
void* Server::HandleClient(void* args) {
  // Pointer to accept()'ed Client
  Client* c = (Client*)args;
  char buffer[BUF_SIZE];
  int len;

  // Add client in Static clients <vector> (Critical section!)
  Thread::LockMutex();

  Server::clients.push_back(*c);

  Thread::UnlockMutex();

  time_t seconds;

  seconds = time(NULL);

  stringstream myString;
  myString << "<" << getpid() << "." << seconds << "@" << Server::hostname
           << ">";

  c->SetHash(myString.str(), Server::right_password);

  if (Server::cFlag)
    Server::SendData(c, "+OK POP3 server ready\r\n");
  else
    Server::SendData(
        c, ("+OK POP3 server ready " + myString.str() + "\r\n").c_str());

  while (1) {
    memset(buffer, 0, sizeof(buffer));
    len = recv(c->socket, buffer, sizeof buffer, 0);

    // Client disconnected?
    if (len <= 0) {
      Server::CloseClient(c);

      // cout << "Client " << c->getID() << " diconnected" << endl;
      close(c->socket);

      break;
    } else {
      Server::Respond(c, string(buffer));
    }
  }

  // End thread
  return NULL;
}

void Server::SendData(Client* c, const char* data) {
  if (data != NULL) {
    send(c->socket, data, strlen(data), 0);
    // cout << "Reply stream: " << data;
  }
}

void Server::Respond(Client* c, string request) {

  string pom_request = request;

  transform(pom_request.begin(), pom_request.end(), pom_request.begin(), ::toupper);

  if (!c->isInTransactionPhase()) {
    // prikaz USER slouzi na kontrolu prihlasovaciho jmena
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

void Server::CloseClient(Client* c) {
  if (c->isInTransactionPhase()) Thread::UnlockTransaction();

  // Remove client in Static clients <vector> (Critical section!)
  Thread::LockMutex();

  int index = Server::FindClientIndex(c);

  if (index != -1) {
    //cout << "Erasing user in position " << index << " whose id is: " << Server::clients[index].getID() << endl;
    Server::clients.erase(Server::clients.begin() + index);
  } else {
    //cerr << "ERROR: Cannot find user to erasing him!" << endl;
  }

  Thread::UnlockMutex();
}

void Server::Update(Client* c) {

  Thread::LockMutex();

  for (auto& email : c->emails) {
    if (email.deleteFlag) remove((Server::maildirPath + string("cur/") + email.name).c_str());
  }

  Thread::UnlockMutex();
}

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
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] != '.')

        MoveFile(newDir + string(ent->d_name), curDir + string(ent->d_name));
    }

    closedir(dir);
  }
}

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
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] != '.') {
        content = "";

        ifstream infile((curDir + string(ent->d_name)).c_str());

        for (int i = 0; infile.eof() != true; i++)  // get content of infile
          content += infile.get();

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

        // e->print();

        c->curEmails++;
        c->emails.push_back(*e);
      }
    }

    closedir(dir);
  } else {
    /* could not open directory */
    cerr << "ERROR: Could not open directory" << endl;
  }
}

void Server::MoveFile(string sourceFile, string destinationFile) {
  ifstream infile((sourceFile).c_str());
  ofstream outfile((destinationFile).c_str());
  string content;

  for (int i = 0; infile.eof() != true; i++)  // get content of infile
    content += infile.get();

  content.erase(content.end() - 1);  // erase last character

  infile.close();

  outfile << content;  // output
  outfile.close();

  remove(sourceFile.c_str());
}

void Server::USER(Client* c, string request) {
  if (Server::cFlag) {
    // pomoci substr() vyberu prihlasovaci jmeno
    try {
      c->username = request.substr(5, request.length() - 7);
    } catch (const std::out_of_range& oor) {
      SendData(c,
               "-ERR Your USER command has bad format. Use -h switch to show "
               "help\r\n");
      return;
    }
    // cout << "DEBUG: Username: " << username << endl;

    // cout << "DEBUG: Userlength: " << username.length() << endl;

    SendData(c, "+OK Send your password\r\n");
  } else
    SendData(c, "-ERR USER command is not allowed. Use APOP instead\r\n");
}

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

    // cout << "Username: " << username << endl;

    // cout << "Hash: " << hash << endl;

    // cout << "RightHash: " << c->getHash() << endl;

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

  // c->emails[emailNumber - 1].print();
}

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
        SendData(c, (to_string(email.getID()) + string(" ") + to_string(email.size) +
                     string("\r\n"))
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
          SendData(c, (string("+OK ") + to_string(a) + string(" ") + to_string(email.size) +
                       string("\r\n"))
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

void Server::NOOP(Client* c, string request) { SendData(c, "+OK\r\n"); }

void Server::QUIT(Client* c, string request) {
  SendData(c, "+OK Bye\r\n");

  Update(c);

  Server::CloseClient(c);

  // clear_userinfo();

  close(c->socket);
}

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

void Server::RSET(Client* c, string request) {
  if (request.length() == 6) {
    for (auto& email : c->emails) {
      if (email.deleteFlag) email.deleteFlag = false;
    }

    SendData(c,
             (string("+OK maildrop has ") + to_string(c->curEmails) + string(" messages (") +
              to_string(c->overalSize) + string(" octets)\r\n"))
                 .c_str());

  } else
    SendData(c, "-ERR bad command\r\n");
}

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

void Server::TOP(Client* c, string request) {

  string numbers = "";

  try {
    numbers = request.substr(4, request.length() - 5);
  } catch (const std::out_of_range& oor) {
    cerr << "ERROR: substr()" << endl;
    return;
  }

  size_t n = count(numbers.begin(), numbers.end(), ' ');

  if(n != 1){
    SendData(c, "-ERR bad command\r\n");
    return;
  }

  stringstream stream(numbers);

  int msgNum, countOfLines;

  stream.exceptions (std::ios::failbit);

  try {

    stream >> msgNum;
    stream >> countOfLines;

  } catch (std::ios::failure e) {
    SendData(c, "-ERR bad command\r\n");
    return;
  }


  if(msgNum < 0){
    SendData(c, (string("-ERR email ") + to_string(msgNum) + string(" was not found\r\n")).c_str());
    return;
  }

  if(countOfLines < 0){
    SendData(c, (string("-ERR number of lines (") + to_string(countOfLines) + string(") have to be a possitive number\r\n")).c_str());
    return;
  }


  for (auto& email : c->emails) {
    if (!email.deleteFlag) {

      if(msgNum == email.getID()){

        istringstream f(email.content);
        string line;  

        int j;
        string result = "";

        for(j = (countOfLines > 0 ? (countOfLines + 11) : 10); getline(f, line) && (j > 0); j--)
          result += line + "\n";

        // cout << j << endl;

        if(j == 0){
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

  if(number.length() == 0) {

    SendData(c, (string("+OK ") + to_string(emailsAfterDel) + string(" messages in maildrop") + string("\r\n")).c_str());

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        SendData(c, (to_string(email.getID()) + string(" ") + email.uniqueName + string("\r\n")).c_str());
      }
    }
    SendData(c, ".\r\n");

  } else {

    size_t n = count(number.begin(), number.end(), ' ');

    if(n != 0){
      SendData(c, "-ERR bad command\r\n");
      return;
    }

    stringstream stream(number);

    int msgNum;

    stream.exceptions (std::ios::failbit);

    try {

      stream >> msgNum;

    } catch (std::ios::failure e) {
      SendData(c, "-ERR bad command\r\n");
      return;
    }

    for (auto& email : c->emails) {
      if (!email.deleteFlag) {
        if(msgNum == email.getID()){
          SendData(c, (string("+OK ") + to_string(msgNum) + string(" ") + email.uniqueName + string("\r\n")).c_str());
          return;
        }
      }
    }

    SendData(c, "-ERR no such message\r\n");
  }
}
