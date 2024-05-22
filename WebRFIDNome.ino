#include <MFRC522.h>  //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <MFRC522Extended.h>
#include <deprecated.h>
#include <require_cpp11.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <vector>
#include <FS.h>  // Inclua a biblioteca do sistema de arquivos SPIFFS
#include "SPIFFS.h"
#include <SPI.h>  //biblioteca para comunicação do barramento SPI
#include <LiquidCrystal.h>

#define SS_PIN 5    // Pino SDA do MFRC522
#define RST_PIN 22  // Pino reset do MFRC522

byte nuidPICC[4] = { 0, 0, 0, 0 };
MFRC522::MIFARE_Key key;                  // Esse objeto 'chave' é utilizado para autenticação
MFRC522::StatusCode status;               // Código de status de retorno da autenticação
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);  // Definicoes pino modulo RC522

String conteudo = "";        // Variável global para armazenar o ID RFID lido e manda-lo para o site 
String conteudoAcesso = "";  // Variável global para armazenar o ID RFID lido e controlar o acesso

LiquidCrystal lcd(25, 26, 33, 32, 14, 27);  //Define os pinos do display lcd

const char *ssid = "Lips";
const char *password = "sala3086";
const char *senha = "3086";

WebServer server(80);

const int solenoide = 15;  // Definicao do pino do solenoide

bool numeroProcessado = false;              // Verifica se o indentificar foi processado na requisicao HTTP
std::vector<String> identificationNumbers;  // Vetor de strings para armazenar os números de identificação

void handleRoot() {  // Configura a rota principal "/"

  if (!server.authenticate(ssid, senha)) {    // Cria a autenticacao para o site 
    return server.requestAuthentication();
  }

  String script = "<script>document.getElementById('identificacao').value = '" + conteudo + "';</script>";

  String message = "<html>\
  <head>\
    <title>Cadastro</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: black; }\
      h1  {font-size: 30px; Color: red;}\
    </style>\
  </head>\
  <body>\
    <h1> LIPS </h1>\
    <h2>Cadastre seu ID</h2>\
    <form method='post'>\
      <label for='identificacao'>Entre com seu nome:</label><br>\
      <input type='text' id='nome' name='nome'><br>\
      <label for='identificacao'>Entre com seu numero de identificacao:</label><br>\
      <input type='text' id='identificacao' name='identificacao'><br>\
      <input type='submit' value='Concluir'>\
    </form>\
    <a href='/identificadores'><button>Ver Identificadores</button></a>\
    <br>\
    <a href='/remover'><button>Remover Identificador</button></a>\
    </body></html>";

  message += script;    // Manda para a pagina o ID lido
  conteudo = "";
  numeroProcessado = false;

  server.send(200, "text/html", message);
}


void handleIdentifiers() {  // Configura a rota para ver cadastros "/identificadores"

  if (!server.authenticate(ssid, senha)) {    // Cria a autenticacao para o site 
    return server.requestAuthentication();
  }

  String message = "<html>\
  <head>\
    <title>Identification Numbers</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: black; }\
    </style>\
  </head>\
  <body>\
    <h1>Numeros de identificacao</h1>\
    <ul>";

  // Adiciona cada número de identificação à lista HTML
  for (int i = 0; i < identificationNumbers.size() + 1; i++) {
    message += "<li>" + identificationNumbers[i] + "</li>";
  }
  message += "</br>";
  message += "<a href='/'><button>Voltar</button></a>";
  message += "</ul></body></html>";

  conteudo = "";
  numeroProcessado = false;

  server.send(200, "text/html", message);
}

void handleRemover() {  // Configura a rota para deletar cadastros "/remover"

  if (!server.authenticate(ssid, senha)) {    // Cria a autenticacao para o site 
    return server.requestAuthentication();
  }

  String script = "<script>document.getElementById('removedor').value = '" + conteudo + "';</script>";
  String message = "<html>\
  <head>\
    <title>Remover Cadastro</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: black; }\
    </style>\
  </head>\
  <body>\
    <h1>Remover ID</h1>\
    <form method='post'>\
      <label for='identificacao'>Entre com o numero de identificacao que deseja remover:</label><br>\
      <input type='text' id='removedor' name='removedor'><br>\
      <input type='submit' value='Concluir'>\
    </form>\
    </ul> ";
  message += "<a href='/'><button>Voltar</button></a>";
  message += "</ul></body></html>";

  message += script;    // Manda para a pagina o ID lido
  conteudo = "";
  numeroProcessado = false;

  server.send(200, "text/html", message);
}

