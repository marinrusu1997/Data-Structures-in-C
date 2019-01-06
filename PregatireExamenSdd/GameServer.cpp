#include "GameServer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <initializer_list>
#include <algorithm>
#include <set>
#define INIT_ITER_DLCL(iterator,dlcl) \
	(dlcl)->iterateDirrection == DLLIterationDirection::FORWARD ? (iterator) = (dlcl)->head : (iterator) = (dlcl)->tail;
#define MOVE_ITER_DLCL(iterator,dlcl) \
	(dlcl)->iterateDirrection == DLLIterationDirection::FORWARD ? (iterator) = (iterator)->next : (iterator) = (iterator)->prev;
#define TEST_EODLCL(iterator,dlcl) \
	(dlcl)->iterateDirrection == DLLIterationDirection::FORWARD ? (iterator) != (dlcl)->head : (iterator) != (dlcl)->tail




void initPlayer(Player *player)
{
	player->idPlayer = 0;
	player->Nickname = nullptr;
	player->Characters = nullptr;
	player->NrOfCharacters = 0;
	player->CashAmount = 0;
	player->Level = 0;
}
void freePlayer(Player *player)
{
	assert(player != nullptr);
	free(player->Nickname);
	for (size_t i = 0; i < player->NrOfCharacters; i++)
		free(player->Characters[i]);
	free(player->Characters);
	player->Nickname = nullptr;
	player->Characters = nullptr;
	player->NrOfCharacters = 0;
}
void copyPlayer(Player *source, Player *destination)
{
	assert(source != nullptr);
	assert(destination != nullptr);
	freePlayer(destination);
	if (source->Nickname) {
		destination->Nickname = (char*)malloc((strlen(source->Nickname) + 1) * sizeof(char));
		strcpy_s(destination->Nickname, strlen(source->Nickname) + 1, source->Nickname);
	}
	else
		destination->Nickname = nullptr;
	if (source->Characters) {
		destination->Characters = (char**)malloc(sizeof(char*) * source->NrOfCharacters);
		destination->NrOfCharacters = source->NrOfCharacters;
		for (size_t i = 0; i < destination->NrOfCharacters; i++) {
			if (source->Characters[i]) {
				destination->Characters[i] = (char*)malloc((strlen(source->Characters[i]) + 1) * sizeof(char));
				strcpy_s(destination->Characters[i], strlen(source->Characters[i]) + 1, source->Characters[i]);
			}
			else
				destination->Characters[i] = nullptr;
		}
	}
	else {
		destination->Characters = nullptr;
		destination->NrOfCharacters = 0;
	}
	destination->idPlayer = source->idPlayer;
	destination->CashAmount = source->CashAmount;
	destination->Level = source->Level;
}
void movePlayer(Player *source, Player *destination)
{
	assert(source != nullptr);
	assert(destination != nullptr);
	freePlayer(destination);
	memcpy(destination, source, sizeof(Player));
	initPlayer(source);
}
Player* createPlayer(int idPlayer, char* nickname, char** characters, int nrOfCharacters, float cash, int level)
{
	assert(nickname != nullptr);
	assert(characters != nullptr);
	assert(nrOfCharacters != 0);
	Player *player = (Player*)malloc(sizeof(Player));
	player->idPlayer = idPlayer;
	player->Nickname = (char*)malloc((strlen(nickname) + 1) * sizeof(char));
	strcpy_s(player->Nickname, strlen(nickname) + 1, nickname);
	player->Characters = (char**)malloc(sizeof(char*) * nrOfCharacters);
	player->NrOfCharacters = nrOfCharacters;
	for (int i = 0; i < nrOfCharacters; i++) {
		if (characters[i]) {
			player->Characters[i] = (char*)malloc((strlen(characters[i]) + 1) * sizeof(char));
			strcpy_s(player->Characters[i], strlen(characters[i]) + 1, characters[i]);
		}
		else
			player->Characters[i] = nullptr;
	}
	player->CashAmount = cash;
	player->Level = level;
	return player;
}

void quickSortPlayersOnline(Player ** players, int nrOfPlayers);
int binarySearchPlayerOnline(Player **players, int first, int last, int idPlayer);
void initHeapPlayers(heapPlayers *heap);
void freeHeapPlayers(heapPlayers *heap);
void copyHeapPlayers(heapPlayers *destination, heapPlayers *source);
void printHeapPlayers(heapPlayers *heap);

/*
		Arrays and Matrix
*/

void initGameServer(GameServer *server)
{
	assert(server != nullptr);
	server->idServer = 0;
	server->PlayersOnline = nullptr;
	server->NrOfPlayersOnline = 0;
	server->Name = nullptr;
	initHeapPlayers(&server->PlayersInQueue);
	server->Moderators = nullptr;
	server->connections = nullptr;
	server->routers = nullptr;
	server->logsPlayers = nullptr;
}
void freeGameServer(GameServer *server)
{
	assert(server != nullptr);
	for (size_t i = 0; i < server->NrOfPlayersOnline; i++) {
		freePlayer(server->PlayersOnline[i]); //clear player struct
		free(server->PlayersOnline[i]); //free player
	}
	free(server->PlayersOnline); //free array of pointers to player
	free(server->Name);
	freeHeapPlayers(&server->PlayersInQueue);
	destroyModerators(server->Moderators);
	destroyConnectionBTree(server->connections);
	destroyRouterAVLTree(server->routers);
	destroyLogPlayerLL(&server->logsPlayers);
	initGameServer(server); //default state
}
//TODO Update copy and move gameServer
void copyGameServer(GameServer *source, GameServer *destination)
{
	assert(source != nullptr);
	assert(destination != nullptr);
	freeGameServer(destination);
	destination->idServer = source->idServer;
	destination->PlayersOnline = (Player**)malloc(sizeof(Player*) * source->NrOfPlayersOnline);
	destination->NrOfPlayersOnline = source->NrOfPlayersOnline;
	for (size_t i = 0; i < destination->NrOfPlayersOnline; i++)
	{
		destination->PlayersOnline[i] = (Player*)malloc(sizeof(Player));
		initPlayer(destination->PlayersOnline[i]);
		copyPlayer(source->PlayersOnline[i], destination->PlayersOnline[i]);
	}
	destination->Name = (char*)malloc(sizeof(char) * (strlen(source->Name) + 1));
	strcpy_s(destination->Name, strlen(source->Name) + 1, source->Name);
	copyHeapPlayers(&destination->PlayersInQueue, &source->PlayersInQueue);
}
void moveGameServer(GameServer *source, GameServer *destination)
{
	assert(source != nullptr);
	assert(destination != nullptr);
	freeGameServer(destination);
	memcpy(destination, source, sizeof(GameServer));
	initGameServer(source);
}
bool addPlayerOnline(GameServer *server, Player *player, ResourcesManagement PlayerResManagement)
{
	assert(server != nullptr);
	assert(player != nullptr);
	if (!server->PlayersOnline) {
		server->PlayersOnline = (Player**)malloc(sizeof(Player*));
		server->NrOfPlayersOnline = 1;
		server->PlayersOnline[0] = (Player*)malloc(sizeof(Player));
		initPlayer(server->PlayersOnline[0]);
		if (PlayerResManagement == ResourcesManagement::COPY)
			copyPlayer(player, server->PlayersOnline[0]);
		else
			movePlayer(player, server->PlayersOnline[0]);
		return true;
	}
	else {
		Player **tempArr = (Player**)realloc(server->PlayersOnline, sizeof(Player) * (server->NrOfPlayersOnline + 1));
		if (tempArr) {
			tempArr[server->NrOfPlayersOnline] = (Player*)malloc(sizeof(Player));
			initPlayer(tempArr[server->NrOfPlayersOnline]);
			if (PlayerResManagement == ResourcesManagement::COPY)
				copyPlayer(player, tempArr[server->NrOfPlayersOnline]);
			else
				movePlayer(player, tempArr[server->NrOfPlayersOnline]);
			server->PlayersOnline = tempArr;
			server->NrOfPlayersOnline = server->NrOfPlayersOnline + 1;
			quickSortPlayersOnline(server->PlayersOnline, server->NrOfPlayersOnline - 1);
			return true;
		}
		else
			return false;
	}
}
void sortPlayersOnline(GameServer *server)
{
	quickSortPlayersOnline(server->PlayersOnline, server->NrOfPlayersOnline - 1); //players and last index
}
void quickSortPlayersOnline(Player ** players, int nrOfPlayers)
{
	int size = nrOfPlayers;
	int k = 0;
	Player *pivot = players[(int)size / 2];
	Player *tmp = nullptr;
	do {
		while (players[k]->idPlayer < pivot->idPlayer) k++;
		while (players[nrOfPlayers]->idPlayer > pivot->idPlayer)nrOfPlayers--;

		if (k <= nrOfPlayers) {
			tmp = players[k];
			players[k] = players[nrOfPlayers];
			players[nrOfPlayers] = tmp;
			k++;
			nrOfPlayers--;
		}
	} while (k <= nrOfPlayers);
	if (nrOfPlayers > 0) quickSortPlayersOnline(players, nrOfPlayers);
	if (k < size) quickSortPlayersOnline(players + k, size - k);
}
bool removePlayerOnline(GameServer *server, int idPlayer)
{
	int poz = binarySearchPlayerOnline(server->PlayersOnline, 0, server->NrOfPlayersOnline - 1, idPlayer);
	if (poz == -1)
		return false;
	freePlayer(server->PlayersOnline[poz]);
	free(server->PlayersOnline[poz]);
	for (size_t i = poz; i < (server->NrOfPlayersOnline - 1); i++)
		server->PlayersOnline[i] = server->PlayersOnline[i + 1];
	Player ** tempPlayers = server->PlayersOnline;
	server->NrOfPlayersOnline = server->NrOfPlayersOnline - 1;
	server->PlayersOnline = (Player**)malloc(sizeof(Player*) * server->NrOfPlayersOnline);
	for (size_t i = 0; i < server->NrOfPlayersOnline; i++)
		server->PlayersOnline[i] = tempPlayers[i];
	free(tempPlayers);
	return true;
}
int binarySearchPlayerOnline(Player **players, int first, int last, int idPlayer)
{
	int middle, poz;
	if (first > last)
		return -1;
	middle = (first + last) / 2;
	if (players[middle]->idPlayer == idPlayer)
		poz = middle;
	if (idPlayer > players[middle]->idPlayer)
		poz = binarySearchPlayerOnline(players, middle + 1, last, idPlayer);
	else if (idPlayer < players[middle]->idPlayer)
		poz = binarySearchPlayerOnline(players, first, middle - 1, idPlayer);
	return poz;
}
stackNode * loadGameServersFromFile(const char* fileName) {
	FILE *f = fopen(fileName, "r");
	if (f) {
		stackNode *head = nullptr;
		GameServer* server; char buffer[70];
		while (!feof(f)) {
			server = (GameServer*)malloc(sizeof(GameServer));
			initGameServer(server);
			fscanf(f, "%d", &server->idServer);
			fscanf(f, "%s", buffer);
			server->Name = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
			strcpy_s(server->Name, strlen(buffer) + 1, buffer);
			fscanf(f, "%d", &server->NrOfPlayersOnline);
			server->PlayersOnline = (Player**)malloc(sizeof(Player*) * server->NrOfPlayersOnline);
			for (size_t i = 0; i < server->NrOfPlayersOnline; i++) {
				Player *player = (Player*)malloc(sizeof(Player));
				initPlayer(player);
				fscanf(f, "%d", &player->idPlayer);
				fscanf(f, "%s", buffer);
				player->Nickname = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
				strcpy_s(player->Nickname, strlen(buffer) + 1, buffer);
				fscanf(f, "%d", &player->NrOfCharacters);
				player->Characters = (char**)malloc(sizeof(char*) * player->NrOfCharacters);
				for (size_t j = 0; j < player->NrOfCharacters; j++) {
					fscanf(f, "%s", buffer);
					player->Characters[j] = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
					strcpy_s(player->Characters[j], strlen(buffer) + 1, buffer);
				}
				fscanf(f, "%f", &player->CashAmount);
				fscanf(f, "%d", &player->Level);
				server->PlayersOnline[i] = player;
			}
			loadQueuePlayersFromFile("QueuePlayers.txt", server);
			server->Moderators = loadModerators("Moderators.txt");
			server->connections = loadConnections("Conexiuni.txt");
			server->routers = loadRouters("Routers.txt");
			server->logsPlayers = loadLogPlayers("LogPlayers.txt");
			pushGameServer(&head, server, DataOwnership::NOT_OWNER);
			sortPlayersOnline(server);
		}
		fclose(f);
		return head;
	}
	else {
		printf("Unnable to open %s", fileName);
		return nullptr;
	}
}
void loadQueuePlayersFromFile(const char *fileName, GameServer *server) {
	FILE *f = fopen(fileName, "r");
	if (f) {
		Player *player = nullptr; char buffer[70];
		server->PlayersInQueue.ownership = DataOwnership::OWNER;
		while (!feof(f)) {
			player = (Player*)malloc(sizeof(Player));
			initPlayer(player);
			fscanf(f, "%d", &player->idPlayer);
			fscanf(f, "%s", buffer);
			player->Nickname = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
			strcpy_s(player->Nickname, strlen(buffer) + 1, buffer);
			fscanf(f, "%d", &player->NrOfCharacters);
			player->Characters = (char**)malloc(sizeof(char*) * player->NrOfCharacters);
			for (size_t j = 0; j < player->NrOfCharacters; j++) {
				fscanf(f, "%s", buffer);
				player->Characters[j] = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
				strcpy_s(player->Characters[j], strlen(buffer) + 1, buffer);
			}
			fscanf(f, "%f", &player->CashAmount);
			fscanf(f, "%d", &player->Level);
			addPlayerInQueue(server, player, ResourcesManagement::MOVE);
			free(player);
		}
		fclose(f);
	}
	else {
		printf("\n Unnable to open %s", fileName);
	}
}
void printPlayer(Player *player) {
	printf("\n Player ID=%d,Nickname=%s,Cash Amount=%f,Level=%d,Characters:\n",
		player->idPlayer, player->Nickname, player->CashAmount, player->Level);
	for (size_t i = 0; i < player->NrOfCharacters; i++)
		printf("%s\n", player->Characters[i]);
}
void printGameServer(GameServer *server) {
	printf("\n Server ID=%d,Name=%s,Players Online:", server->idServer, server->Name);
	for (size_t i = 0; i < server->NrOfPlayersOnline; i++)
		printPlayer(server->PlayersOnline[i]);
	printf("Players In Queue:");
	printHeapPlayers(&server->PlayersInQueue);
	printf("Moderators:");
	preordineModerators(server->Moderators);
	printf("Conexiuni:");
	inordineClientBTree(server->connections);
	printf("Routers:");
	inordineRouterAVLTree(server->routers);
	printf("Logs Players:");
	printLogPLayerLL(server->logsPlayers);
}

