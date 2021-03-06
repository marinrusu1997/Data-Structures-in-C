// PregatireExamenSdd.cpp : Defines the entry point for the console application.
//

//#include "GameServer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <iostream>


/*
--> Graf : reprezentare liste simple inlantuite <--

*/

struct Aeroport {
	int idAeroport;
	const char *numeAeroport;
};

void initAeroport(Aeroport *a) {
	a->idAeroport = 0;
	a->numeAeroport = nullptr;
}
Aeroport* createAeroport(int id, const char *nume) {
	Aeroport *a = (Aeroport*)malloc(sizeof(Aeroport));
	a->idAeroport = id;
	a->numeAeroport = _strdup(nume);
	return a;
}
void freeAeroport(Aeroport *& aeroport) {
	free((void*)aeroport->numeAeroport);
	free(aeroport);
	aeroport = nullptr;
}
void clearAeroport(Aeroport *aeroport) {
	free((void*)aeroport->numeAeroport);
}
void moveAeroport(Aeroport *destination, Aeroport *source) {
	clearAeroport(destination);
	memcpy(destination, source, sizeof(Aeroport));
	initAeroport(source);
}

/*
--> Deque <--
Utilitati:
* push_front
* push_back
* pop_front
* pop_back
*/


struct NodLS {
	Aeroport *aeroport;
	NodLS *next;
	float cost;
};
//nodul din lista secundara are doar o copie a aeroportului
NodLS *makeNodLS(Aeroport *a, float costRuta) {
	NodLS *nod = (NodLS*)malloc(sizeof(NodLS));
	nod->aeroport = a;
	nod->cost = costRuta;
	nod->next = nullptr;
	return nod;
}
//nodul nu e posesorul nodului
void freeNodLS(NodLS *&nod) {
	free(nod);
	nod = nullptr;
}
void destroyLS(NodLS *&head) {
	if (head == nullptr) return;
	NodLS *next = nullptr;
	while (head) {
		next = head->next;
		freeNodLS(head);
		head = next;
	}
}
void insertLS(NodLS *&head, Aeroport *a, float cost) {
	if (!head) {
		head = makeNodLS(a, cost);
	}
	else {
		NodLS *nod = makeNodLS(a, cost);
		nod->next = head;
		head = nod;
	}
}

struct NodLP {
	Aeroport *aeroport;
	NodLP *next;
	NodLS *vecini;
};
NodLP *makeNodLP(Aeroport *a, NodLS *vecini) {
	NodLP *nod = (NodLP*)malloc(sizeof(NodLP));
	nod->aeroport = createAeroport(a->idAeroport, a->numeAeroport);
	nod->vecini = vecini;
	nod->next = nullptr;
	return nod;
}
void freeNodLP(NodLP *&nod) {
	freeAeroport(nod->aeroport);
	destroyLS(nod->vecini);
	free(nod);
	nod = nullptr;
}
void destroyLP(NodLP *&head) {
	NodLP *next = nullptr;
	while (head) {
		next = head->next;
		freeNodLP(head);
		head = next;
	}
}
void insertLP(NodLP *&head, Aeroport *a) {
	if (!head) {
		head = makeNodLP(a, nullptr);
	}
	else {
		NodLP *nod = makeNodLP(a, nullptr);
		nod->next = head;
		head = nod;
	}
}
NodLP *findNodLP(NodLP *head, int idAeroport) {
	while (head) {
		if (head->aeroport->idAeroport == idAeroport)
			return head;
		head = head->next;
	}
	return nullptr;
}
size_t sizeLP(NodLP *head) {
	size_t size = 0;
	while (head) {
		size++;
		head = head->next;
	}
	return size;
}


struct DequeAeropNode {
	NodLP *aeroport;
	DequeAeropNode *next;
};
struct DequeAerop {
	DequeAeropNode *tail, *head;
};
DequeAeropNode *makeDeqNode(NodLP *a) {
	DequeAeropNode *node = (DequeAeropNode*)malloc(sizeof(DequeAeropNode));
	node->aeroport = a;
	node->next = nullptr;
	return node;
}
void freeDeqNode(DequeAeropNode *node) {
	free(node);
}
void printAeroport(Aeroport *a) {
	printf("\n Id %d Nume %s", a->idAeroport, a->numeAeroport);
}
//Functie pentru adaugarea la inceputul deque
void push_front(DequeAerop *deque, NodLP *a) {
	DequeAeropNode *node = makeDeqNode(a);
	if (deque->head == nullptr && deque->tail == nullptr) { //empty deque
		deque->head = deque->tail = node;
	}
	else {
		node->next = deque->head;
		deque->head = node;
	}
}
//Functie pentru adaugarea la sfirsitul deque
void push_back(DequeAerop *deque, NodLP *a) {
	DequeAeropNode *node = makeDeqNode(a);
	if (deque->head == nullptr && deque->tail == nullptr) {
		deque->head = deque->tail = node;
	}
	else {
		deque->tail->next = node;
		deque->tail = node;
	}
}
//Functie pentru preluarea log de la inceputul deque
NodLP *pop_front(DequeAerop *deque) {
	if (!deque->head) return nullptr;
	NodLP *info = deque->head->aeroport;
	DequeAeropNode *nodeToRemove = deque->head;
	deque->head = deque->head->next;
	freeDeqNode(nodeToRemove);
	if (!deque->head) deque->tail = nullptr;
	return info;
}
//Functie pentru preluarea log de la sfirsitul deque
NodLP *pop_back(DequeAerop *deque) {
	if (!deque->tail) return nullptr;
	NodLP *info = deque->tail->aeroport;
	DequeAeropNode *toDelete = deque->tail;
	if (deque->head == deque->tail) {
		deque->head = deque->tail = nullptr;
		freeDeqNode(toDelete);
		return info;
	}

	DequeAeropNode *aux = deque->head;
	while (aux->next != deque->tail) aux = aux->next;
	deque->tail = aux;
	deque->tail->next = nullptr;

	freeDeqNode(toDelete);
	return info;
}

