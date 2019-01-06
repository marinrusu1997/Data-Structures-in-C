#pragma once
#include <time.h>
#include <functional>

#define INIT_DLCL(dlcl,inspos,iterDir) \
	(dlcl)->head = (dlcl)->tail = nullptr; \
	(dlcl)->insertPosition = (inspos); \
	(dlcl)->iterateDirrection = (iterDir); 
#define INIT_QUEUE(queue) \
	(queue)->head = nullptr; \
	(queue)->tail = nullptr;
#define INIT_STACK_ARRIMPL(stack,capacity) \
	(stack)->players = (LogPlayer**)calloc(capacity,sizeof(LogPlayer*)); \
	(stack)->capacity = capacity; \
	(stack)->top = -1;
#define DESTROY_STACK_ARRIMPL(stack) \
	free( (stack)->players ); \
	(stack)->players = nullptr; \
	(stack)->capacity = 0; \
	(stack)->top = -1;
#define IS_FULL_STACK_ARR_IMPL(stack) \
	(stack)->top == (stack)->capacity - 1
#define IS_EMPTY_STACK_ARR_IMPL(stack) \
	(stack)->top == -1
#define TOP_STACK_ARR_IMPL(stack) \
	(stack)->top == -1 ? nullptr : (stack)->players[(stack)->top]

#define INIT_STACK(stack) \
	(stack) = nullptr;
#define INIT_HASH_TABLE_REHASHING(hash_table,capacity) \
	(hash_table)->capacity = capacity; \
	(hash_table)->players = (LogPlayer**)calloc(capacity,sizeof(LogPlayer*)); \
	(hash_table)->size = 0;
#define DESTROY_HASH_TABLE_REHASHING(hash_table)\
	free((hash_table)->players);\
	(hash_table)->players = nullptr;\
	(hash_table)->capacity = 0;\
	(hash_table)->size = 0;
#define INIT_DEQUE(deque)\
	(deque)->head = (deque)->tail = nullptr;
/*Structuri ramase de implementat in cadrul serverului
	* Deque
	* HashTable
		* linear probing
		* quadratic probing
	* Grafuri
		* matrice
		* liste
	* Arbori B
	* Matrici Rare
*/
/*Structuri de date implementate pina la momentul actual
	* Lista dubla inlantuita
	* Lista dubla circulara
	* Lista simpla inlantuita
	* Lista simpla circulara
	* HashTable
		* chaining
		* rehashing
	* Stiva
		* lista
		* masiv
	* Coada
		* lista
		* masiv
	* Heap -> vector
	* Arbore oarecare -> nod fiu si lista de frati
	* Arbore binar de cautare
	* Arbore AVL
*/
#define INSERR_BTREE_NODE_EXISTS_ALREADY 2356
#define INSERR_AVLREE_NODE_EXISTS_ALREADY 2357
enum class ResourcesManagement {COPY = 0,MOVE = 1};
enum class DataOwnership { OWNER = 0, NOT_OWNER = 1 };
enum class ModeratorPriority {ADMINISTRATOR,MODERATOR,SUPERVISOR};
enum class DLLInsertPosition {HEAD,TAIL};
enum class DLLIterationDirection {FORWARD,BACKWARD};
struct LogPlayer;
using dll_elem_handler = std::function<void(LogPlayer*log, void* args)>;
using hash_func = std::function<uint32_t(const char*)>;
using log_predicate = std::function<bool(LogPlayer*, void*)>;
using log_handler = std::function<void(LogPlayer*, void*)>;

struct Player{
	int idPlayer;
	char* Nickname;
	char** Characters;
	size_t NrOfCharacters;
	float CashAmount;
	int Level;
};

struct heapPlayers {
	Player **Players;
	size_t NoOfPlayers;
	DataOwnership ownership;
};

struct ServerModerator {
	char *Name;
	ModeratorPriority Priority;
};
struct ClientConnection {
	int port;
	const char *adress;
	struct Headers {
		const char *tcp_header;
		const char *ip_header;
		const char *mac_adress;		
	} headers;
};
struct Router {
	const char *adress;
	const char *username;
	const char *password;
	const char *connectionType;
	const char *DNS;
	const char *MAC_AdressClone;
};
struct LogPlayer {
	int idPlayer;
	int port;
	const char *adress;
	time_t time;
};