stackNode* makeStackNode(GameServer *server, DataOwnership policy) {
	stackNode * node = (stackNode*)malloc(sizeof(stackNode));
	node->resPolicy = policy;
	node->next = nullptr;
	if (node->resPolicy == DataOwnership::NOT_OWNER)
		node->server = server;
	else if (node->resPolicy == DataOwnership::OWNER) {
		node->server = (GameServer*)malloc(sizeof(GameServer));
		initGameServer(node->server);
		copyGameServer(server, node->server);
	}
	return node;
}
void pushGameServer(stackNode **head, GameServer *server, DataOwnership policy) {
	if (*head == nullptr)
	{
		*head = makeStackNode(server, policy);
	}
	else {
		stackNode *new_head = makeStackNode(server, policy);
		new_head->next = *head;
		*head = new_head;
	}
}
GameServer* popGameServer(stackNode **head) {
	if (*head == nullptr)
		return nullptr;
	stackNode *old_head = *head;
	*head = (*head)->next;
	GameServer *server = old_head->server;
	free(old_head);
	return server;
}
void destroyGameServerStack(stackNode **head)
{
	stackNode *aux = nullptr;
	while (*head != nullptr) {
		if ((*head)->resPolicy == DataOwnership::OWNER) {
			freeGameServer((*head)->server);
			free((*head)->server);
		}
		aux = (*head)->next;
		free(*head);
		*head = aux;
	}
}


/*
		Heap
*/

void initHeapPlayers(heapPlayers *heap) {
	heap->Players = nullptr;
	heap->NoOfPlayers = 0;
	heap->ownership = DataOwnership::OWNER;
}
void freeHeapPlayers(heapPlayers *heap) {
	if (heap->ownership == DataOwnership::OWNER) {
		for (size_t i = 0; i < heap->NoOfPlayers; i++) {
			freePlayer(heap->Players[i]);
			free(heap->Players[i]);
		}
	}
	free(heap->Players);
	heap->Players = nullptr;
	heap->NoOfPlayers = 0;
}
void copyHeapPlayers(heapPlayers *destination, heapPlayers *source) {
	freeHeapPlayers(destination);
	destination->NoOfPlayers = source->NoOfPlayers;
	destination->ownership = source->ownership;
	destination->Players = (Player**)malloc(sizeof(Player*) * destination->NoOfPlayers);
	for (size_t i = 0; i < destination->NoOfPlayers; i++) {
		destination->Players[i] = (Player*)malloc(sizeof(Player));
		initPlayer(destination->Players[i]);
		copyPlayer(source->Players[i], destination->Players[i]);
	}
}
void moveHeapPlayers(heapPlayers *destination, heapPlayers *source) {
	freeHeapPlayers(destination);
	memcpy(destination, source, sizeof(heapPlayers));
	initHeapPlayers(source);
}
void filterDownHeapPlayers(heapPlayers *heap, int parent = 0) {
	int indexMax = parent;
	size_t leftSon = 2 * parent + 1;
	size_t rightSon = 2 * parent + 2;

	if (leftSon < heap->NoOfPlayers && heap->Players[leftSon]->idPlayer > heap->Players[indexMax]->idPlayer)
		indexMax = leftSon;
	if (rightSon < heap->NoOfPlayers && heap->Players[rightSon]->idPlayer > heap->Players[indexMax]->idPlayer)
		indexMax = rightSon;

	if (indexMax != parent) {
		Player *temp = heap->Players[parent];
		heap->Players[parent] = heap->Players[indexMax];
		heap->Players[indexMax] = temp;
		filterDownHeapPlayers(heap, indexMax);
	}
}
void filterUpHeapPlayers(heapPlayers *heap, int child) {
	int parent = (child - 1) / 2;
	if (parent >= 0 && heap->Players[child]->idPlayer > heap->Players[parent]->idPlayer) {
		Player *temp = heap->Players[parent];
		heap->Players[parent] = heap->Players[child];
		heap->Players[child] = temp;
		filterUpHeapPlayers(heap, parent);
	}
}
bool insertHeapPlayers(heapPlayers *heap, Player *player, ResourcesManagement PlayerResManagement) {
	Player **tempPlayers = (Player**)realloc(heap->Players, (heap->NoOfPlayers + 1) * sizeof(Player*));
	if (tempPlayers) {
		heap->NoOfPlayers++;
		heap->Players = tempPlayers;
		Player *playerHeap = nullptr;
		if (heap->ownership == DataOwnership::NOT_OWNER)
			playerHeap = player;
		if (heap->ownership == DataOwnership::OWNER) {
			playerHeap = (Player*)malloc(sizeof(Player));
			initPlayer(playerHeap);
			if (PlayerResManagement == ResourcesManagement::COPY)
				copyPlayer(player, playerHeap);
			if (PlayerResManagement == ResourcesManagement::MOVE)
				movePlayer(player, playerHeap);
		}
		heap->Players[heap->NoOfPlayers - 1] = playerHeap;

		filterUpHeapPlayers(heap, heap->NoOfPlayers - 1);
		return true;
	}
	else {
		return false;
	}
}
void removeHeapPlayers(heapPlayers *heap, Player **player) {
	if (!heap->NoOfPlayers) {
		*player = nullptr;
		return;
	}
	Player **new_players = (Player**)malloc(sizeof(Player*) * (heap->NoOfPlayers - 1));
	*player = heap->Players[0];

	Player *temp = heap->Players[0];
	heap->Players[0] = heap->Players[heap->NoOfPlayers - 1];
	heap->Players[heap->NoOfPlayers - 1] = temp;

	for (size_t i = 0; i < heap->NoOfPlayers - 1; i++)
		new_players[i] = heap->Players[i];
	heap->NoOfPlayers--;
	free(heap->Players);
	heap->Players = new_players;

	filterDownHeapPlayers(heap);
}
void printHeapPlayers(heapPlayers *heap)
{
	for (size_t i = 0; i < heap->NoOfPlayers; i++)
		printPlayer(heap->Players[i]);
}

bool addPlayerInQueue(GameServer *server, Player *player, ResourcesManagement PlayerResManagement) {
	return insertHeapPlayers(&server->PlayersInQueue, player, PlayerResManagement);
}
void getPlayerFromQueue(GameServer *server, Player **player) {
	removeHeapPlayers(&server->PlayersInQueue, player);
}

void initModerator(ServerModerator *moderator) {
	moderator->Name = nullptr;
	moderator->Priority = ModeratorPriority::SUPERVISOR;
}
void freeModerator(ServerModerator *moderator) {
	free(moderator->Name);
	moderator->Name = nullptr;
}
void copyModerator(ServerModerator *destination, ServerModerator *source) {
	freeModerator(destination);
	destination->Name = (char*)malloc((strlen(source->Name) + 1) * sizeof(char));
	strcpy_s(destination->Name, strlen(source->Name) + 1, source->Name);
	destination->Priority = source->Priority;
}
void moveModerator(ServerModerator *destination, ServerModerator *source) {
	freeModerator(destination);
	memcpy(destination, source, sizeof(ServerModerator));
	initModerator(source);
}
const char* moderatorRoleToString(ModeratorPriority priority) {
	switch (priority)
	{
	case ModeratorPriority::ADMINISTRATOR:
		return "Administrator";
		break;
	case ModeratorPriority::MODERATOR:
		return "Moderator";
		break;
	case ModeratorPriority::SUPERVISOR:
		return "Supervisor";
		break;
	default:
		return "Unknown";
		break;
	}
}
ModeratorPriority moderatorRoleToPriority(const char *priority) {
	if (strcmp(priority, "ADMINISTRATOR") == 0)
		return ModeratorPriority::ADMINISTRATOR;
	if (strcmp(priority, "MODERATOR") == 0)
		return ModeratorPriority::MODERATOR;
	if (strcmp(priority, "SUPERVISOR") == 0)
		return ModeratorPriority::SUPERVISOR;
}
void printModerator(ServerModerator *moderator) {
	printf("\nModerator Name: %s, Role: %s", moderator->Name, moderatorRoleToString(moderator->Priority));
}

/*	
			Arbore oarecare
*/

ModeratorNode* makeModeratorNode(ServerModerator *moderator, DataOwnership ownership,
	ResourcesManagement ModerResManagement) {
	ModeratorNode *node = (ModeratorNode*)malloc(sizeof(ModeratorNode));
	node->brother = nullptr;
	node->son = nullptr;
	node->ownership = ownership;
	if (ownership == DataOwnership::NOT_OWNER)
		node->moderator = moderator;
	if (ownership == DataOwnership::OWNER) {
		ServerModerator *moderatorNode = (ServerModerator*)malloc(sizeof(ServerModerator));
		initModerator(moderatorNode);
		if (ModerResManagement == ResourcesManagement::COPY)
			copyModerator(moderatorNode, moderator);
		if (ModerResManagement == ResourcesManagement::MOVE)
			moveModerator(moderatorNode, moderator);
		node->moderator = moderatorNode;
	}
	return node;
}
void findModeratorNode(ModeratorNode *root, const char* name, ModeratorNode *&found) {
	if (root) {
		if (strcmp(root->moderator->Name, name) == 0) { //prelucrez nodul curent
			found = root;
			return; //stop recursie
		}
		findModeratorNode(root->son, name, found); //parsare primul subarbore descendent al lui root
		if (found)
			return; //moderatorul a fost gasit in primul subarbore
		if (root->son) {
			ModeratorNode *temp = root->son;
			while (temp->brother) {
				findModeratorNode(temp->brother, name, found);
				if (found) //moderatorul a fost gasit in unul din subarbori
					return;
				temp = temp->brother;
			}
		}
	}
}
void preordineModerators(ModeratorNode *root) {
	if (root) {
		printModerator(root->moderator);

		preordineModerators(root->son);

		if (root->son) {
			ModeratorNode *tmp = root->son;
			while (tmp->brother) {
				preordineModerators(tmp->brother);
				tmp = tmp->brother;
			}
		}
	}
}
ModeratorNode *inserare(ModeratorNode *r, ServerModerator *moderator, const char *numeSuperior,
	DataOwnership ownership, ResourcesManagement resManagement) {

	if (!numeSuperior) {
		ModeratorNode* nou = makeModeratorNode(moderator, ownership, resManagement); // nodul care se insereaza devine nod radacina

		if (r) {
			nou->son = r;
		}

		r = nou;
	}
	else {
		ModeratorNode* nou = makeModeratorNode(moderator, ownership, resManagement);

		ModeratorNode* p = nullptr;
		findModeratorNode(r, numeSuperior, p);
		if (!p->son)
			p->son = nou;
		else {

			if (!p->son->brother)
				p->son->brother = nou;
			else {
				ModeratorNode* tmp = p->son;
				while (tmp->brother)
					tmp = tmp->brother;

				tmp->brother = nou;
			}
		}
	}

	return r;
}
void destroyModerators(ModeratorNode *root) {
	if (root) {
		destroyModerators(root->son);
		destroyModerators(root->brother);
		if (root->ownership == DataOwnership::OWNER) {
			freeModerator(root->moderator);
			free(root->moderator);
		}
		free(root);
	}
}
ModeratorNode* loadModerators(const char *fName) {
	FILE *f = fopen(fName, "r");
	if (f) {
		ServerModerator * moderator; char buffer[100]; ModeratorNode * root = nullptr;
		while (!feof(f)) {
			char **tokens = (char**)malloc(3 * sizeof(char*));
			fscanf(f, "%s", buffer);
			int lineNr = 0;
			for (char *token = strtok(buffer, ","); token != nullptr; lineNr++, token = strtok(NULL, ",")) {
				tokens[lineNr] = _strdup(token);
			}
			moderator = (ServerModerator*)malloc(sizeof(ServerModerator));
			moderator->Name = tokens[0];
			moderator->Priority = moderatorRoleToPriority(tokens[1]);
			if (moderator->Priority == ModeratorPriority::ADMINISTRATOR)
				root = inserare(root, moderator, nullptr, DataOwnership::OWNER, ResourcesManagement::MOVE);
			else
				root = inserare(root, moderator, tokens[2], DataOwnership::OWNER, ResourcesManagement::MOVE);
			for (int i = 1; i < 3; i++)
				free(tokens[i]);
			free(tokens);
			free(moderator);
		}
		fclose(f);
		return root;
	}
	else {
		printf("\n Eroare la deschiderea fisier");
		return nullptr;
	}
}

