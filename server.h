#ifndef _server_h_
#define _server_h_

#include "common.h"
#include "thread.h"
#include "client.h"
#include "email.h"
#include "md5.h"


class Server {
 private:
  static vector<Client> clients;

 public:
  static uint16_t port;
  static int clientID, cFlag, resetFlag;
  static char hostname[128];
  static string right_username, right_password, maildirPath;

  int server_sock, client_sock;
  struct sockaddr_in serverAddr, clientAddr;
  char buff[256];
  
  pid_t pid;

 public:
  Server();
  void AcceptAndDispatch();
  static void* HandleClient(void *args);

 private:
  static int FindClientIndex(Client *c);
  static void Respond(Client* c, string request);
  static void SendData(Client* c, const char * data);
  static void CloseClient(Client* c);
  static void AddNewEmails(Client* c);
  static void CurrentEmails(Client* c);
  static void MoveFile(string sourceFile, string destinationFile);
  static void Update(Client* c);

  static void USER(Client* c, string request);
  static void PASS(Client* c, string request);
  static void APOP(Client* c, string request);
  static void DELE(Client* c, string request);
  static void LIST(Client* c, string request);
  static void NOOP(Client* c, string request);
  static void QUIT(Client* c, string request);
  static void RETR(Client* c, string request);
  static void RSET(Client* c, string request);
  static void STAT(Client* c);
  static void TOP(Client* c, string request);
  static void UIDL(Client* c, string request);

};

#endif