struct GraphAeropoarte {
	NodLP *head;
};

void initGraphAeropoarte(GraphAeropoarte *graph) {
	graph->head = nullptr;
}
void addAeroport(GraphAeropoarte *graph, Aeroport *aeroport) {
	insertLP(graph->head, aeroport);
}
bool addVecin(GraphAeropoarte *graph, float cost, int idAeropDinspre, int idAeropCatre) {
	NodLP *nodAeropDinspre = findNodLP(graph->head, idAeropDinspre);
	if (nodAeropDinspre == nullptr) return false;
	NodLP *nodAeropCatre = findNodLP(graph->head, idAeropCatre);
	if (nodAeropCatre == nullptr) return false;
	insertLS(nodAeropDinspre->vecini, nodAeropCatre->aeroport, cost);
	return true;
}
void destroyGraph(GraphAeropoarte *graph) {
	destroyLP(graph->head);
	graph->head = nullptr;
}
void depthFirst(GraphAeropoarte *graph) {
	int *vizitati = (int*)calloc(sizeLP(graph->head),sizeof(int));
	DequeAerop deq;
	deq.head = deq.tail = nullptr;
	//stack
	push_front(&deq, graph->head);
	vizitati[graph->head->aeroport->idAeroport - 1] = 1;
	NodLP *a = nullptr;
	while ((a = pop_front(&deq)) != nullptr) {
		printAeroport(a->aeroport);
		NodLS *aux = a->vecini;
		while (aux) {
			if (!vizitati[aux->aeroport->idAeroport - 1]) {
				push_front(&deq, findNodLP(graph->head, aux->aeroport->idAeroport));
				vizitati[aux->aeroport->idAeroport - 1] = 1;
			}
			aux = aux->next;
		}
	}
	free(vizitati);
}
void breadthFirst(GraphAeropoarte *graph) {
	bool *vizitati = (bool*)calloc(sizeLP(graph->head), sizeof(bool));
	DequeAerop deq;
	deq.head = deq.tail = nullptr;
	push_back(&deq, graph->head);
	vizitati[graph->head->aeroport->idAeroport - 1] = true;
	NodLP *aerop = nullptr;
	while ((aerop = pop_front(&deq)) != nullptr) {
		printAeroport(aerop->aeroport);
		NodLS *vecin = aerop->vecini;
		while (vecin) {
			if (!vizitati[vecin->aeroport->idAeroport - 1]) {
				push_back(&deq, findNodLP(graph->head, vecin->aeroport->idAeroport));
				vizitati[vecin->aeroport->idAeroport - 1] = true;
			}
			vecin = vecin->next;
		}
	}


	free(vizitati);
}


int main()
{

	GraphAeropoarte graph;
	initGraphAeropoarte(&graph);
	Aeroport a;
	a.idAeroport = 1;
	a.numeAeroport = "Chisinau";
	addAeroport(&graph, &a);
	a.idAeroport = 2;
	a.numeAeroport = "Bucuresti";
	addAeroport(&graph, &a);
	a.idAeroport = 3;
	a.numeAeroport = "Moscova";
	addAeroport(&graph, &a);
	a.idAeroport = 4;
	a.numeAeroport = "Londra";
	addAeroport(&graph, &a);
	a.idAeroport = 5;
	a.numeAeroport = "Paris";
	addAeroport(&graph, &a);
	a.idAeroport = 6;
	a.numeAeroport = "New York";
	addAeroport(&graph, &a);

	//adaugare vecini aeroport 1
	addVecin(&graph, 100, 1, 2);
	addVecin(&graph, 55.6f, 1, 5);
	//adaugare vecini aeroport 2
	addVecin(&graph, 100, 2, 1);
	addVecin(&graph, 156.8f, 2, 3);
	addVecin(&graph, 200.7f, 2, 6);
	addVecin(&graph, 10, 2, 4);
	//adaugare vecini aeroport 3
	addVecin(&graph, 156.8f, 3, 2);
	//adaugare vecini aeroport 4
	addVecin(&graph, 10, 4, 2);
	addVecin(&graph, 59.9f, 4, 5);
	//adaugare vecini aeroport 5
	addVecin(&graph, 55.6f, 5, 1);
	addVecin(&graph, 89.9f, 5, 4);
	//adaugare vecini aeroport 6
	addVecin(&graph, 200.7f, 6, 2);

	depthFirst(&graph);
	printf("\n");
	breadthFirst(&graph);

	destroyGraph(&graph);

	system("pause");
}