void initClientConnection(ClientConnection *connection) {
	connection->adress = nullptr;
	connection->port = 0;
	connection->headers.tcp_header = nullptr;
	connection->headers.ip_header = nullptr;
	connection->headers.mac_adress = nullptr;
}
void freeClientConnection(ClientConnection *connection) {
	free((void*)connection->adress);
	free((void*)connection->headers.tcp_header);
	free((void*)connection->headers.ip_header);
	free((void*)connection->headers.mac_adress);
}
void copyClientConnection(ClientConnection *destination, ClientConnection *source) {
	freeClientConnection(destination);
	destination->port = source->port;
	destination->adress = _strdup(source->adress);
	destination->headers.tcp_header = _strdup(source->headers.tcp_header);
	destination->headers.ip_header = _strdup(source->headers.ip_header);
	destination->headers.mac_adress = _strdup(source->headers.mac_adress);
}
void moveClientConnection(ClientConnection *destination, ClientConnection *source) {
	freeClientConnection(destination);
	memcpy(destination, source, sizeof(ClientConnection));
	initClientConnection(source);
}
void printClientConnection(ClientConnection *connection) {
	printf("\n Port %d,Adress %s,TCP Header %s,IP Header %s,MAC Adress %s",
		connection->port, connection->adress, connection->headers.tcp_header,
		connection->headers.ip_header, connection->headers.mac_adress);
}

/*
		Binary Tree
*/

ConnectionNode* makeConnectionNode(ClientConnection *connection, ResourcesManagement resManagement) {
	ConnectionNode *node = (ConnectionNode*)malloc(sizeof(ConnectionNode));
	node->left = nullptr;
	node->right = nullptr;
	initClientConnection(&node->connection);
	if (resManagement == ResourcesManagement::COPY) {
		copyClientConnection(&node->connection, connection);
	}
	if (resManagement == ResourcesManagement::MOVE) {
		moveClientConnection(&node->connection, connection);
	}
	return node;
}
ConnectionNode* insertConnectionNode(ConnectionNode *root, ClientConnection *connection,
	ResourcesManagement resManagement, int *errorCode) {
	if (!root) {
		root = makeConnectionNode(connection, resManagement);
	}
	else if (root->connection.port == connection->port) *errorCode = INSERR_BTREE_NODE_EXISTS_ALREADY;
	else
		if (connection->port < root->connection.port)
			root->left = insertConnectionNode(root->left, connection, resManagement, errorCode);
		else
			root->right = insertConnectionNode(root->right, connection, resManagement, errorCode);
	return root;
}
void findConnection(ConnectionNode *root, int port, ClientConnection **connection) {
	if (root) {
		if (root->connection.port == port)
			*connection = &root->connection;
		else if (port < root->connection.port)
			findConnection(root->left, port, connection);
		else
			findConnection(root->right, port, connection);
	}
}
void findConnectionNode(ConnectionNode *root, int port, ConnectionNode **connectionNode) {
	if (root) {
		if (root->connection.port == port)
			*connectionNode = root;
		else if (port < root->connection.port)
			findConnectionNode(root->left, port, connectionNode);
		else
			findConnectionNode(root->right, port, connectionNode);
	}
}
void destroyConnectionBTree(ConnectionNode *root) {
	if (root) {
		freeClientConnection(&root->connection);
		destroyConnectionBTree(root->left);
		destroyConnectionBTree(root->right);
		free(root);
	}
}
void minValueNode(ConnectionNode *root, ConnectionNode **node) {
	if (root) {
		*node = root;
		while ((*node)->left != nullptr)
			*node = (*node)->left;
	}
}
ConnectionNode* removeConnectionNodeBTree(ConnectionNode *root, int port) {
	//daca root e null inturnam null inapoi
	if (!root) return root;
	//cautam sa stergem in stringa
	//noul subarbore care deja nu mai contine nodul de sters se atribuie in nodul sting al nodului curent
	//astfel recursiv se vor reconstrui subarborii
	if (port < root->connection.port)
		root->left = removeConnectionNodeBTree(root->left, port);
	//sau in dreapta
	else if (port > root->connection.port)
		root->right = removeConnectionNodeBTree(root->right, port);
	//daca l-am gasit
	else if (port == root->connection.port) {
		//cazul cind are doar 1 nod frunza sau niciunul
		//dezalocam nodul
		//interschimbam nodul care il stergem cu copilul pe care il are
		if (root->left == nullptr) {
			ConnectionNode *temp = root->right;
			freeClientConnection(&root->connection);
			free(root);
			return temp;
		}
		if (root->right == nullptr) {
			ConnectionNode *temp = root->left;
			freeClientConnection(&root->connection);
			free(root);
			return temp;
		}
		//cazul in care are 2 descendenti
		//cautam nodul cel mai mic din arborele din dreapta al nodului de sters
		ConnectionNode *min = nullptr;
		minValueNode(root->right, &min);
		//muta informatia din nodul minim in nodul curent
		//WARNING dupa iesirea din functia move portul sursei va fi 0 , trebuie salvat portul sursei inainte de stergere
		port = min->connection.port;
		moveClientConnection(&root->connection, &min->connection);
		min->connection.port = port;
		//sterge nodul minim si intoarce noul subarbore modificat in subarborele drept al nodului curent
		root->right = removeConnectionNodeBTree(root->right, port);
	}
	//intoarce nodul care a avut 2 descendenti
	return root;
}
void preordineClientBTree(ConnectionNode *root) {
	if (root) {
		printClientConnection(&root->connection);
		preordineClientBTree(root->left);
		preordineClientBTree(root->right);
	}
}
void postordineClientBTree(ConnectionNode *root) {
	if (root) {
		postordineClientBTree(root->left);
		postordineClientBTree(root->right);
		printClientConnection(&root->connection);
	}
}
void inordineClientBTree(ConnectionNode *root) {
	if (root) {
		inordineClientBTree(root->left);
		printClientConnection(&root->connection);
		inordineClientBTree(root->right);
	}
}
int  inaltimeBTree(ConnectionNode *root) {
	if (root)
		return 1 + std::max(inaltimeBTree(root->right), inaltimeBTree(root->left));
	else
		return 0;
}
ConnectionNode* loadConnections(const char *fName) {
	FILE *f = fopen(fName, "r");
	if (f) {
		ClientConnection connection; char buffer[300]; //declarare buffere
		ConnectionNode *root = nullptr; int errorCode = 0; //parametrii pentru BTree , nullptr inseamna noul root 
		char **tokens = (char**)malloc(5 * sizeof(char*)); //buffer pentru tokenuri de pe linie din fisier
		while (!feof(f)) {
			fgets(buffer, 300, f); //citeste linie
			int columnNr = 0;
			for (char *token = strtok(buffer, ","); token != nullptr; token = strtok(nullptr, ","))
				tokens[columnNr++] = _strdup(token);
			connection.port = atoi(tokens[0]);
			connection.adress = tokens[1];
			connection.headers.tcp_header = tokens[2];
			connection.headers.ip_header = tokens[3];
			connection.headers.mac_adress = tokens[4];
			//muta resursele din buffer in nodul din btree
			root = insertConnectionNode(root, &connection, ResourcesManagement::MOVE, &errorCode);
			//daca mutarea nu a avut succes,curata bufferul
			if (errorCode == INSERR_BTREE_NODE_EXISTS_ALREADY)
				freeClientConnection(&connection);
			//curata stringul cu port
			free(tokens[0]);
		}
		//curata vectorul de pointeri catre tokens
		free(tokens);
		//inchide fisier
		fclose(f);
		return root;
	}
	else {
		printf("\n Unnable to open file");
		return nullptr;
	}
}

void initRouter(Router *router) {
	router->adress = nullptr;
	router->connectionType = nullptr;
	router->DNS = nullptr;
	router->MAC_AdressClone = nullptr;
	router->password = nullptr;
	router->username = nullptr;
}
void freeRouter(Router *router) {
	free((void*)router->adress);
	free((void*)router->connectionType);
	free((void*)router->DNS);
	free((void*)router->MAC_AdressClone);
	free((void*)router->password);
	free((void*)router->username);

}
void copyRouter(Router *destination, Router *source) {
	freeRouter(destination);
	destination->adress = _strdup(source->adress);
	destination->connectionType = _strdup(source->connectionType);
	destination->DNS = _strdup(source->DNS);
	destination->MAC_AdressClone = _strdup(source->MAC_AdressClone);
	destination->password = _strdup(source->password);
	destination->username = _strdup(source->username);
}
void moveRouter(Router *destination, Router *source) {
	freeRouter(destination);
	memcpy(destination, source, sizeof(Router));
	initRouter(source);
}
void printRouter(Router *router) {
	printf("\nRouter Adress %s,Connection Type %s,DNS %s,MAC Adress Clone %s,Username %s,Password %s",
		router->adress, router->connectionType, router->DNS, router->MAC_AdressClone, router->username, router->password);
}

/*
		Arbori AVL
*/