void handleNotFound() {     // Configura a rota desconhecida
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void carregarIdentificadoresFromSPIFFS() {  // Carrega os identificadores da memoria flash
  File file = SPIFFS.open("/identificadores.txt", "r");
  if (file) {
    while (file.available()) {
      String identificadores = file.readStringUntil('\n');
      identificadores.trim();            // Remove espaços em branco extras
      if (!identificadores.isEmpty()) {  // Verifica se a linha não está vazia
        identificationNumbers.push_back(identificadores);   // Adiciona os identificadores na lista
      }
    }
    file.close();
  } else {
    Serial.println("Falha ao abrir identificadores.txt para leitura");
  }
}

void salvarIdentificadoresToSPIFFS(const String &identifier) {  // Salva os identificadores na memoria flash
  File file = SPIFFS.open("/identificadores.txt", "a");
  if (file) {
    file.println(identifier);
    file.close();
  } else {
    Serial.println("Falha ao abrir identificadores.txt para escrita");
  }
}

void removerIdentificador(const String &identifier) {  // Remove um identificador da memoria flash
  String identificador = identifier;
  String nome = "";
  String identif = "";
  for (int k = 0; k < identificationNumbers.size() + 1; k++) {
    int pos3 = identificationNumbers[k].indexOf(',');
    if (pos3 != -1) {
      // A primeira parte é do início até a posição da vírgula
      nome = identificationNumbers[k].substring(0, pos3);
      nome.trim();
      // A segunda parte é da posição após a vírgula até o final
      identif = identificationNumbers[k].substring(pos3 + 1);
      identif.trim();
    }
    if (identifier == identif) {
      identificador = nome + "," + identif;
    } 
  }

  auto it = std::find(identificationNumbers.begin(), identificationNumbers.end(), identificador);  // Procura o identificador no vetor
  if (it != identificationNumbers.end()) {                                                         // Se o identificador for encontrado no vetor
    identificationNumbers.erase(it);                                                               // Remove o identificador do vetor

    // Abre o arquivo em modo de escrita
    File file = SPIFFS.open("/identificadores.txt", "w");
    if (file) {
      // Escreve os identificadores restantes no arquivo
      for (const auto &id : identificationNumbers) {
        file.println(id);
      }
      file.close();
    } else {
      Serial.println("Falha ao abrir identificadores.txt para escrita");
    }
  }
  else {
    Serial.print("Nao entrou");
  }
}

void setup(void) {
  pinMode(solenoide, OUTPUT);
  digitalWrite(solenoide, HIGH);
  lcd.begin(16, 2);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  SPI.begin();
  rfid.PCD_Init();  // Inicia MFRC522

  Serial.println("");
  identificationNumbers.push_back("1");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Inicializa o sistema de arquivos SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Carrega os identificadores do arquivo, se existir
  carregarIdentificadoresFromSPIFFS();

  server.on("/", handleRoot);     // Rota principal
  server.on("/identificadores", handleIdentifiers);  // Rota para exibir os identificadores
  server.on("/remover", handleRemover);              // Rota para remover os identificadores
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  lcd.setCursor(0, 0);
  lcd.print(F("Acesso ao LIPS"));
  lcd.setCursor(0, 1);
  lcd.print(F("Aprox. o cartao"));

  server.handleClient();
  readRFID();

  if (numeroProcessado) {  // Verifica se a requisicao foi processada
    return;
  }

  if (server.hasArg("nome")) {  // Identifica se teve alguma modificao no campo entrar com nome
    String name = server.arg("nome");
    name.trim();
    if (server.hasArg("identificacao")) {  // Identifica se teve alguma modificao no campo entrar com identificacao
      String identif = server.arg("identificacao");
      identif.trim();
      String identification = name + "," + identif;             // Concatena as duas string, nome e id para uma idenficacao apenas
      identification.trim();                                    // Remove espaços em branco extras
      if (!identification.isEmpty() && !procurarID(identif)) {  // Verificar se o Id ja existe
        identificationNumbers.push_back(identification);
        salvarIdentificadoresToSPIFFS(identification);
        Serial.println("Numero de identificacao adicionado: " + identification);
        numeroProcessado = true;
      } else if (procurarID(identif)) {
        Serial.println("Numero de identificacao já adicionado!");
        numeroProcessado = true;
      }
    }
  } else if (server.hasArg("removedor")) {  // Identifica se teve alguma modificao no campo remover com identificacao
    String idRemover = server.arg("removedor");
    idRemover.trim();             // Remove espaços em branco extras
    if (procurarID(idRemover)) {  // Verificar se o Id ja existe
      removerIdentificador(idRemover);
      Serial.println("Identificador removido: " + idRemover);
      numeroProcessado = true;
    } else {
      Serial.println("Nao existe este identificador");
      numeroProcessado = true;
    }
  }
  delay(100);
}

void readRFID(void) {  // Funcao de leitura do cartao no RFID
  if (!rfid.PICC_IsNewCardPresent()) {
    return;  // Se nao tiver um cartao para ser lido recomeça o void loop
  }
  if (!rfid.PICC_ReadCardSerial()) {
    // return;  // Se nao conseguir ler o cartao recomeça o void loop tambem
  }

  conteudo = "";
  for (byte i = 0; i < 4; i++) {  // Transforma o identificar em uma string para envia-la ao web server
    nuidPICC[i] = rfid.uid.uidByte[i];
    conteudo.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(rfid.uid.uidByte[i], HEX));
    conteudo.toUpperCase();
  }

  Serial.print(F("RFID In Hex: "));  // Imprime o identificador do cartao lido no monitor serial
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  conteudoAcesso = conteudo;
  conteudoAcesso.trim();


  if (procurarID(conteudoAcesso)) {  // Verifica se o acesso foi permitido
    lcd.clear();
    Serial.println("Acesso permitido");
    printNome(conteudoAcesso);    // Printa o nome no lcd
    delay(1000);
    digitalWrite(solenoide, LOW);  // Ativa o solenoide para abrir a tranca
    delay(70);
    digitalWrite(solenoide, HIGH);
    lcd.clear();
  } else {
    Serial.println("Acesso negado");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" Acesso negado "));
    delay(1500);
    lcd.clear();
  }

  lcd.clear();
  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void printHex(byte *buffer, byte bufferSize) {  // Funcao que imprime o identificador em hexadecimal no monitor serial
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

bool procurarID(String &identificador) {  // Funcao que procura um identificador no armazenamento
  String nome = "";
  String identif = "";
  for (int j = 0; j < identificationNumbers.size() + 1; j++) {
    int pos1 = identificationNumbers[j].indexOf(',');  // Procura o caractere de separacao
    if (pos1 != -1) {                                  // Separa a identificacao em nome e numero de id
      // A primeira parte é do início até a posição da vírgula
      nome = identificationNumbers[j].substring(0, pos1);
      nome.trim();
      // A segunda parte é da posição após a vírgula até o final
      identif = identificationNumbers[j].substring(pos1 + 1);
      identif.trim();
    }
    if (identificador == identif) {  // Retorna true se achar o numero de idenficacao
      return true;
    }
    else if (identificador == identificationNumbers[j]) {
      return true;
    }
  }
  return false;
}

void printNome(String &identificador) {  // Funcao que printa o nome de identificao
  String nome = "";
  String identif = "";
  for (int j = 0; j < identificationNumbers.size() + 1; j++) {
    int pos2 = identificationNumbers[j].indexOf(',');  // Procura o caractere de separacao
    if (pos2 != -1) {                                  // Separa a identificacao em nome e numero de id
      // A primeira parte é do início até a posição da vírgula
      nome = identificationNumbers[j].substring(0, pos2);
      nome.trim();
      // A segunda parte é da posição após a vírgula até o final
      identif = identificationNumbers[j].substring(pos2 + 1);
      identif.trim();
    }
    if (identificador == identif) {  // Printa no lcd se achar o nome
      lcd.setCursor(0, 0);
      lcd.print(F("Bem vindo!"));
      lcd.setCursor(0, 1);
      lcd.print(nome);
    } else {
      lcd.setCursor(0, 0);
      lcd.print(F("Bem vindo!"));
    }
  }
}