struct ModeratorNode {
	ServerModerator *moderator;
	ModeratorNode *son, *brother;
	DataOwnership ownership;
};
struct ConnectionNode {
	ClientConnection connection;
	ConnectionNode *left, *right;
};
struct RouterNodeAVL {
	Router *router;
	int GE;
	RouterNodeAVL *left, *right;
	DataOwnership ownership;
};
struct LogPlayerNodeLL {
	LogPlayer *logPlayer;
	LogPlayerNodeLL *next;
	DataOwnership ownership;
};
struct LogPlayerNodeDLL {
	LogPlayer *logPlayer;
	LogPlayerNodeDLL *next, *prev;
	DataOwnership owenership;
};
struct LogPlayerQueue {
	LogPlayerNodeLL *head, *tail;
};
struct LogPlayerQueueArrImpl {
	int size; //numarul de elemente din coada
	size_t capacity; //capacitatea cozii
	int front, rear; //indexul capului si sfirsitului
	LogPlayer **logArr; //coada
	DataOwnership owner; //coada este posesoarea resursei
};
struct LogPlayerStackArrImpl {
	int capacity;
	int top;
	LogPlayer **players;
};

struct LogPlayerHashTable_Rehashing {
	size_t capacity; //capacitatea tabelei
	LogPlayer **players; //vectorul de logs
	unsigned size; //numarul de elemente din tabela
	size_t maximSize; //dimensiunea maxima pe care o are tabela
	//DEFAULT VALUES -> load factor , hash function , hash function pentur calcularea pasului
	float loadFactor = .75f; //load factor indica cind tabela trebuie redimensionata
	hash_func firsh_hash = [](auto key) {
		uint32_t hash, i, len = strlen(key);
		for (hash = i = 0; i < len; ++i)
		{
			hash += key[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return hash;
	}; //functie pentru calcularea hash pozitiei
	hash_func second_hash = [](auto key) {
		//rezultatul intors este diferit de 0
		//mai mic decit dimensiunea masivului
		//functia e diferita de prima functie
		//dimensiunea masivului trebuie sa fie simpla fata de 5,4,3,2
		uint32_t constant = 5;
		return constant - (strlen(key) % constant);
	}; //functie pentru calcului pasului de incrementare
};
struct LogPlayerHashTable_Chaining {
	LogPlayerNodeLL **arrayPlayers;
	size_t capacity;
	uint32_t size, maximSize;
	float_t loadFactor;
	hash_func hash = [](auto key) {
		uint32_t hash = 0x55555555;
		while (*key++ != '\0') {
			hash += *key;
			hash += (hash << 10);
			hash ^= (hash >> 6);			
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return hash;
	};
};

struct LogPlayerDeque {
	LogPlayerNodeLL *head, *tail;
};

struct LogPlayerDoublyLL {
	LogPlayerNodeDLL *head, *tail;
	DLLInsertPosition insertPosition = DLLInsertPosition::HEAD; //default head
	DLLIterationDirection iterateDirrection = DLLIterationDirection::FORWARD;
};
struct GameServer{
	int idServer;
	char *Name;
	Player** PlayersOnline;
	size_t NrOfPlayersOnline;
	heapPlayers PlayersInQueue;
	ModeratorNode* Moderators;
	ConnectionNode *connections;
	RouterNodeAVL *routers;
	LogPlayerNodeLL *logsPlayers;
};

struct stackNode {
	GameServer* server;
	stackNode *next;
	DataOwnership resPolicy;
};

void initLogPlayer(LogPlayer *logPlayer);
void freeLogPlayer(LogPlayer *logPlayer);
void copyLogPlayer(LogPlayer *destination,LogPlayer *source);
void moveLogPlayer(LogPlayer *destination, LogPlayer *source);
void printLogPlayer(LogPlayer *logPlayer);

void initRouter(Router *router);
void freeRouter(Router *router);
void copyRouter(Router *destination, Router *source);
void moveRouter(Router *destination, Router *source);
void printRouter(Router *router);

void initClientConnection(ClientConnection *connection);
void freeClientConnection(ClientConnection *connection);
void copyClientConnection(ClientConnection *destination,ClientConnection *source);
void moveClientConnection(ClientConnection *destination, ClientConnection *source);
void printClientConnection(ClientConnection *connection);

void initModerator(ServerModerator *moderator);
void freeModerator(ServerModerator *moderator);
void copyModerator(ServerModerator *destination, ServerModerator *source);
void moveModerator(ServerModerator *destination, ServerModerator *source);
void printModerator(ServerModerator *moderator);


void initPlayer(Player *player);
void freePlayer(Player *player);
void copyPlayer(Player *source,Player *destination);
void movePlayer(Player *source, Player *destination);
Player* createPlayer(int idPlayer, char* nickname, char** characters,int nrOfCharacters, float cash, int level);
void printPlayer(Player *player);

void initGameServer(GameServer *server);
void freeGameServer(GameServer *server);
void copyGameServer(GameServer *source, GameServer *destination);
void moveGameServer(GameServer *source, GameServer *destination);
bool addPlayerOnline(GameServer *server, Player *player, ResourcesManagement PlayerResManagement);
bool removePlayerOnline(GameServer *server, int idPlayer);
void sortPlayersOnline(GameServer *server);
stackNode * loadGameServersFromFile(const char* fileName);
void loadQueuePlayersFromFile(const char *fileName, GameServer *server);
void printGameServer(GameServer *server);

void pushGameServer(stackNode **head, GameServer *server, DataOwnership policy);
GameServer* popGameServer(stackNode **head);
void destroyGameServerStack(stackNode **head);

bool addPlayerInQueue(GameServer *server, Player *player,ResourcesManagement PlayerResManagement);
void getPlayerFromQueue(GameServer *server, Player **player);

ModeratorNode* loadModerators(const char *fName);
void destroyModerators(ModeratorNode *root);
void preordineModerators(ModeratorNode *root);

ConnectionNode* loadConnections(const char *fName);
void destroyConnectionBTree(ConnectionNode *root);
ConnectionNode* removeConnectionNodeBTree(ConnectionNode *root, int port);
void preordineClientBTree(ConnectionNode *root);
void postordineClientBTree(ConnectionNode *root);
void inordineClientBTree(ConnectionNode *root);
int  inaltimeBTree(ConnectionNode *root);

RouterNodeAVL* loadRouters(const char *fName);
void destroyRouterAVLTree(RouterNodeAVL *root);
void inordineRouterAVLTree(RouterNodeAVL *root);
RouterNodeAVL* removeRouter(RouterNodeAVL *root, const char *adress);

LogPlayerNodeLL* loadLogPlayers(const char *fName);
void removeLogPlayerLL(LogPlayerNodeLL **head, int idPlayer);
void findLogPlayerLL(LogPlayerNodeLL *head, LogPlayer *& logPlayer, int idPlayer);
void destroyLogPlayerLL(LogPlayerNodeLL **head);
void printLogPLayerLL(LogPlayerNodeLL *head);

void destroyLogPlayerLCL(LogPlayerNodeLL **head);
void printLogPlayerLCL(LogPlayerNodeLL *head);
LogPlayerNodeLL* loadLogPlayersLCL(const char *fName);
void removeLogPlayerLCL(LogPlayerNodeLL **head, int idPlayer);
void findLogPlayerLCL(LogPlayerNodeLL *head, LogPlayer *&logPlayer, int idPlayer);

void loadLogsDLLFromBynaryFile(const char *fName, LogPlayerDoublyLL *dll);
int saveLogsDLLToBinaryFile(LogPlayerDoublyLL *dll, const char *fName);
void destroyLogPlayerDLL(LogPlayerDoublyLL *dll);
void initDLL(LogPlayerDoublyLL *dll, DLLInsertPosition ins, DLLIterationDirection dir);
void insertLogPlayerNodeDLL(LogPlayerDoublyLL *dll, LogPlayer *logPlayer, ResourcesManagement resMng,
	DataOwnership ownership);
void findLogPlayerDLL(LogPlayerDoublyLL *dll, LogPlayer *&logPlayer, int idPlayer);
bool removeLogPlayerDLL(LogPlayerDoublyLL *dll, int idPlayer);
void for_each_DLL(const LogPlayerDoublyLL *dll, dll_elem_handler handler, void *handler_args);

void loadLogsDLCLFromBynaryFile(const char *fName, LogPlayerDoublyLL *dll);
void foreachDLCL(LogPlayerDoublyLL *dlcl, dll_elem_handler handler, void* handler_args);
LogPlayer* findLogDLCL(LogPlayerDoublyLL *dlcl, int idPlayer);
bool removeLogDLCL(LogPlayerDoublyLL *dlcl, int idPlayer);
void destroyDLCL(LogPlayerDoublyLL *dlcl);
void insertLogDLCL(LogPlayerDoublyLL *dlcl, LogPlayer *log, DataOwnership ownership, ResourcesManagement resMng);

void putQueue(LogPlayerQueue *queue, LogPlayer *log, DataOwnership ownership, ResourcesManagement resMng);
LogPlayer* getQueue(LogPlayerQueue *queue, bool &isShared);
void destroyQueue(LogPlayerQueue *queue);
void loadLogsQueueFromBynaryFile(const char *fName, LogPlayerQueue *queue);

void initQueueArrImpl(LogPlayerQueueArrImpl *queue, size_t capacity, DataOwnership ownership);
bool putQueueArrImpl(LogPlayerQueueArrImpl *queue, LogPlayer *log, ResourcesManagement resMng);
LogPlayer* getQueueArrImpl(LogPlayerQueueArrImpl *queue, bool &isShared);
void destroyQueueArrImpl(LogPlayerQueueArrImpl *queue);

//specificatorul extern trebuie specificat,pentru ca linkerul sa vada definitia functiei in fisierul .cpp
//i.e by default toate functiile care sunt inline trebuie sa fie definite in aceelasi "translation unit" , i.e in acest header
//dar in cazul nostru definitia se afla in alt fisier,de aceea e nevoie modificatorul extern
extern inline bool pushStackArrImpl(LogPlayerStackArrImpl *stack, LogPlayer *log);
extern inline LogPlayer *popStackArrImpl(LogPlayerStackArrImpl *stack);
extern inline LogPlayer *peekStackArrImpl(LogPlayerStackArrImpl *stack);

extern inline void pushStack(LogPlayerNodeLL **head, LogPlayer *log, ResourcesManagement resMng, DataOwnership ownership);
extern inline LogPlayer* popStack(LogPlayerNodeLL **head, bool &isShared);
extern inline LogPlayer *peekStack(LogPlayerNodeLL *head);

bool initLogHashTable(LogPlayerHashTable_Rehashing *hash_table, size_t capacity, size_t maximSize);
bool insertLogHashTable(LogPlayerHashTable_Rehashing *hash_table, LogPlayer *log);
LogPlayer *removeLogHashTable(LogPlayerHashTable_Rehashing *hash_table, const char *key,bool all);
void findLogHashTable(LogPlayerHashTable_Rehashing *hash_table, std::list<LogPlayer*>& occurences, const char *key,
	log_predicate *predicate, void *args);

//Functie pentru initializarea hash table,returneaza true in caz de succes,false invers
//DEFAULT VALUES -> loadFactor = 0.75;
bool initLogHashTable(LogPlayerHashTable_Chaining *hash_table, size_t capacity, size_t maximSize);
//Functie pentru adaugarea unui element in hash table
//Daca ownership == DataOwnership::OWNER -> se creaza o copie locala a logului in nodul in care se adauga
//											iar in functie de ResourcesManagement are log mutarea sau copierea resurselor 
//											obiectului care se adauga
//Daca ownership == DataOwnership::NOT_OWNER -> are loc copierea adresei logului in cadrul nodului in care se adauga
//											WARNING! copierea sau mutarea resurselor nu are loc
//Return value -> true in caz de succes,false in caz de esec
bool insertLogHashTable(LogPlayerHashTable_Chaining *hash_table, LogPlayer *log, DataOwnership ownership,
	ResourcesManagement resMng);
//Functie pentru cautarea unui element in hash table
//Daca predicate == null -> se adauga in occurences tot clusterul
//Daca predicate != null -> se adauga elementele din cluster care corespund acestui predicat
//WARNING! ** Inainte de inserare lista de occurences se goleste **
void findLogHashTable(LogPlayerHashTable_Chaining *hash_table, std::list<LogPlayer*>& occurences, const char *key,
	log_predicate *predicate, void *args);
//Functie pentru stergerea elementelor din hash table din cadrul unui cluster care corespund unui predicat
//Daca all == false -> se sterge doar prima aparitie
//Daca all == true -> se sterg toate aparitiile
//Return Value -> true in caz de succes , false daca clusterul e gol sau nici un element nu corespunde predicatului
bool removeLogHashTable(LogPlayerHashTable_Chaining *hash_table, const char *key, log_predicate& predicate, void *args, bool all);
//Functie pentru distrugerea hash table
//Dezaloca toate nodurile,apoi vectorul de pointeri la liste
//Intoarce tabela la valori default -> capacity = 0 ; size = 0; arrayPlayers = null;
void destroyLogHashTable(LogPlayerHashTable_Chaining *hash_table);
//Functie pentru procesarea tuturor nodurilor din hash table
void foreachLogHashTable(LogPlayerHashTable_Chaining *hash_table, log_handler handler, void *args);
//Functie pentru procesare unui cluster specific din hash table
void foreachLogHashTable(LogPlayerHashTable_Chaining *hash_table, const char* key, log_handler handler, void *args);