RouterNodeAVL *makeRouteNodeAVL(Router *router, DataOwnership ownership, ResourcesManagement resManagement) {
	RouterNodeAVL *node = (RouterNodeAVL*)malloc(sizeof(RouterNodeAVL));
	node->GE = 0;
	node->left = node->right = nullptr;
	node->ownership = ownership;
	if (ownership == DataOwnership::NOT_OWNER)
		node->router = router;
	if (ownership == DataOwnership::OWNER) {
		Router *routerAVL = (Router*)malloc(sizeof(Router));
		initRouter(routerAVL);
		if (resManagement == ResourcesManagement::COPY)
			copyRouter(routerAVL, router);
		if (resManagement == ResourcesManagement::MOVE)
			moveRouter(routerAVL, router);
		node->router = routerAVL;
	}
	return node;
}
int inaltimeAVLTree(RouterNodeAVL *root) {
	if (root)
		return 1 + std::max(inaltimeAVLTree(root->left), inaltimeAVLTree(root->right));
	return 0;
}
void calculGE(RouterNodeAVL *root) {
	if (root)
		root->GE = inaltimeAVLTree(root->right) - inaltimeAVLTree(root->left);
}
RouterNodeAVL* rotireSimplaDreapta(RouterNodeAVL *pivot, RouterNodeAVL *fiuSt) {
	pivot->left = fiuSt->right; //subarborele dreapt al fiului din stinga se duce in stinga pivotului
	calculGE(pivot); //calcul GE al pivotului modificat
	fiuSt->right = pivot; //urcind cu un nivel mai sus,fiul din stinga devine root local si pe dreapta lui se ancoreaza subarborele pivotului
	calculGE(fiuSt); //calcul GE pentru fiul din Stinga modificat

	return fiuSt; //intoarcere noul root local
}
RouterNodeAVL* rotireSimplaStinga(RouterNodeAVL *pivot, RouterNodeAVL *fiuDr) {
	pivot->right = fiuDr->left; //subarborele sting al fiului din dreapta se ancoreaza in dreapta pivotului
	calculGE(pivot); //calcul GE al pivotului modificat
	fiuDr->left = pivot; // fiulDr urca cu un nivel mai sus,devine root local,iar de stinga lui se ancoreaza subarb pivotului
	calculGE(fiuDr);

	return fiuDr; //intoarce noul root local
}
RouterNodeAVL* rotireDublaStingaDreapta(RouterNodeAVL *pivot, RouterNodeAVL *fiuSt) {
	//rotire simpla stinga in fiul din stinga
	//dupa aceasta rotire in locul fiului stinga al pivotului va veni fiul din dreapta al fiului stinga al pivotului
	//update fiul stinga al pivotului cu noul root intors de functia de rotire simpla stinga
	pivot->left = rotireSimplaStinga(fiuSt, fiuSt->right);
	//recalcul GE pentru pivotul modificat
	calculGE(pivot);
	//rotire simpla dreapta in pivot propriu zis 
	//dupa aceasta rotire noul root local va deveni fiul sting al pivotului
	//salvam noul root local in fiu stinga
	fiuSt = rotireSimplaDreapta(pivot, pivot->left);
	//recalcul GE pentru noul subarbore
	calculGE(fiuSt);
	//returneaza noul root pentru subarborele modificat
	return fiuSt;
}
RouterNodeAVL *rotireDublaDreaptaStinga(RouterNodeAVL *pivot, RouterNodeAVL *fiuDr) {
	//aducerea dezechilibrului pe aceeasi directie
	//dupa rotirea astea fiul sting al fiului drept al pivotului va deveni noul root local
	//fiul drept al pivotului se updateaza cu noul root local
	pivot->right = rotireSimplaDreapta(fiuDr, fiuDr->left);
	//recalcul GE al pivotului
	calculGE(pivot);
	//rotirea simpla stinga in pivot , dupa rotirea fiul drept al pivotului va deveni noul root local
	//salvarea aceastui nou root
	fiuDr = rotireSimplaStinga(pivot, pivot->right);
	//recalcul GE al noului root local
	calculGE(fiuDr);
	//intoarcem noul root local
	return fiuDr;
}
RouterNodeAVL *inserareRouterAVL(RouterNodeAVL *root, Router *router, DataOwnership ownership,
	ResourcesManagement resManagement, int &errCode) {
	if (root) {
		//determinare pe care subarbore va ajunge noul nod
		if (strcmp(router->adress, root->router->adress) == -1)
			root->left = inserareRouterAVL(root->left, router, ownership, resManagement, errCode);
		else if (strcmp(router->adress, root->router->adress) == 1)
			root->right = inserareRouterAVL(root->right, router, ownership, resManagement, errCode);
		else
			errCode = INSERR_AVLREE_NODE_EXISTS_ALREADY;
	}
	else {
		//ultimul col recursiv creare nod
		root = makeRouteNodeAVL(router, ownership, resManagement);
	}
	//reintoarcere inapoi catre root incepind cu noul nod care se insereaza in structura
	//calcul GE pentru noul nod adaugat in arbore
	calculGE(root);
	//daca nodul curent pe drumul inapoi catre root are dezechilibru
	if (root->GE == 2)  //daca e dezechilibru puternic pe dreapta
		if (root->right->GE == -1)  //daca fiul dreapta are dezechilibru usor pe directia opusa,adica pe stinga
			root = rotireDublaDreaptaStinga(root, root->right);
		else //daca fiul dreapta nu are dezechilibru sau are un dezechilibru usor pe aceeasi directie,adica dreapta
			root = rotireSimplaStinga(root, root->right);
	else if (root->GE == -2) //daca e dezechilibru puternic pe stinga
		if (root->left->GE == 1) //daca fiul stinga are un dezechilibru usor pe directie opusa,adica pe dreapta
			root = rotireDublaStingaDreapta(root, root->left);
		else //daca fiul stinga nu are dezechilibru sau are dezechilibru usor pe aceeasi directie,adica stinga
			root = rotireSimplaDreapta(root, root->left);
	//intoarce noul subarbore echilibrat
	return root;
}
void inordineRouterAVLTree(RouterNodeAVL *root) {
	if (root) {
		inordineRouterAVLTree(root->left);
		printRouter(root->router);
		printf("\n GE for this node %d", root->GE);
		inordineRouterAVLTree(root->right);
	}
}
void destroyRouterAVLTree(RouterNodeAVL *root) {
	if (root) {
		freeRouter(root->router);
		if (root->ownership == DataOwnership::OWNER)
			free(root->router);
		destroyRouterAVLTree(root->left);
		destroyRouterAVLTree(root->right);
		free(root);
	}
}
RouterNodeAVL* loadRouters(const char *fName) {
	FILE *f = fopen(fName, "r");
	if (f) {
		char **tokens = (char**)malloc(6 * sizeof(char*));
		char buffer[100]; Router router; int columnNo;
		RouterNodeAVL *root = nullptr; int errorCode;
		while (!feof(f)) {
			fgets(buffer, 100, f); columnNo = 0;
			for (char *token = strtok(buffer, ","); token != nullptr; token = strtok(nullptr, ","))
				tokens[columnNo++] = _strdup(token);
			router.adress = tokens[0];
			router.username = tokens[1];
			router.password = tokens[2];
			router.connectionType = tokens[3];
			router.DNS = tokens[4];
			router.MAC_AdressClone = tokens[5];
			root = inserareRouterAVL(root, &router, DataOwnership::OWNER, ResourcesManagement::MOVE, errorCode);
			if (errorCode == INSERR_AVLREE_NODE_EXISTS_ALREADY)
				freeRouter(&router);
		}
		free(tokens);
		fclose(f);
		return root;
	}
	else {
		printf("\nUnnable to open file %s", fName);
		return nullptr;
	}
}
void freeRouterNode(RouterNodeAVL *node) {
	freeRouter(node->router);
	if (node->ownership == DataOwnership::OWNER)
		free(node->router);
	free(node);
}
RouterNodeAVL* getMinRouter(RouterNodeAVL *root) {
	if (!root) return nullptr;
	RouterNodeAVL *temp = root;
	while (temp->left)
		temp = temp->left;
	return temp;
}
void swapRouterNodes(RouterNodeAVL *routerNode1, RouterNodeAVL *routerNode2) {
	std::swap(routerNode1->GE, routerNode2->GE);
	std::swap(routerNode1->left, routerNode2->left);
	std::swap(routerNode1->right, routerNode2->right);
	std::swap(routerNode1->ownership, routerNode2->ownership);
	std::swap(routerNode1->router, routerNode2->router);
}
RouterNodeAVL* removeRouter(RouterNodeAVL *root, const char *adress) {
	if (!root) return root;
	//STEP 1 stergere normala a nodului cautat
	if (strcmp(adress, root->router->adress) == -1)
		root->left = removeRouter(root->left, adress);
	else if (strcmp(adress, root->router->adress) == 1)
		root->right = removeRouter(root->right, adress);
	else if (strcmp(adress, root->router->adress) == 0) {
		//cazul in care un singur fiu sau niciunul
		if (root->left == nullptr || root->right == nullptr) {
			RouterNodeAVL *child = root->left ? root->left : root->right;
			//daca nu are nici un fiu
			if (child == nullptr) {
				child = root;
				root = nullptr;
			}
			else {//are un fiu,interschimbam nodurile
				swapRouterNodes(root, child);
			}
			freeRouterNode(child); //stergem nodul copil
		}
		else { //cazul cind avem 2 fii
			//cauta cel mai mic nod din subarborele drept al nodului curent
			RouterNodeAVL *minNode = getMinRouter(root->right);
			//interschimba muta routerul din cel mai mic nod in nodul curent,
			//functia de mutare automat dezaloca datele din ruterul din parinte si le face null
			//de aceea trebuie de memorat adressa din cel mai mic nod inainte de mutarea datelor
			//trebuie de facut o copie a acestui string,deoarece daca nu se va face si root si fiul vor accesa aceeasi
			//zona de memorie pentru adresa
			//si dupa dezalocarea nodului copil,nodul parinte se va avea "dangling pointer" catre adresa
			adress = _strdup(minNode->router->adress);
			moveRouter(root->router, minNode->router);
			minNode->router->adress = adress; //ii reatribuim adresa inapoi pentru a putea fi gasit ulterior
			//sterge cel mai mic nod acum
			//adresa care a fost alocata mai sus nu e nevoie  sa fie dezalocata
			//deoarece in ultimul col recursiv ea se va dezaloca odata cu stergere routerului din nodul care vrem sa il stergem
			root->right = removeRouter(root->right, adress);
		}
	}
	//daca root e nullptr,adica arborele continea un singur nod,adica a intrat pe ramura in care root nu avea nici un fiu
	//atunci returnam null inapoi,pentru ca nu avem ce echilibra pentru acest subarbore
	//deja in urmatorul coll recursiv de dinainte se va face update la GE pentru nodul parinte al acestui root
	//pentru ca el a pierdut un nod
	if (!root) return root;
	//in cazul in care root a avut copii,atunci luind in cosiderare faptul ca de fapt tot timpul
	//se sterge un nod terminal , adica root a pierdut un copil trebuie de actualizat GE
	calculGE(root);
	//verificam daca in urma stergerii nodului nu au aparut dezechilibre
	if (root->GE == -2) //daca e dezechilibru puternic pe stinga
		if (root->left->GE == 1) //avem dezechilibru usor in fiul sting pe directia opusa -> dubla rotire
			root = rotireDublaStingaDreapta(root, root->left);
		else //avem dezechilibru usor pe aceeasi directie sau nu avem dezechilibru deloc -> rotire simpla
			root = rotireSimplaDreapta(root, root->left);
	else if (root->GE == 2) //daca e dezechilibru puternic pe dreapta
		if (root->right->GE == -1) //avem dezechilibru usor pe directia opusa -> facem ca directiile de dezechilibru sa coincida
			root = rotireDublaDreaptaStinga(root, root->right);
		else //avem dezechilibru usor pe acceasi directie sau nu avem dezechilibru deloc
			root = rotireSimplaStinga(root, root->right);
	//intoarcem noul subarbore echilibrat
	return root;
}

void initLogPlayer(LogPlayer *logPlayer)
{
	logPlayer->adress = nullptr;
	logPlayer->idPlayer = 0;
	logPlayer->port = 0;
	logPlayer->time = 0;
}
void freeLogPlayer(LogPlayer *logPlayer)
{
	free((void*)logPlayer->adress);
}
void copyLogPlayer(LogPlayer *destination, LogPlayer *source)
{
	freeLogPlayer(destination);
	destination->adress = _strdup(source->adress);
	destination->idPlayer = source->idPlayer;
	destination->port = source->port;
	destination->time = source->time;
}
void moveLogPlayer(LogPlayer *destination, LogPlayer *source)
{
	freeLogPlayer(destination);
	memcpy(destination, source, sizeof(LogPlayer));
	initLogPlayer(source);
}
void printLogPlayer(LogPlayer *logPlayer)
{
	printf("\nTime: %s,Adress: %s,Port: %d,ID Player: %d", ctime(&logPlayer->time),
		logPlayer->adress, logPlayer->port, logPlayer->idPlayer);
}

LogPlayerNodeLL *makeLogPlayerNodeLL(LogPlayer *logPlayer, ResourcesManagement resMng,
	DataOwnership ownership)
{
	LogPlayerNodeLL *node = (LogPlayerNodeLL*)malloc(sizeof(LogPlayerNodeLL));
	node->next = nullptr;
	node->ownership = ownership;
	if (ownership == DataOwnership::NOT_OWNER)
		node->logPlayer = logPlayer;
	if (ownership == DataOwnership::OWNER) {
		node->logPlayer = (LogPlayer*)malloc(sizeof(LogPlayer));
		initLogPlayer(node->logPlayer);
		if (resMng == ResourcesManagement::COPY)
			copyLogPlayer(node->logPlayer, logPlayer);
		if (resMng == ResourcesManagement::MOVE)
			moveLogPlayer(node->logPlayer, logPlayer);
	}
	return node;
}
void freeLogPlayerNodeLL(LogPlayerNodeLL *node) {	
	if (node->ownership == DataOwnership::OWNER) {
		freeLogPlayer(node->logPlayer); 
		free(node->logPlayer);
	}	
	free(node);
}

/*
	Linked List
*/
void insertLogPlayerLL(LogPlayerNodeLL **head, LogPlayer *logPlayer, ResourcesManagement resMng,
	DataOwnership ownership)
{
	if (*head == nullptr) {
		*head = makeLogPlayerNodeLL(logPlayer, resMng, ownership);
	}
	else {
		LogPlayerNodeLL *aux = *head;
		while (aux->next != nullptr)
			aux = aux->next;
		aux->next = makeLogPlayerNodeLL(logPlayer, resMng, ownership);
	}
}
void destroyLogPlayerLL(LogPlayerNodeLL **head) {
	LogPlayerNodeLL *temp;
	while (*head != nullptr) {
		temp = (*head)->next;
		freeLogPlayerNodeLL(*head);
		*head = temp;
	}
}
void findLogPlayerLL(LogPlayerNodeLL *head, LogPlayer *& logPlayer, int idPlayer) {
	logPlayer = nullptr;
	while (head != nullptr) {
		if (head->logPlayer->idPlayer == idPlayer) {
			logPlayer = head->logPlayer;
			break;
		}
		head = head->next;
	}
}
void removeLogPlayerLL(LogPlayerNodeLL **head, int idPlayer) {
	if (*head == nullptr || head == nullptr) return;
	LogPlayerNodeLL *prev, *current = nullptr;
	if ((*head)->logPlayer->idPlayer == idPlayer) {
		current = (*head)->next;
		freeLogPlayerNodeLL(*head);
		*head = current;
		return;
	}
	prev = *head;
	current = (*head)->next;
	while (current) {
		if (current->logPlayer->idPlayer == idPlayer) {
			prev->next = current->next;
			freeLogPlayerNodeLL(current);
			break;
		}
		prev = current;
		current = current->next;
	}
}
void printLogPLayerLL(LogPlayerNodeLL *head) {
	while (head) {
		printLogPlayer(head->logPlayer);
		head = head->next;
	}
}
LogPlayerNodeLL* loadLogPlayers(const char *fName) {
	FILE *f = fopen(fName, "r");
	if (f) {
		char **tokens = (char**)malloc(3 * sizeof(char*));
		LogPlayer player; char buffer[100]; int columnNo;
		LogPlayerNodeLL *head = nullptr;
		while (!feof(f)) {
			fgets(buffer, 100, f);
			columnNo = 0;
			for (char *token = strtok(buffer, ","); token != nullptr; token = strtok(nullptr, ","))
				tokens[columnNo++] = _strdup(token);
			player.adress = tokens[0];
			player.idPlayer = atoi(tokens[1]);
			player.port = atoi(tokens[2]);
			player.time = time(nullptr);
			insertLogPlayerLL(&head, &player, ResourcesManagement::MOVE, DataOwnership::OWNER);
			free(tokens[1]);
			free(tokens[2]);
		}
		free(tokens);
		fclose(f);
		return head;
	}
	else
		return nullptr;
}

/*
	Circular Linked List
*/
void insertLogPlayerLCL(LogPlayerNodeLL **head, LogPlayer *logPlayer, ResourcesManagement resMng,
	DataOwnership ownership) {
	if (*head == nullptr) { //daca lista e goala
		*head = makeLogPlayerNodeLL(logPlayer, resMng, ownership); //creaza cap nou
		(*head)->next = *head; //referinta ciclica
		return;
	}
	LogPlayerNodeLL *aux = *head;
	while (aux->next != *head) //cauta ultimul nod
		aux = aux->next;
	aux->next = makeLogPlayerNodeLL(logPlayer, resMng, ownership); //ancoreaza noul nod la final
	aux->next->next = *head; //seteaza acest nou nod sa duca pe head
}
void destroyLogPlayerLCL(LogPlayerNodeLL **head) {
	if (*head == nullptr) return; //daca lista e goala nu avem ce sterge
	LogPlayerNodeLL *aux = (*head)->next; //parcurgem lista incepind cu nodul imediat urmator headului
	LogPlayerNodeLL *temp;				//deoarece avem nevoie de acest head pentru a testa sfirsitul
										//liste in ciclul while
	while (aux->next != *head) { //cit timp mai sunt elemente in lista
		temp = aux->next; //salveaza adresa urmatorului nod
		freeLogPlayerNodeLL(aux); //sterge nodul curent !NOTA primul nod sters va fi al doilea nod din lista
		aux = temp; // deplasare pe urmatorul nod
	}
	if (aux == *head) { //in cazul in care lista are un singur nod
						//i.e. aux = head->next <=> aux = head -> referinta ciclica
		freeLogPlayerNodeLL(aux);
		*head = nullptr;
		return;
	}
	freeLogPlayerNodeLL(aux); //sterge ultimul nod din lista ramas
	freeLogPlayerNodeLL(*head); //sterge capul
	*head = nullptr; //fa lista goala
}
void findLogPlayerLCL(LogPlayerNodeLL *head, LogPlayer *&logPlayer, int idPlayer) {
	if (head == nullptr) {
		logPlayer = nullptr;
		return;
	}
	LogPlayerNodeLL *temp = head;
	while (temp->next != head) { //parcurge toata lista si cauta-l
		if (temp->logPlayer->idPlayer == idPlayer) {
			logPlayer = temp->logPlayer;
			return;
		}
		temp = temp->next;
	}
	//proceseaza ultimul nod sau head-ul daca lista contine un singur element
	if (temp->logPlayer->idPlayer == idPlayer) {
		logPlayer = temp->logPlayer;
		return;
	}
	logPlayer = nullptr; //daca nu a fost gasit semnalizeaza asta 
}
void removeLogPlayerLCL(LogPlayerNodeLL **head, int idPlayer) {
	if (*head == nullptr) return;
	LogPlayerNodeLL *current = *head, *prev = nullptr;
	//cauta nodul unde se afla log-ul cautat
	//cautarea incepe direct cu head-ul
	while (current->logPlayer->idPlayer != idPlayer) {
		if (current->next == *head) //daca am ajuns la ultimul nod si nu a fost gasit -> STOP
			return;
		prev = current; //memorare nodul anterior
		current = current->next; //avansare cu un nod inainte
	}
	//daca a fost gasit in head si e doar un singur nod
	if (current == *head && current->next == *head) {
		freeLogPlayerNodeLL(current); //sterge headul
		*head = nullptr; //semnalizeaza ca lista e goala
		return; // STOP
	}
	//daca a fost gasit in head si sunt mai multe noduri
	if (current == *head && current->next != *head) {
		while (current->next != *head) //cauta ultimul nod
			current = current->next;
		current->next = (*head)->next; //ultimul nod pointeaza catre noul head,i.e urmatorul dupa old head
		freeLogPlayerNodeLL(*head); //sterge head
		*head = current->next; //actualizeaza head cu noul head spre care pointeaza ultimul nod
		return; // STOP
	}
	//daca a fost gasit in alta parte
	prev->next = current->next; //comeaza referinta pe nodul urmator dupa nodul de sters
	freeLogPlayerNodeLL(current); //sterge acest nod
}
void printLogPlayerLCL(LogPlayerNodeLL *head) {
	if (!head) {
		printf("Lista este goala!");
		return;
	}
	LogPlayerNodeLL *aux = head;
	while (aux->next != head) {
		printLogPlayer(aux->logPlayer);
		aux = aux->next;
	}
	printLogPlayer(aux->logPlayer);
}
LogPlayerNodeLL* loadLogPlayersLCL(const char *fName) {
	FILE *f = fopen(fName, "r");
	if (f) {
		char **tokens = (char**)malloc(3 * sizeof(char*));
		LogPlayer player; char buffer[100]; int columnNo;
		LogPlayerNodeLL *head = nullptr;
		while (!feof(f)) {
			fgets(buffer, 100, f);
			columnNo = 0;
			for (char *token = strtok(buffer, ","); token != nullptr; token = strtok(nullptr, ","))
				tokens[columnNo++] = _strdup(token);
			player.adress = tokens[0];
			player.idPlayer = atoi(tokens[1]);
			player.port = atoi(tokens[2]);
			player.time = time(nullptr);
			insertLogPlayerLCL(&head, &player, ResourcesManagement::MOVE, DataOwnership::OWNER);
			free(tokens[1]);
			free(tokens[2]);
		}
		free(tokens);
		fclose(f);
		return head;
	}
	else
		return nullptr;
}

/*
	Doubly Linked List
	Utilitati suportate:
		* initializare lista
		* inserare , cu crearea unei liste noi daca aceasta e goala
		* iterare
		* cautare
		* stergere
		* for each cu executia unui handler pe fiecare element
		* serializare lista in fisier binar
		* deserializare lista din fisier binar
		* distrugere lista
*/
//Functie pentru deplasarea iteratorului catre noua pozitie
inline void moveIteratorDLL(LogPlayerNodeDLL *&iterator, DLLIterationDirection dirrection) {
	if (iterator == nullptr) return; //daca iteratorul nu mai este valid
	if (dirrection == DLLIterationDirection::FORWARD) //head -> tail , i.e next node
		iterator = iterator->next;
	if (dirrection == DLLIterationDirection::BACKWARD) //tail -> head , i.e prev node
		iterator = iterator->prev;
}
//Functia pentru initializarea iteratorului
inline void initIteratorDLL(const LogPlayerDoublyLL *dll, LogPlayerNodeDLL *&iterator) {
	if (dll->iterateDirrection == DLLIterationDirection::FORWARD) //daca incepem cu head
		iterator = dll->head;
	if (dll->iterateDirrection == DLLIterationDirection::BACKWARD) //daca incepem cu tail
		iterator = dll->tail;
}
//Functia pentru parcurgerea DLL,cu salvarea ulterioara a nodului curent parcurs
//WARNING functia nu e reentranta
LogPlayerNodeDLL* iterateDLL(const LogPlayerDoublyLL *dll) {
	static const LogPlayerDoublyLL *doublyLL = nullptr; //DLL intern
	static LogPlayerNodeDLL *iterator = nullptr; //nodul curent
	static DLLIterationDirection dirrection = DLLIterationDirection::FORWARD;//Default FORWARD 
																			 //daca primim o lista noua,reinitializam bufferele si o parsam pe aceasta
	if (dll) {
		doublyLL = dll; //initializam DLL intern cu lista de parsat
		dirrection = dll->iterateDirrection; //initializam pozitia de unde incepem
		initIteratorDLL(doublyLL, iterator); //initializare iterator
		if (iterator != nullptr) //daca lista nu e goala	
			return iterator; //intoarce logul curent
		else
			return nullptr; //semnalizeaza ca nu mai sunt elemente in lista
	}
	//daca primim nullptr inseamna ca lucram cu lista care o avem in buffer de la apelurile precendente
	else {
		moveIteratorDLL(iterator, dirrection); //muta iteratorul
		if (iterator != nullptr) //daca au mai ramas elemente in lista
			return iterator;
		else
			return nullptr; //semnalizeaza ca nu mai sunt elemente in lista
	}
}
//Functie pentru crearea unui nod DLL default
LogPlayerNodeDLL* makeLogPlayerNodeDLL(LogPlayer *logPlayer, ResourcesManagement resMng,
	DataOwnership ownership) {
	LogPlayerNodeDLL *node = (LogPlayerNodeDLL*)malloc(sizeof(LogPlayerNodeDLL));
	node->next = node->prev = nullptr;
	node->owenership = ownership;
	if (ownership == DataOwnership::NOT_OWNER)
		node->logPlayer = logPlayer;
	if (ownership == DataOwnership::OWNER) {
		node->logPlayer = (LogPlayer*)malloc(sizeof(LogPlayer));
		initLogPlayer(node->logPlayer);
		if (resMng == ResourcesManagement::COPY)
			copyLogPlayer(node->logPlayer, logPlayer);
		if (resMng == ResourcesManagement::MOVE)
			moveLogPlayer(node->logPlayer, logPlayer);
	}
	return node;
}
//Functie pentru stergerea unui nod DLL impreuna cu datele pe care le detine
void freeLogPlayerNodeDLL(LogPlayerNodeDLL *node) {
	freeLogPlayer(node->logPlayer);
	if (node->owenership == DataOwnership::OWNER)
		free(node->logPlayer);
	free(node);
}
//Functia pentru inserarea in DLL
void insertLogPlayerNodeDLL(LogPlayerDoublyLL *dll, LogPlayer *logPlayer, ResourcesManagement resMng,
	DataOwnership ownership) {
	//creaza un nod default
	LogPlayerNodeDLL *new_node = makeLogPlayerNodeDLL(logPlayer, resMng, ownership);
	if (dll->head == nullptr) { //daca lista e goala incepem sa o construim
		dll->head = dll->tail = new_node;
	}
	else { //daca nu e goala
		if (dll->insertPosition == DLLInsertPosition::HEAD) { //daca se doreste adaugarea la inceput
			//legam noul nod de head vechi
			new_node->next = dll->head;
			dll->head->prev = new_node;
			//update noul head
			dll->head = new_node;
		}
		if (dll->insertPosition == DLLInsertPosition::TAIL) { //daca se doreste adaugarea la sfirsit
			//legam noul nod de coada
			dll->tail->next = new_node;
			new_node->prev = dll->tail;
			//update noul tail
			dll->tail = new_node;
		}
	}
}
//Functia pentru initializarea DLL
void initDLL(LogPlayerDoublyLL *dll, DLLInsertPosition ins, DLLIterationDirection dir)
{
	dll->head = dll->tail = nullptr;
	dll->insertPosition = ins;
	dll->iterateDirrection = dir;
}
//Functia pentru distrugerea DLL
void destroyLogPlayerDLL(LogPlayerDoublyLL *dll) {
	//!Memory Optimization folosim ca nod temporar pentru a salva vecinul coada listei
	while (dll->head) {
		dll->tail = dll->head->next; //salvarea urmatorul nod
		freeLogPlayerNodeDLL(dll->head); //sterge capul listei
		dll->head = dll->tail; //update capul listei cu urmatorul nod salvat anterior
	}
	//resetare lista
	dll->head = dll->tail = nullptr;
}
//Functia pentru cautarea unui log in DLL
void findLogPlayerDLL(LogPlayerDoublyLL *dll, LogPlayer *&logPlayer, int idPlayer) {
	assert(dll != nullptr); //asigra ca lista nu e null,pentru ca functia de iterare sa poate lucra pe o lista noua
	//oricum daca nu faceam assert functia de iterare,in cazul in care a iterat toata lista anterioara din bufferul ei
	//iteratorul va sta pe null si pina nu se va introduce o noua lista, i.e diferita de nulll
	//iteratorul nu se va reseta,aici pentru precautie ne asiguram ca totusi lucram sigur cu o lista noua
	for (LogPlayerNodeDLL *logNode = iterateDLL(dll); logNode != nullptr; logNode = iterateDLL(nullptr))
		if (logNode->logPlayer->idPlayer == idPlayer) { //daca a fost gasit
			logPlayer = logNode->logPlayer;
			return;
		}
	logPlayer = nullptr; //anuntat caller-ului ca log-ul nu a fost gasit
}
//Functia pentru stergerea unui log din DLL
bool removeLogPlayerDLL(LogPlayerDoublyLL *dll, int idPlayer) {
	LogPlayerNodeDLL *logNode;
	for (logNode = iterateDLL(dll); logNode != nullptr; logNode = iterateDLL(nullptr))
		if (logNode->logPlayer->idPlayer == idPlayer) //daca l-am gasit
			break;
	if (!logNode) return false; //nodul nu a fost gasit
	//daca stergem head-ul
	//cind se va ajunge la ultimul nod,head si tail automat se vor reseta
	//deoarece si nodul de sters va fi egal atit cu head cit si cu tail
	if (logNode == dll->head)
		dll->head = logNode->next; //head avanseaza cu o pizitie inainte , cind va fi doar un singur nod head va deveni null
	//daca stergem tail
	if (logNode == dll->tail)
		dll->tail = logNode->prev; //tail se avanseaza cu o pozitie inapoi , cind va ramane un singur nod tail va deveni null
	if (logNode->next) //deplasare "safe" catre nodul next si refacere legatura pentru dinsul
		logNode->next->prev = logNode->prev;
	if (logNode->prev) //deplasare "safe" catre nodul prev si refacere legatura pentru dinsul
		logNode->prev->next = logNode->next;
	//stergerea nodului propriuzisa
	freeLogPlayerNodeDLL(logNode);
	return true;
}
//Functie pentru traversarea listei cu aplicarea unui handler pe fiecare element(log)
void for_each_DLL(const LogPlayerDoublyLL *dll, dll_elem_handler handler, void *handler_args) {
	for (LogPlayerNodeDLL *logNode = iterateDLL(dll); logNode != nullptr; logNode = iterateDLL(nullptr))
		handler(logNode->logPlayer, handler_args);
}
//Functie pentru salvarea DLL intr-un fisier binar
int saveLogsDLLToBinaryFile(LogPlayerDoublyLL *dll, const char *fName) {
	FILE *fBin = fopen(fName, "wb");
	if (fBin) {
		size_t sizeOfAdressString = 0;
		for (LogPlayerNodeDLL *log = iterateDLL(dll); log != nullptr; log = iterateDLL(nullptr)) {
			sizeOfAdressString = strlen(log->logPlayer->adress);
			fwrite(reinterpret_cast<const char*>(&sizeOfAdressString), sizeof(size_t), 1, fBin);
			fwrite(log->logPlayer->adress, sizeOfAdressString, 1, fBin);
			fwrite(reinterpret_cast<const char*>(&log->logPlayer->idPlayer), sizeof(int), 1, fBin);
			fwrite(reinterpret_cast<const char*>(&log->logPlayer->port), sizeof(int), 1, fBin);
			fwrite(reinterpret_cast<const char*>(&log->logPlayer->time), sizeof(time_t), 1, fBin);
		}
		fclose(fBin);
		return 1;
	}
	else
		return -1;
}
//Functie pentru a verifica daca nu s-a ajuns la sfirsitul fisierului
bool CheckFileEnd(FILE *fp)
{
	bool res;
	long currentOffset = ftell(fp);

	fseek(fp, 0, SEEK_END);

	if (currentOffset >= ftell(fp))
		res = true;
	else
		res = false;

	fseek(fp, currentOffset, SEEK_SET);

	return res;
}
//Functie pentru citirea logurilor in DLL
void loadLogsDLLFromBynaryFile(const char *fName, LogPlayerDoublyLL *dll) {
	FILE *f = fopen(fName, "rb");
	if (f) {
		size_t sizeOfAdressString = 0;
		LogPlayer log;
		while (!CheckFileEnd(f)) {
			fread(&sizeOfAdressString, sizeof(size_t), 1, f);
			log.adress = (char*)malloc((sizeOfAdressString + 1) * sizeof(char));
			fread((void*)log.adress, sizeOfAdressString, 1, f);
			const_cast<char*>(log.adress)[sizeOfAdressString] = '\0';
			fread(&log.idPlayer, sizeof(int), 1, f);
			fread(&log.port, sizeof(int), 1, f);
			fread(&log.time, sizeof(time_t), 1, f);
			insertLogPlayerNodeDLL(dll, &log, ResourcesManagement::MOVE, DataOwnership::OWNER);
		}
		fclose(f);
	}
}

/*
	 --> Doubly Linked Circular List <--
	Utilitati suportate:
		* inserare lista
		* iterare lista
		* for_each lista
		* cautare
		* stergere
		* distrugere
		* deserializare lista din fisier binar
*/

//Functie pentru inserarea unui nou nod,daca head este null lista se creaza 
void insertLogDLCL(LogPlayerDoublyLL *dlcl, LogPlayer *log, DataOwnership ownership, ResourcesManagement resMng) {
	LogPlayerNodeDLL * node = makeLogPlayerNodeDLL(log, resMng, ownership);
	if (dlcl->head == nullptr) { //daca lista e goala
		dlcl->head = dlcl->tail = node; //initializeaza lista
		dlcl->head->next = dlcl->head->prev = dlcl->head; //referinta ciclica pe sine insusi
	}
	else { //altfel daca lista nu e goala
		if (dlcl->insertPosition == DLLInsertPosition::HEAD) {
			//legam noul nod cu head
			node->next = dlcl->head;
			dlcl->head->prev = node;
			//legam noul nod cu tail
			node->prev = dlcl->tail;
			dlcl->tail->next = node;
			//update head
			dlcl->head = node;
		}
		if (dlcl->insertPosition == DLLInsertPosition::TAIL) {
			//legam noul nod de tail
			node->prev = dlcl->tail;
			dlcl->tail->next = node;
			//legam noul nod de head
			node->next = dlcl->head;
			dlcl->head->prev = node;
			//update tail
			dlcl->tail = node;
		}
	}
}
//Functie pentru iterarea DLCL
//WARNING Functia nu e reentranta,contine buffere statice in interiorul sau
//Returneaza null in cazul in care depisteaza sfirsitul listei
LogPlayerNodeDLL* iterateDLCL(LogPlayerDoublyLL *doubly_linked_circular_list) {
	static LogPlayerDoublyLL *dlcl = nullptr; //lista care se itereaza
	static LogPlayerNodeDLL *iterator = nullptr; //iterator

	if (doubly_linked_circular_list) { //cazul in care iteram o lista noua
		dlcl = doubly_linked_circular_list; //initializare buffer lista
		if (dlcl->head) { //daca lista nu e goala
			INIT_ITER_DLCL(iterator, dlcl); //initializare iterator
			return iterator; //intoarce nodul curent, i.e tail sau head
		}
		else
			return nullptr; //altfel semnalizeaza ca nu sunt elemente in lista
	}
	else { //daca lucram pe acceasi lista
		MOVE_ITER_DLCL(iterator, dlcl);
		if (TEST_EODLCL(iterator, dlcl)) //daca nu am ajuns inapoi la inceputul listei
			return iterator;
		else
			return nullptr; //am ajuns inapoi in head,lista s-a terminat
	}
}
//Functie care travereseaza DLCL si aplica handlerul pentru fiecare element
void foreachDLCL(LogPlayerDoublyLL *dlcl, dll_elem_handler handler, void* handler_args) {
	for (LogPlayerNodeDLL *logNode = iterateDLCL(dlcl); logNode != nullptr; logNode = iterateDLCL(nullptr))
		handler(logNode->logPlayer, handler_args);
}
//Functie pentru cautarea unui log in DLCL,returneaza null daca nu gaseste
LogPlayer* findLogDLCL(LogPlayerDoublyLL *dlcl, int idPlayer) {
	for (LogPlayerNodeDLL *node = iterateDLCL(dlcl); node != nullptr; node = iterateDLCL(nullptr))
		if (node->logPlayer->idPlayer == idPlayer)
			return node->logPlayer;
	return nullptr;
}
//Functie pentru stergerea unui log in DLCL
bool removeLogDLCL(LogPlayerDoublyLL *dlcl, int idPlayer) {
	LogPlayerNodeDLL *node;
	for (node = iterateDLCL(dlcl); node != nullptr; node = iterateDLCL(nullptr))
		if (node->logPlayer->idPlayer == idPlayer)
			break;
	if (!node) return false; //daca nu l-am gasit
	//daca a mai ramas un singur nod
	if (node == dlcl->head && node == dlcl->tail) {
		freeLogPlayerNodeDLL(node);
		dlcl->head = dlcl->tail = nullptr;
		return true;
	}
	//daca nodul de sters e head
	if (node == dlcl->head) {
		dlcl->head = node->next; //head avanseaza cu o pozitie inainte
		//update legatura dintre tail cu noul head
		dlcl->tail->next = dlcl->head;
		dlcl->head->prev = dlcl->tail;
	}
	//daca nodul de sters e tail
	if (node == dlcl->tail) {
		dlcl->tail = node->prev; //tail avanseaza cu o pozitie inapoi
		//update legatura head cu noul tail
		dlcl->head->prev = dlcl->tail;
		dlcl->tail->next = dlcl->head;
	}
	//daca e un nod arbitrar
	node->prev->next = node->next;
	node->next->prev = node->prev;
	//stergearea nodului
	freeLogPlayerNodeDLL(node);
	return true;
}
//Functie pentru distrugerea DLCL,cu eliberarea tuturor resurselor
void destroyDLCL(LogPlayerDoublyLL *dlcl) {
	if (!dlcl->head) return;
	LogPlayerNodeDLL *tempHead = dlcl->head, *temp; //salveaza adresa vechiului head pentru ca ulterior sa poata fi testat capatul listei
	while (dlcl->head->next != tempHead) { //cit timp nu am ajuns inapoi in head
		temp = dlcl->head->next; //salveaza urmatorul nod
		freeLogPlayerNodeDLL(dlcl->head); //stergea head curent
		dlcl->head = temp; //avanseaza head cu o pozitie inainte
	}
	//dupa toate iteratiile head va pointa exact pe ultimul nod sau daca lista contine un singur nod va pointa pe sine insusi
	//stergere ultimul nod
	freeLogPlayerNodeDLL(dlcl->head);
	//reinitializeaza lista pentru a marca ca e goala
	INIT_DLCL(dlcl, dlcl->insertPosition, dlcl->iterateDirrection);
}
//Functie pentru deserializarea DLCL din fisier binar
void loadLogsDLCLFromBynaryFile(const char *fName, LogPlayerDoublyLL *dll) {
	FILE *f = fopen(fName, "rb");
	if (f) {
		size_t sizeOfAdressString = 0;
		LogPlayer log;
		while (!CheckFileEnd(f)) {
			fread(&sizeOfAdressString, sizeof(size_t), 1, f);
			log.adress = (char*)malloc((sizeOfAdressString + 1) * sizeof(char));
			fread((void*)log.adress, sizeOfAdressString, 1, f);
			const_cast<char*>(log.adress)[sizeOfAdressString] = '\0';
			fread(&log.idPlayer, sizeof(int), 1, f);
			fread(&log.port, sizeof(int), 1, f);
			fread(&log.time, sizeof(time_t), 1, f);
			insertLogDLCL(dll, &log, DataOwnership::OWNER, ResourcesManagement::MOVE);
		}
		fclose(f);
	}
}

/*
		--> Coada implementata prin lista inlantuita simpla <--
	Utilitati suportate:
		* adaugare
		* extragere
		* distrugere
		* deserializare din fisier binar
*/

//Functie pentru adaugarea unui log in capatul cozii
void putQueue(LogPlayerQueue *queue, LogPlayer *log, DataOwnership ownership, ResourcesManagement resMng)
{
	assert(queue != nullptr);
	assert(log != nullptr);
	LogPlayerNodeLL *node = makeLogPlayerNodeLL(log, resMng, ownership);
	if (!queue->tail) { //coada este goala
		queue->head = queue->tail = node; //adauga nod in capatul cozii,totodata initializind-o
	}
	else { //coada nu e goala
		queue->tail->next = node; //adauga nod in capatul cozii
		queue->tail = node; //actualizeaza capatul cozii cu ultimul nod adaugat
	}
}
//Functie pentru extragerea unui log de la inceputul cozii
//Functia intoarce doar informatia utila
//Flagul isShared indica asupra faptului daca headul este proprietarul logului sau nu
//Daca e true -> atunci callerul trebuie sa fie atent la stergere,deoarece nu el este posesorul acestei resurse
//Daca e false -> atunci atentioneaza callerul ca de acum incolo el este proprietarul si e responsabil de dezalocarea resursei
LogPlayer* getQueue(LogPlayerQueue *queue, bool &isShared) {
	assert(queue != nullptr);
	LogPlayer *log = nullptr; LogPlayerNodeLL *toDelete = nullptr;
	if (queue->head) { //daca sunt date la inceputul cozii
		log = queue->head->logPlayer; //extrage informatia utila
		isShared = queue->head->ownership == DataOwnership::OWNER ? false : true; //atentioneaza caller
		toDelete = queue->head; //memoreaza nodul care urmeaza sa fie dezalocat
		queue->head = queue->head->next; //avanseaza head cu o pozitie inainte
		free(toDelete);
	}
	//daca dupa extragere inceputul cozii a trecut de capatul ei,reseteaza coada
	if (queue->head == nullptr)
		queue->tail = nullptr;
	return log; //intoarna apelatorului log,daca coada era goala se intoarce null si il atentioneaza ca coada e goala
}
//Functie pentru distrugerea cozii
void destroyQueue(LogPlayerQueue *queue) {
	assert(queue != nullptr);
	LogPlayer *log = nullptr;
	bool isShared;
	while ((log = getQueue(queue, isShared)) != nullptr) { //sterge head queue la fiecare iteratie
		if (!isShared) //daca nodul era posesorul resursei atunci noi suntem responsabili sa o dezalocam
			freeLogPlayer(log);
	}
}
//Functie pentru deserializarea cozii din fisier binar
void loadLogsQueueFromBynaryFile(const char *fName, LogPlayerQueue *queue) {
	FILE *f = fopen(fName, "rb");
	if (f) {
		size_t sizeOfAdressString = 0;
		LogPlayer log;
		while (!CheckFileEnd(f)) {
			fread(&sizeOfAdressString, sizeof(size_t), 1, f);
			log.adress = (char*)malloc((sizeOfAdressString + 1) * sizeof(char));
			fread((void*)log.adress, sizeOfAdressString, 1, f);
			const_cast<char*>(log.adress)[sizeOfAdressString] = '\0';
			fread(&log.idPlayer, sizeof(int), 1, f);
			fread(&log.port, sizeof(int), 1, f);
			fread(&log.time, sizeof(time_t), 1, f);
			putQueue(queue, &log, DataOwnership::OWNER, ResourcesManagement::MOVE);
		}
		fclose(f);
	}
}

/*
		--> Coada implementata prin masiv <--
	Utilitati suportate:
		* adaugare
		* extragere
		* distrugere
*/

//Functie pentru initializarea cozii cu capacitatea specificata
void initQueueArrImpl(LogPlayerQueueArrImpl *queue, size_t capacity, DataOwnership ownership) {
	queue->capacity = capacity;
	queue->front = 0;
	queue->rear = capacity - 1; //avem nevoie de asta la inserare pentru comutarea ciclica a indexului de sfirsit
	queue->size = 0;
	queue->logArr = (LogPlayer**)malloc(capacity * sizeof(LogPlayer*));
	memset(queue->logArr, 0, capacity);
	queue->owner = ownership;
}
//Functie pentru adaugarea in coada a unui log
bool putQueueArrImpl(LogPlayerQueueArrImpl *queue, LogPlayer *log, ResourcesManagement resMng) {
	if (queue->size == queue->capacity) return false; //coada este plina
	queue->rear = (queue->rear + 1) % queue->capacity; //deplaseaza ciclic capatul cozii
	if (queue->owner == DataOwnership::NOT_OWNER)
		queue->logArr[queue->rear] = log;
	if (queue->owner == DataOwnership::OWNER) {
		queue->logArr[queue->rear] = (LogPlayer*)malloc(sizeof(LogPlayer));
		initLogPlayer(queue->logArr[queue->rear]);
		if (resMng == ResourcesManagement::COPY)
			copyLogPlayer(queue->logArr[queue->rear], log);
		if (resMng == ResourcesManagement::MOVE)
			moveLogPlayer(queue->logArr[queue->rear], log);
	}
	queue->size = queue->size + 1;
	return true;
}
//Functie pentru extragerea din coada a unui log
//isShared indica asupra faptui daca coada poseda sau nu resursa de log
//isShared = true -> apelatorul trebuie sa ia grija la dezalocarea resursei,nu el e owner
//isShared = false -> apelatorul este responsabil de dezalocarea resursei,deoarece de acum in colo el e owner
LogPlayer* getQueueArrImpl(LogPlayerQueueArrImpl *queue, bool &isShared) {
	if (queue->size == 0) return nullptr; //daca coada e goala -> semnalizeaza
	LogPlayer *log = queue->logArr[queue->front]; //salvare nod ce trebuie returnat
	isShared = queue->owner == DataOwnership::OWNER ? false : true; //anunta apelator
	queue->logArr[queue->front] = nullptr; //marcheaza pozitia ca fiind libera
	queue->front = (queue->front + 1) % queue->capacity; //deplaseaza head inainte ciclic
	queue->size = queue->size - 1;
	return log;
}
//Functie pentru distrugerea cozii
void destroyQueueArrImpl(LogPlayerQueueArrImpl *queue) {
	LogPlayer *temp; bool isShared;
	while ((temp = getQueueArrImpl(queue, isShared)) != nullptr) {
		if (!isShared) {
			printf("\nStergere: ");
			printLogPlayer(temp);
			freeLogPlayer(temp);
			free(temp);
		}
	}
	free(queue->logArr);
	queue->front = queue->rear = queue->size = queue->capacity = 0;
	queue->logArr = nullptr;
	queue->owner = DataOwnership::NOT_OWNER;
}

/*
		--> Stack Array Implementation <--
	Utilitati suportate:
		* adaugare
		* stergere
		* verificare head
*/

//Functie pentru adaugarea unui log in capul stack
inline bool pushStackArrImpl(LogPlayerStackArrImpl *stack, LogPlayer *log) {
	if (IS_FULL_STACK_ARR_IMPL(stack)) return false;
	return (stack->players[++stack->top] = log) == nullptr;
}
//Functie pentru extragerea unui log din head stack
inline LogPlayer *popStackArrImpl(LogPlayerStackArrImpl *stack) {
	if (IS_EMPTY_STACK_ARR_IMPL(stack)) return nullptr;
	return stack->players[stack->top--];
}
//Functie pentru verificarea informatiei din head
//WARNING informatia nu se extrage,functia are rol de a consulta
inline LogPlayer *peekStackArrImpl(LogPlayerStackArrImpl *stack) {
	if (IS_EMPTY_STACK_ARR_IMPL(stack)) return nullptr;
	return stack->players[stack->top];
}

/*
		--> Stack LL implementation <--
	Utilitati suportate:
		* adaugare
		* stergere
		* verificare head
*/

//Functie pentru adaugarea unui log in capul stack
inline void pushStack(LogPlayerNodeLL **head, LogPlayer *log, ResourcesManagement resMng, DataOwnership ownership) {
	LogPlayerNodeLL *nou = makeLogPlayerNodeLL(log, resMng, ownership);
	if (!*head) { //creare stack nou
		*head = nou;
	}
	else {
		nou->next = *head; //legare nodul nou de head
		*head = nou; //head avanseaza inapoi cu o pozitie, i.e stackul creste in sus
	}
}
//Functie pentru stergerea unui log din capul stack
inline LogPlayer* popStack(LogPlayerNodeLL **head, bool &isShared) {
	if (!*head) return nullptr; //stack e gol
	LogPlayer * log = (*(*head)).logPlayer; //extrage info utila din head
	isShared = (*(*head)).ownership == DataOwnership::OWNER ? false : true; //determinare daca e "shared resource"
	LogPlayerNodeLL *toDelete = *head; //memorare nod de dezalocat
	*head = (*(*head)).next; //head avanseaza,i.e stack scade
	free(toDelete); //dezaloca old head
	return log;
}
//This method is similar to the pop method, but peek does not modify the Stack.
//This method is an O(1) operation.
inline LogPlayer *peekStack(LogPlayerNodeLL *head) {
	return head ? head->logPlayer : nullptr;
}

/*
		--> Tabela de dispersie : evitarea coliziunilor prin rehashing <--
	Utilitati suportate:
		* inserare
		* cautare
			* toate elementele
			* toate elementele care corespund unui predicat
		* stergere
			* primul element gasit
			* toate elementele gasite
*/

//FORWARD DECLARATION
bool rehash(LogPlayerHashTable_Rehashing *hash_table);

int isPrime(int n)
{
	for (int i = 2; i * i <= n; i++)
		if (n % i == 0) return 0;
	return 1;
}
int nextPrime(int n)
{
	while (!isPrime(++n));
	return n;
}

//Functie pentru initializarea tabelei de dispersie
bool initLogHashTable(LogPlayerHashTable_Rehashing *hash_table, size_t capacity, size_t maximSize) {
	hash_table->size = 0;
	hash_table->maximSize = maximSize;
	hash_table->capacity = capacity;
	hash_table->players = (LogPlayer**)calloc(capacity, sizeof(LogPlayer*));
	if (!hash_table->players) return false; //FAIL
	return true; //SUCCES
}
//Functie pentru adaugarea unui log nou in tabela de dispersie
bool insertLogHashTable(LogPlayerHashTable_Rehashing *hash_table, LogPlayer *log) {

	unsigned int whenToRehash = hash_table->capacity * hash_table->loadFactor;
	if (whenToRehash == hash_table->size) //daca nr elemente constituie 75% din capacitatea tab
		if (!rehash(hash_table)) //rehash -> creste capacitatea tabelei catre urmatorul numar prim
			return false; //in caza de FAIL -> semnaleaza

	int hashVal = hash_table->firsh_hash(log->adress) % hash_table->capacity; //calcul pozitie hash
	int stepSize = hash_table->second_hash(log->adress); //calcul dimensiune pas

	while (hash_table->players[hashVal] != nullptr) { //cit timp nu am gasit o pozitie libera
		hashVal += stepSize; //avanseaza pozitie hash cu step size
		hashVal %= hash_table->capacity; //intoarcere catre inceput daca am avansat peste capacitatea hash table
	}
	hash_table->players[hashVal] = log; //insereaza log in pozitia calculata
	hash_table->size++; //creste numarul de elemente din tabela
	return true; //SUCCES
}
//Functie pentru rehash daca numarul de elemente este peste load factor
bool rehash(LogPlayerHashTable_Rehashing *hash_table) {
	if (hash_table->size >= hash_table->maximSize) {
		printf("\nTabela a atins dimensiunea maxima si nu mai poate sa creasca");
		return false; //FAIL
	}

	LogPlayerHashTable_Rehashing tempBuffer; //declara buffer
	//initializeazal cu o capacitate egala cu urmatorul numarul prim dupa dubla capacitate veche 
	//si un numar maxim cu hash table primit
	//daca primim eroare la initializare , i.e fail la alocare vector nou
	if (initLogHashTable(&tempBuffer, nextPrime(2 * hash_table->capacity), hash_table->maximSize) == false) {
		printf("\nEroare la rehashing in timpul alocarii memoriei pentru noul masiv");
		return false;
	}
	//insereaza playerii din hash table in buffer temporar
	for (size_t i = 0; i < hash_table->capacity; i++) {
		if (hash_table->players[i] != nullptr)
			insertLogHashTable(&tempBuffer, hash_table->players[i]);
	}

	//dezalocare noul vector
	DESTROY_HASH_TABLE_REHASHING(hash_table);
	//muta datele din buffer in hash table
	hash_table->capacity = tempBuffer.capacity;
	hash_table->players = tempBuffer.players;
	hash_table->size = tempBuffer.size;

	return true; //SUCCES
}
//Functie pentru stergerea unui log din tabela de dispersie
//Daca all = true -> sterge toate coincidentele
//Daca e false -> doar prima coincidenta
LogPlayer *removeLogHashTable(LogPlayerHashTable_Rehashing *hash_table, const char *key, bool all) {
	int hash = hash_table->firsh_hash(key) % hash_table->capacity; //determinare hash
	int stepSize = hash_table->second_hash(key); //determinare pas

	LogPlayer *temp = nullptr; //elementul inca nu a fost gasit

	while (hash_table->players[hash] != nullptr) { //pina nu a fost gasit
		if (strcmp(hash_table->players[hash]->adress, key) == 0) { //log a fost gasit
			temp = hash_table->players[hash]; //salvare element
			hash_table->players[hash] = nullptr; //marcheaza pozitia libera
			hash_table->size--; //micsoreaza dimensiunea
			if (all == false) //daca se doreste doar primul element
				break; //stop cautare
		}
		//daca nu nu conincide
		hash += stepSize; //cauta in urmatorul cluster,i.e urmatoarea pozitie unde putea fi inserat
		hash %= hash_table->capacity; //intoarcere catre inceput
	}

	return temp; //elementul nu a fost gasit
}
//Functie pentru cautarea unor loguri din tabela de dispersie care corespund predicatului primit
//Daca predicatul e null atunci se incarca toate aparitiile
void findLogHashTable(LogPlayerHashTable_Rehashing *hash_table, std::list<LogPlayer*>& occurences, const char *key,
	log_predicate *predicate, void *args) {
	occurences.clear(); //curata lista
	int hash = hash_table->firsh_hash(key) % hash_table->capacity;
	int stepSize = hash_table->second_hash(key);

	while (hash_table->players[hash] != nullptr) { //daca l-am gasit din prima,sau daca il gasim in clusterele ulterioare
		if (predicate && (*predicate)(hash_table->players[hash], args)) //daca am primit predicat si satisfica conditia
			occurences.push_front(hash_table->players[hash]);
		if (predicate == nullptr) //daca nu avem predicat adaugam toate aparitiile
			occurences.push_front(hash_table->players[hash]);
		hash += stepSize;
		hash %= hash_table->capacity;
	}
}

/*
		--> Tabela de dispersie : evitarea coliziunilor prin chaining <--
	Utilitati suportate:
		* inserare
		* cautare
		* stergere
		* distrugere
		* for each
			* toata tabela
			* doar un cluster specific
*/

//Iterator
struct hash_table_ch_iter {
	size_t arr_pos;
	LogPlayerNodeLL *current;
};
#define INIT_HT_CH_IT(iterator,hash_table)\
	(iterator)->arr_pos=0;\
	(iterator)->current= (hash_table)->arrayPlayers[0];


//Functie pentru initializarea hash table,returneaza true in caz de succes,false invers
//DEFAULT VALUES -> loadFactor = 0.75;
bool initLogHashTable(LogPlayerHashTable_Chaining *hash_table, size_t capacity, size_t maximSize)
{
	assert(hash_table != nullptr);
	assert(capacity > 0 && maximSize > 0);
	hash_table->arrayPlayers = (LogPlayerNodeLL**)calloc(capacity, sizeof(LogPlayerNodeLL*));
	if (!hash_table->arrayPlayers) return false; //INIT FAILURE
	hash_table->capacity = capacity;
	hash_table->loadFactor = 0.75f; //DEFAULT FACTOR
	hash_table->maximSize = maximSize;
	hash_table->size = 0; //EMPTY HASH TABLE
	return true; //SUCCES
}
//Functie pentru iterarea hash table
LogPlayerNodeLL* iterate_hash_table(LogPlayerHashTable_Chaining *hash_table, hash_table_ch_iter *iter) {
	while (iter->current == nullptr) { //cauta primul cap de lista nenul
		if (iter->arr_pos < hash_table->capacity - 1) { //daca mai sunt elemente in vector
			iter->arr_pos++; //avanseaza cu un index inainte
			iter->current = hash_table->arrayPlayers[iter->arr_pos]; //incarca capul listei
		}
		else
			return nullptr; //FAIL -> EOF Hash Table
	}
	LogPlayerNodeLL *node = iter->current;
	if (iter->current) //daca mai sunt elemente in lista
		iter->current = iter->current->next; //advance
	return node;
}
//Functie pentru iterarea unei liste din hash table
LogPlayerNodeLL *iterate_hash_table_cluster(LogPlayerHashTable_Chaining *hash_table, hash_table_ch_iter *iter) {
	if (iter->current) {
		LogPlayerNodeLL *current = iter->current;
		iter->current = iter->current->next;
		return current;
	}
	else
		return nullptr;
}
//Functie pentru rehash
bool rehash(LogPlayerHashTable_Chaining *hash_table) {
	if (hash_table->size >= hash_table->maximSize)
		return false;
	LogPlayerHashTable_Chaining tempHT;
	if (initLogHashTable(&tempHT, nextPrime(hash_table->capacity), hash_table->maximSize) == false)
		return false;
	LogPlayerNodeLL *node = nullptr;
	hash_table_ch_iter iterator;
	INIT_HT_CH_IT(&iterator, hash_table);
	while ((node = iterate_hash_table(hash_table, &iterator)) != nullptr)
		//muta resursele din nodul vechi,indiferent daca acesta era posesor sau nu
		insertLogHashTable(&tempHT, node->logPlayer, node->ownership, ResourcesManagement::MOVE);
	//distrugere tabela veche
	destroyLogHashTable(hash_table);
	//copiem date din buffer inapoi in tabela
	hash_table->capacity = tempHT.capacity;
	hash_table->arrayPlayers = tempHT.arrayPlayers;
	hash_table->size = tempHT.size;
	printf("\nRehashed , capacity %d", hash_table->capacity);
	return true;
}
//Functie pentru adaugarea unui element in hash table
//Daca ownership == DataOwnership::OWNER -> se creaza o copie locala a logului in nodul in care se adauga
//											iar in functie de ResourcesManagement are log mutarea sau copierea resurselor 
//											obiectului care se adauga
//Daca ownership == DataOwnership::NOT_OWNER -> are loc copierea adresei logului in cadrul nodului in care se adauga
//											WARNING! copierea sau mutarea resurselor nu are loc
bool insertLogHashTable(LogPlayerHashTable_Chaining *hash_table, LogPlayer *log, DataOwnership ownership,
	ResourcesManagement resMng) {
	assert(hash_table != nullptr && hash_table->capacity > 0 && log != nullptr);
	unsigned loadFactorNumElem = hash_table->capacity * hash_table->loadFactor;
	if (hash_table->size == loadFactorNumElem) { //e nevoie de rehash
		if (rehash(hash_table) == false)
			return false;
	}
	int position = hash_table->hash(log->adress) % hash_table->capacity;
	if (hash_table->arrayPlayers[position] == nullptr) { //daca lista e goala;
		hash_table->arrayPlayers[position] = makeLogPlayerNodeLL(log, resMng, ownership);
	}
	else {
		LogPlayerNodeLL *node = makeLogPlayerNodeLL(log, resMng, ownership);
		node->next = hash_table->arrayPlayers[position]; //leaga noul node cu capul listei
		hash_table->arrayPlayers[position] = node; //schimba capul listei
	}
	hash_table->size++;
	return true;
}
//Functie pentru cautarea unui element in hash table
//Daca predicate == null -> se adauga in occurences tot clusterul
//Daca predicate != null -> se adauga elementele din cluster care corespund acestui predicat
//WARNING! ** Inainte de inserare lista de occurences se goleste **
void findLogHashTable(LogPlayerHashTable_Chaining *hash_table, std::list<LogPlayer*>& occurences, const char *key,
	log_predicate *predicate, void *args) {
	assert(hash_table != nullptr && key != nullptr);
	if (hash_table->size == 0) { //daca tabela e goala
		return;
	}
	hash_table_ch_iter iterator;
	iterator.arr_pos = hash_table->hash(key) % hash_table->capacity;
	iterator.current = hash_table->arrayPlayers[iterator.arr_pos];
	LogPlayerNodeLL *current = nullptr;
	occurences.clear();
	while ((current = iterate_hash_table_cluster(hash_table, &iterator)) != nullptr) {
		if (predicate && (*predicate)(current->logPlayer, args))
			occurences.push_front(current->logPlayer);
		if (!predicate)
			occurences.push_front(current->logPlayer);
	}
} 
//Functie pentru stergerea elementelor din hash table din cadrul unui cluster care corespund unui predicat
//Daca all == false -> se sterge doar prima aparitie
//Daca all == true -> se sterg toate aparitiile
//Return Value -> true in caz de succes , false daca clusterul e gol sau nici un element nu corespunde predicatului
bool removeLogHashTable(LogPlayerHashTable_Chaining *hash_table, const char *key, log_predicate& predicate, void *args, bool all) {
	assert(hash_table != nullptr && key != nullptr);
	if (hash_table->size == 0) { //daca tabela e goala
		return false;
	}
	//initializare iterator cu clusterul unde se cauta
	hash_table_ch_iter iterator;
	iterator.arr_pos = hash_table->hash(key) % hash_table->capacity;
	iterator.current = hash_table->arrayPlayers[iterator.arr_pos];
	//nodulrile de referinta
	LogPlayerNodeLL *current = nullptr, *prev = nullptr;
	//nodurile care trebuiesc sterse sunt puse intr-o structura auxiliara
	//deoarece iteratorul tine referinta la nodul precedent
	//si la urmatoarea iteratie se va trezi cu "dangling pointer"
	std::set<LogPlayerNodeLL*> nodesToDelete;
	while ((current = iterate_hash_table_cluster(hash_table, &iterator)) != nullptr) {
		if (predicate(current->logPlayer, args)) { //daca nodul curent coincide cu parametrii cu care se cauta
			if (current == hash_table->arrayPlayers[iterator.arr_pos]) //daca log a fost gasit in head
				hash_table->arrayPlayers[iterator.arr_pos] = current->next; //update head
			else  //nu l-am gasit in head
				prev->next = current->next; //refacem legatura cu urmatorul nod, i.e sarim peste nodul curent
			//adauga nodul catre stergere
			nodesToDelete.insert(current);
			if (all == false) // daca nu se doreste stergerea tutor,ci doar a primei
				break; //STOP ITERARE
		} else //daca nodul nu a putut fi sters atunci muta prev pe urmatorul nod care ramane in lista si nu se sterge
			//pentru ca dupa sa se poate face legarea sigura,i.e prev tot timpul pointeaza pe un nod valid care va ramane in lista
			//si nu va fi sters
			prev = current; 
	}
	//dupa procedura de cautare a candidatilor catre stergere,are log efectiv dezalocarea lor
	for_each(nodesToDelete.begin(), nodesToDelete.end(), [](auto node) {freeLogPlayerNodeLL(node); });
	hash_table->size -= nodesToDelete.size();
	return nodesToDelete.size() != 0 ? true : false; //daca set contine noduri inseamna ca stergerea a fost cu succes
}
//Functie pentru distrugerea hash table
//Dezaloca toate nodurile,apoi vectorul de pointeri la liste
//Intoarce tabela la valori default -> capacity = 0 ; size = 0; arrayPlayers = null;
void destroyLogHashTable(LogPlayerHashTable_Chaining *hash_table) {
	hash_table_ch_iter iterator;
	INIT_HT_CH_IT(&iterator, hash_table);
	std::set<LogPlayerNodeLL*> nodesToDelete;
	LogPlayerNodeLL *nodeToDelete = nullptr;
	while ((nodeToDelete = iterate_hash_table(hash_table, &iterator)) != nullptr)
		nodesToDelete.insert(nodeToDelete);
	//stergere noduri din hash table
	for_each(nodesToDelete.begin(), nodesToDelete.end(), [](auto logNode) {freeLogPlayerNodeLL(logNode); });
	//stergere array
	free(hash_table->arrayPlayers);
	hash_table->arrayPlayers = nullptr;
	hash_table->capacity = 0;
	hash_table->size = 0;
}
//Functie pentru procesarea tuturor nodurilor din hash table
void foreachLogHashTable(LogPlayerHashTable_Chaining *hash_table, log_handler handler, void *args) {
	assert(hash_table != nullptr);
	if (hash_table->size == 0)
		return;
	hash_table_ch_iter iterator;
	INIT_HT_CH_IT(&iterator, hash_table);
	LogPlayerNodeLL *node = nullptr;
	while ((node = iterate_hash_table(hash_table, &iterator)) != nullptr)
		handler(node->logPlayer, args);
}
//Functie pentru procesare unui cluster specific din hash table
void foreachLogHashTable(LogPlayerHashTable_Chaining *hash_table, const char* key, log_handler handler, void *args) {
	assert(hash_table != nullptr && key != nullptr);
	if (hash_table->size == 0)
		return;
	hash_table_ch_iter iterator;
	iterator.arr_pos = hash_table->hash(key) % hash_table->capacity;
	iterator.current = hash_table->arrayPlayers[iterator.arr_pos];
	LogPlayerNodeLL *node = nullptr;
	while ((node = iterate_hash_table_cluster(hash_table, &iterator)) != nullptr)
		handler(node->logPlayer, args);
}

/*
		--> Deque <--
	Utilitati:
		* push_front
		* push_back
		* pop_front
		* pop_back
*/

//Functie pentru adaugarea la inceputul deque
void push_front(LogPlayerDeque *deque, LogPlayer *log,ResourcesManagement resMng,DataOwnership ownership) {
	LogPlayerNodeLL *node = makeLogPlayerNodeLL(log, resMng, ownership);
	if (deque->head == nullptr && deque->tail == nullptr) { //empty deque
		deque->head = deque->tail = node;
	}
	else {
		node->next = deque->head;
		deque->head = node;
	}
}
//Functie pentru adaugarea la sfirsitul deque
void push_back(LogPlayerDeque *deque, LogPlayer *log, ResourcesManagement resMng, DataOwnership ownership) {
	LogPlayerNodeLL *node = makeLogPlayerNodeLL(log, resMng, ownership);
	if (deque->head == nullptr && deque->tail == nullptr) {
		deque->head = deque->tail = node;
	}
	else {
		deque->tail->next = node;
		deque->tail = node;
	}
}
//Functie pentru preluarea log de la inceputul deque
LogPlayer *pop_front(LogPlayerDeque *deque) {
	if (!deque->head) return nullptr;
	LogPlayer *info = deque->head->logPlayer;
	LogPlayerNodeLL *nodeToRemove = deque->head;
	deque->head = deque->head->next;
	free(nodeToRemove);
	if (!deque->head) deque->tail = nullptr;
	return info;
}
//Functie pentru preluarea log de la sfirsitul deque
LogPlayer *pop_back(LogPlayerDeque *deque) {
	if (!deque->tail) return nullptr;
	LogPlayer *info = deque->tail->logPlayer;
	LogPlayerNodeLL *toDelete = deque->tail;
	if (deque->head == deque->tail) {
		deque->head = deque->tail = nullptr;
		free(toDelete);
		return info;
	}

	LogPlayerNodeLL *aux = deque->head;
	while (aux->next != deque->tail) aux = aux->next;
	deque->tail = aux;
	deque->tail->next = nullptr;

	free(toDelete);
	return info;
}

