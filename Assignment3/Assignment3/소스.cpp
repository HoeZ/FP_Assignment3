#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdlib>
#include <cmath>
#include "Tokenizer.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

#pragma warning(disable:4996)

using namespace std;

#define DEGREE 510 // 3�̻����� �� ��, 510�̸� 4096Byte�� ������
#define MAX 30  // �޸� �����ϱ� ���� ����ũ�� 
#define LEAF 1
#define INDEX 2
#define FULL 1
#define EPS 0.000001
#define BLOCKSIZE 4096

typedef struct LEAF_NODE
{
	float key[DEGREE];
	int data[DEGREE];
	struct NODE *next;
}LeafNode;

typedef struct INDEX_NODE
{
	float key[DEGREE - 1];
	struct NODE *pointer[DEGREE];
}IndexNode;

typedef struct NODE
{
	int type;
	int full;
	struct NODE *parent;
	union
	{
		LeafNode leafNode;
		IndexNode indexNode;
	}node;
}Node;

typedef struct _students
{
	char name[20];
	unsigned int studentID;
	float score;
	unsigned int advisorID;
}Students;

typedef struct _professors
{
	char name[20];
	unsigned int professorID;
	int salary;
}Professors;

typedef struct _block
{
	Students records[BLOCKSIZE / sizeof(Students)];
}Block;

typedef struct _block2
{
	Professors records[BLOCKSIZE / sizeof(Professors)];
}Block2;

typedef struct
{
	int key;
	int tableNum;
}IndexMap;

Node * root;
Node * Stack[MAX];
int StackPoint;

void initBPlusTree();
void initStack();
Node * getStack();
void addStack(Node *thisNode);
// void Free(Node *thisNode);
Node * FindKey(float AddKey, int inverse, Node *thisNode);

void InsertKey(float key, int data);
void InsertKey1(float insertKey, int insertData, Node *thisNode);
void InsertKey2(float insertKey, int insertData, Node *thisNode);
void InsertKey3(Node *preNode, Node *nextNode, float addkey, Node *thisNode);
void InsertKey4(Node *preNode, Node *nextNode, float addkey, Node *thisNode);

void DeleteKey(float deleteKey);
void DeleteKey1(Node *thisNode);
void DeleteKey2(Node *thisNode);

void SelectKey(float selectKey);

void traverse();
void traverse(Node * p, int depth);

void insertDB(Block*& blocks, int count);
void insertDB2(Block2*& blocks2, int count);
int getInputData(Block*& blocks, string input_str);
int getInputData2(Block2*& blocks, string input_str);
int writeIndexFile(FILE *& fout);
int writeIndexFile2(FILE *& fout);

int main()
{

	initBPlusTree();

	Block *readBlocks, *writeBlocks;
	int count = getInputData(writeBlocks, "sampleData2.csv");
	insertDB(writeBlocks, count);
	int numOfRecords = BLOCKSIZE / sizeof(Students);
	FILE *readDB = fopen("Students.DB", "rb");
	fseek(readDB, 0, SEEK_SET);
	readBlocks = new Block[count / numOfRecords + 1];
	fread((void*)readBlocks, sizeof(Block), count / numOfRecords + 1, readDB);

	//score is the key of B+tree
	//insert <key, value> into index
	for (int j = 0; j < count / numOfRecords + 1; j++) {
		for (int i = 0; i < numOfRecords; i++) {
			InsertKey(readBlocks[j].records[i].score, 999);
		}
	}

	//make Students.idx
	FILE *indexFile = fopen("Students_score.idx", "wb");
	if (writeIndexFile(indexFile) == -1)
		cout << ".idx file error." << endl;

	traverse();

	system("pause");
	//Free(root);
	return 0;
}

void traverse()
{
	traverse(root, 0);
}

void traverse(Node * p, int depth)
{
	int i;

	printf("depth %d : ", depth);
	if (p->type == LEAF)
	{
		for (i = 0; i < DEGREE; i++)
		{
			if (p->node.indexNode.key[i] != 0)
				printf("%.6f ", p->node.leafNode.key[i]);
		}
		printf("\n");
	}
	else
	{
		for (i = 0; i < DEGREE - 1; i++)
		{
			if (p->node.indexNode.key[i] != 0)
				printf("%.6f ", p->node.indexNode.key[i]);
		}
		printf("\n");
		for (i = 0; i < DEGREE - 1; i++)
		{
			if (p->node.indexNode.pointer[i] != NULL)
			{
				traverse(p->node.indexNode.pointer[i], depth + 1);
			}
		}
		if (p->node.indexNode.pointer[i] != NULL)
		{
			traverse(p->node.indexNode.pointer[i], depth + 1);
		}
		printf("\n");
	}
}

void SelectKey(float selectKey)
{
	Node *thisNode;
	int i;
	if ((thisNode = FindKey(selectKey, 1, root)) == NULL)
	{
		printf("Key = %.1f\n", selectKey);
		puts("Ű�� �������� �ʽ��ϴ�.\n");
		return;
	}
	// key ���� ���� Key �� �ִ��� Ȯ��
	for (i = 0; i < DEGREE; i++)
		if (fabsf(thisNode->node.leafNode.key[i] - selectKey) < EPS)
		{
			printf("Key = %.1f\n", selectKey);
			printf("Data = %d\n", thisNode->node.leafNode.data[i]);
		}
}
void InsertKey4(Node *preNode, Node *nextNode, float addkey, Node *thisNode)
{
	/*
	InsertKey4 �Լ�
	������ �̷������ ��尡 �ε��� ��� �� �� ���

	Node *preNode : Ű ���ʿ� �߰��� ������
	Node *nextNode : Ű ���ʿ� �߰��� ������
	int addKey  : Ű ��ȣ
	Node *thisNode : ���ҵ� �ε��� ���
	*/
	Node *addNode;
	Node *addIndexNode;
	Node *tempPoint[DEGREE + 1];
	float tempKey[DEGREE];
	int i, j;
	// �ε����� ����Ʈ�� Ű�� ����
	for (i = 0; i < DEGREE; i++)
		tempPoint[i] = thisNode->node.indexNode.pointer[i];
	for (i = 0; i < DEGREE - 1; i++)
		tempKey[i] = thisNode->node.indexNode.key[i];
	// �߰��� key ������ ū ���� ã��
	for (i = 0; i < DEGREE - 1; i++)
	{
		// ���� ���� Ű�� �����ϴ��� Ȯ�� 
		if (fabsf(tempKey[i] - addkey) < EPS) return;
		// ���� ����Ű�� �ִ� Key �� �߰��� Key ���� ũ�ų� 0���� Ȯ��
		if ((tempKey[i] > addkey) || (tempKey[i] == 0)) break;
	}
	// �߰��ؾ� �� ���� ������� ���� ��
	if (tempKey[i] != 0)
	{
		// �߰��� �ڸ��� �������� ���������� ����Ʈ
		for (j = DEGREE; j > i; j--)
			tempPoint[j] = tempPoint[j - 1];
		for (j = DEGREE - 1; j > i; j--)
			tempKey[j] = tempKey[j - 1];
	}
	tempKey[i] = addkey;
	tempPoint[i] = preNode;
	tempPoint[i + 1] = nextNode;
	// ���ο� �ε��� ��� ����
	addNode = (Node *)malloc(sizeof(Node));
	memset((char *)addNode, 0, sizeof(Node));
	addNode->type = INDEX;
	addNode->parent = thisNode->parent;
	memset((char *)thisNode, 0, sizeof(Node));
	thisNode->type = INDEX;
	thisNode->parent = addNode->parent;
	// �ݹݾ� ������ thisNode , addNode�� �Ҵ� ���� �ڽ� ��忡�� �θ��� �˸�
	for (i = 0; i < DEGREE / 2; i++)
	{
		thisNode->node.indexNode.pointer[i] = tempPoint[i];
		thisNode->node.indexNode.key[i] = tempKey[i];
		thisNode->node.indexNode.pointer[i]->parent = thisNode;
	}
	thisNode->node.indexNode.pointer[i] = tempPoint[i];
	thisNode->node.indexNode.pointer[i]->parent = thisNode;
	i++;
	for (j = 0; i < DEGREE; j++, i++)
	{
		addNode->node.indexNode.pointer[j] = tempPoint[i];
		addNode->node.indexNode.key[j] = tempKey[i];
		addNode->node.indexNode.pointer[j]->parent = addNode;
	}
	addNode->node.indexNode.pointer[j] = tempPoint[i];
	addNode->node.indexNode.pointer[j]->parent = addNode;

	// �θ��尡 ���ٸ� ����
	if (thisNode->parent == NULL)
	{
		addIndexNode = (Node *)malloc(sizeof(Node));
		memset((char *)addIndexNode, 0, sizeof(Node));
		addIndexNode->type = INDEX;
		addIndexNode->node.indexNode.pointer[0] = thisNode;
		addIndexNode->node.indexNode.pointer[1] = addNode;
		addIndexNode->node.indexNode.key[0] = tempKey[DEGREE / 2];
		// �θ� ��� ����
		thisNode->parent = addIndexNode;
		addNode->parent = addIndexNode;
		root = addIndexNode;
	}
	else
		// �θ� ��尡 �ִٸ� InsertKey3 �� ����
		InsertKey3(thisNode, addNode, tempKey[DEGREE / 2], thisNode->parent);
}
void InsertKey3(Node *preNode, Node *nextNode, float addkey, Node *thisNode)
{
	/*
	InsertKey3 �Լ�
	�ε��� ��� �̰� ��忡 ������� ���� �� ���

	Node *preNode : Ű ���ʿ� �߰��� ������
	Node *nextNode : Ű ���ʿ� �߰��� ������
	int addKey  : Ű ��ȣ
	Node *thisNode : � �ε��� ��忡 �߰��� ������
	*/
	int i, j;
	//thisNode �� �ε��� ��� ���� Ȯ��
	if (thisNode->type != INDEX) return;
	// �ε��� ��尡 �����ִٸ� InsertKey4 �� ����
	if (thisNode->full == FULL)
	{
		InsertKey4(preNode, nextNode, addkey, thisNode);
		return;
	}
	// �߰��� key ������ ū ���� ã��
	for (i = 0; i < DEGREE - 1; i++)
	{
		// ���� ���� Ű�� �����ϴ��� Ȯ�� 
		if (fabsf(thisNode->node.indexNode.key[i] - addkey) < EPS) return;
		// ���� ����Ű�� �ִ� Key �� �߰��� Key ���� ũ�ų� 0���� Ȯ��
		if ((thisNode->node.indexNode.key[i] > addkey) || (thisNode->node.indexNode.key[i] == 0)) break;
	}
	// �߰��ؾ� �� ���� ������� ���� ��
	if (thisNode->node.indexNode.key[i] != 0)
	{
		// �߰��� �ڸ��� �������� ���������� ����Ʈ
		for (j = DEGREE - 1; j > i; j--)
			thisNode->node.indexNode.pointer[j] = thisNode->node.indexNode.pointer[j - 1];
		for (j = DEGREE - 2; j > i; j--)
			thisNode->node.indexNode.key[j] = thisNode->node.indexNode.key[j - 1];
	}
	thisNode->node.indexNode.key[i] = addkey;
	thisNode->node.indexNode.pointer[i] = preNode;
	thisNode->node.indexNode.pointer[i + 1] = nextNode;
	// �߰� �� �� ��尡 ���� ���ִ��� Ȯ��
	if (thisNode->node.indexNode.key[DEGREE - 2] != 0) thisNode->full = FULL;
}
void InsertKey2(float insertKey, int insertData, Node *thisNode)
{
	/*
	InsertKey2 �Լ�
	��������̰� ���� �� �� ���

	int insertKey  : �߰��� Ű
	int insertData  : �߰��� ������
	Node *thisNode  : ���� �� ���
	*/
	int i;
	Node *addNode;
	Node *addIndexNode;
	// ���� ��尡 �� ���ִ��� Ȯ���Ѵ�.
	if (thisNode->full != FULL) return;
	// ���� ��尡 ���� ��尡 �ƴ϶�� ����
	if (thisNode->type != LEAF) return;
	// �ϳ��� ���� ��� ���� �� �ʱ�ȭ
	addNode = (Node *)malloc(sizeof(Node));
	memset((char *)addNode, 0, sizeof(Node));
	addNode->type = LEAF;
	// ���� ����� ���� ������ �κ��� ���ο� ��忡 �����Ѵ�.
	InsertKey1(thisNode->node.leafNode.key[DEGREE - 1], thisNode->node.leafNode.data[DEGREE - 1], addNode);
	// �߰��� Ű�� �����͸� ����� FULL ���°� �ƴϹǷ� 0���� �ٲ۴�.
	thisNode->node.leafNode.key[DEGREE - 1] = 0;
	thisNode->node.leafNode.data[DEGREE - 1] = 0;
	thisNode->full = 0;
	// ���� ��忡 ���ο� Ű�� �߰� ��Ų��.
	InsertKey1(insertKey, insertData, thisNode);
	thisNode->full = FULL;
	// ���� ����� ������ �߰� ��忡 �����Ѵ�.
	for (i = DEGREE / 2 + 1; i < DEGREE; i++)
	{
		InsertKey1(thisNode->node.leafNode.key[i], thisNode->node.leafNode.data[i], addNode);

		thisNode->node.leafNode.key[i] = 0;
		thisNode->node.leafNode.data[i] = 0;
	}
	thisNode->full = 0;
	// ������ ����
	addNode->node.leafNode.next = thisNode->node.leafNode.next;
	thisNode->node.leafNode.next = addNode;
	addNode->parent = thisNode->parent;
	// �θ� ��尡 ���ٸ� ����
	if (thisNode->parent == NULL)
	{
		addIndexNode = (Node *)malloc(sizeof(Node));
		memset((char *)addIndexNode, 0, sizeof(Node));
		addIndexNode->type = INDEX;
		addIndexNode->node.indexNode.pointer[0] = thisNode;
		addIndexNode->node.indexNode.pointer[1] = addNode;
		addIndexNode->node.indexNode.key[0] = thisNode->node.leafNode.key[DEGREE / 2];
		// �θ� ��� ����
		thisNode->parent = addIndexNode;
		addNode->parent = addIndexNode;
		root = addIndexNode;
	}
	else
		// �θ� ��尡 �ִٸ� InsertKey3 �� ����
		InsertKey3(thisNode, addNode, thisNode->node.leafNode.key[DEGREE / 2], thisNode->parent);
}
void InsertKey1(float insertKey, int insertData, Node *thisNode)
{
	/*
	InsertKey1 �Լ�
	��������̰� ��忡 ������� �־ �׳� �߰��� �� ���

	int insertKey : �߰��� Ű
	int insertData : �߰��� ������
	Node *thisNode : � ���� ��忡 �߰��� ������
	*/
	int i, j;
	// thisNode �� NULL �� �� ����
	if (thisNode == NULL)
	{
		printf("Ű�� �ߺ� �˴ϴ�..\n");
		return;
	}
	// ��尡 ���� �������� InsertKey2 ����
	if (thisNode->full == FULL)
	{
		InsertKey2(insertKey, insertData, thisNode);
		return;
	}
	// ���� ��尡 ������尡 �ƴ϶�� ����
	if (thisNode->type != LEAF) return;
	// �߰��� key ������ ū ���� ã��
	for (i = 0; i < DEGREE; i++)
	{
		// ���� ���� Ű�� �����ϸ� ����
		if (fabsf(thisNode->node.leafNode.key[i] - insertKey) < EPS) return;
		// ���� ����Ű�� �ִ� Key �� �߰��� Key ���� ũ�ų� 0���� Ȯ��
		if ((thisNode->node.leafNode.key[i] > insertKey) || (thisNode->node.leafNode.key[i] == 0)) break;
	}
	// �߰��� ���� ����־� ���� ���� ��
	if (thisNode->node.leafNode.key[i] != 0)
		// �߰��� �ڸ��� �������� ���������� ����Ʈ
		for (j = DEGREE - 1; j > i; j--)
		{
			thisNode->node.leafNode.key[j] = thisNode->node.leafNode.key[j - 1];
			thisNode->node.leafNode.data[j] = thisNode->node.leafNode.data[j - 1];
		}

	// Ű�� ������ �߰� 
	thisNode->node.leafNode.key[i] = insertKey;
	thisNode->node.leafNode.data[i] = insertData;
	// �߰� �� �� ��尡 ���� ���ִ��� Ȯ��
	if (thisNode->node.leafNode.key[DEGREE - 1] != 0) thisNode->full = FULL;
}
void InsertKey(float key, int data)
{
	//Ű ���� 0�� �� �� ���� 0�� ����ִ� Ű�� ����ϱ� ����
	if (key == 0) return;
	InsertKey1(key, data, FindKey(key, 0, root));
}
void initStack()
{
	// ���� �ʱ�ȭ
	int i;
	for (i = 0; i < MAX; i++)
		Stack[i] = NULL;
	StackPoint = 0;
}
//void Free(Node *thisNode)
//{
//	/*
//	�޸𸮸� ���� �����ϴ� �Լ�
//	Stack �� �̿��Ͽ� malloc �Լ��� ������� ��� �޸𸮸� �����Ѵ�.
//	*/
//	int i;
//	initStack();
//	if (thisNode->type == LEAF)
//	{
//		free(thisNode);
//		return;
//	}
//	for (i = 0; i<DEGREE; i++)
//		if (thisNode->node.indexNode.pointer[i] != NULL)
//			addStack(root->node.indexNode.pointer[i]);
//	free(thisNode);
//	while ((thisNode = getStack()) != NULL)
//	{
//		if (thisNode->type == LEAF)
//			free(thisNode);
//		else
//		{
//			for (i = 0; i<DEGREE; i++)
//			{
//				if (thisNode->node.indexNode.pointer[i] != NULL)
//					addStack(thisNode->node.indexNode.pointer[i]);
//			}
//			free(thisNode);
//		}
//	}
//}
void DeleteKey2(Node *thisNode)
{
	/*
	DeleteKey2 �Լ�
	�ε��� ����� ��й� �� �����ϴ� �Լ�

	Node *thisNode : �� ���� �� ������ �ؾ��ϴ� ���
	*/
	int i, j, k;
	int thisPoint;
	Node *parent;
	Node *broNode;
	Node *tempNode;
	// ���� ��尡 �ε��� ��尡 �ƴ϶�� ����
	if (thisNode->type != INDEX) return;
	// �θ��� ����
	parent = thisNode->parent;
	// ���� ��尡 ��Ʈ�̸� ����
	if (parent == NULL) return;
	// ���� ��尡 �θ� ����� ���° ����Ʈ�� ���� Ȯ��
	for (thisPoint = 0; thisPoint < DEGREE - 1; thisPoint++)
		if (parent->node.indexNode.pointer[thisPoint] == thisNode) break;
	// �����尡 �θ� ����� 0��° �ε����� �ƴ� �� 
	broNode = (thisPoint != 0) ? parent->node.indexNode.pointer[thisPoint - 1] : parent->node.indexNode.pointer[thisPoint + 1];
	// ���� ��忡 ����Ʈ�� ���� Ȯ���Ѵ�.
	for (i = 0; i < DEGREE; i++)
		if (broNode->node.indexNode.pointer[i] == NULL) break;
	// ���� ���� ��尡 ���� ��忡�� ����Ʈ�� �� �ٶ� 50% �̻��� ������ �ȵɰ�� ������ �Ѵ�.
	if ((DEGREE % 2 == 0 && DEGREE / 2 >= i) || (DEGREE % 2 == 1 && DEGREE / 2 + 1 >= i)) // ����
	{
		// thisPoint == 0 �� �� broNode �� thisNode �����͸� ��ü�Ѵ�.
		if (thisPoint == 0)
		{
			tempNode = thisNode;
			thisNode = broNode;
			broNode = tempNode;
		}
		else
		{
			thisPoint--;
		}
		// �߰��� ���� �����͸� ã��
		for (j = 0; j < DEGREE; j++)
			if (broNode->node.indexNode.pointer[j] == NULL) break;
		broNode->node.indexNode.key[j - 1] = parent->node.indexNode.key[thisPoint];

		// thisNode �� �����͸� broNode ���� �ű�
		for (i = j, k = 0; i < DEGREE; i++, k++)
		{
			broNode->node.indexNode.pointer[i] = thisNode->node.indexNode.pointer[k];
			broNode->node.indexNode.pointer[i]->parent = broNode;
		}
		// thisNode �� Ű�� broNode ���� �ű�
		for (i = j, k = 0; i < DEGREE - 1; i++, k++)
			broNode->node.indexNode.key[i] = thisNode->node.indexNode.key[k];
		broNode->full = FULL;
		for (i = thisPoint + 1; i < DEGREE - 1; i++)
			parent->node.indexNode.pointer[i] = parent->node.indexNode.pointer[i + 1];

		parent->node.indexNode.pointer[i] = NULL;

		for (i = thisPoint; i < DEGREE - 2; i++)
			parent->node.indexNode.key[i] = parent->node.indexNode.key[i + 1];
		parent->node.indexNode.key[i] = 0;
		parent->full = 0;
		free(thisNode);
		// ���� Ʈ���� ������ �����ϴ��� Ȯ�� M�� ���϶� �ּ� 50% �̻� ����Ʈ�� ������ �־����
		for (i = 0; i < DEGREE; i++)
			if (parent->node.indexNode.pointer[i] == NULL) break;
		// �θ��尡 root �̰� ����Ʈ���� �ϳ� �ۿ� ���ٸ� index ��尡 �ʿ� ����
		if (parent == root)
		{
			if ((i == 1))
			{
				root = parent->node.indexNode.pointer[0];
				root->parent = NULL;
				free(parent);
			}
			return;
		}
		// ������ ¦���̰� 50% �̻� ���� �̰ų� ������ Ȧ���̰� 66% �̻� ���� �Ǹ� ����
		if ((DEGREE % 2 == 0 && DEGREE / 2 <= i) || (DEGREE % 2 == 1 && DEGREE / 2 + 1 <= i)) return;
		//50%�� ������ �ȵǸ�
		DeleteKey2(parent);
	}
	else // ��й�
	{
		if (thisPoint == 0)
		{
			// Ű�� 0�� ���� ã�Ƽ� key ���� �����Ѵ�.
			for (i = 0; i < DEGREE; i++)
			{
				if (thisNode->node.indexNode.key[i] == 0)
				{
					thisNode->node.indexNode.key[i] = parent->node.indexNode.key[thisPoint];
					break;
				}
			}
			// broNode ���� ���� ���ϳ��� �޾� �´�.
			thisNode->node.indexNode.pointer[i + 1] = broNode->node.indexNode.pointer[0];
			thisNode->node.indexNode.pointer[i + 1]->parent = thisNode;
			parent->node.indexNode.key[thisPoint] = broNode->node.indexNode.key[0];
			broNode->full = 0;
			// broNode�� �����Ѵ�.
			for (j = 0; j < DEGREE - 2; j++)
			{
				broNode->node.indexNode.key[j] = broNode->node.indexNode.key[j + 1];
				broNode->node.indexNode.pointer[j] = broNode->node.indexNode.pointer[j + 1];
			}
			broNode->node.indexNode.key[j] = 0;
			broNode->node.indexNode.pointer[j] = broNode->node.indexNode.pointer[j + 1];
			broNode->node.indexNode.pointer[j + 1] = NULL;
		}
		else
		{
			// ���� ��忡 ������ Key ��ġ�� Ȯ���Ѵ�.
			for (i = 0; i < DEGREE; i++)
				if (broNode->node.indexNode.pointer[i] == NULL) break;
			i--;
			// thisNode�� �����Ѵ�.
			for (j = DEGREE - 1; j > 0; j--)
				thisNode->node.indexNode.pointer[j] = thisNode->node.indexNode.pointer[j - 1];
			for (j = DEGREE - 2; j > 0; j--)
				thisNode->node.indexNode.key[j] = thisNode->node.indexNode.key[j - 1];

			// broNode���� ���� �޾ƿ´�.
			thisNode->node.indexNode.pointer[0] = broNode->node.indexNode.pointer[i];
			thisNode->node.indexNode.pointer[0]->parent = thisNode;

			broNode->node.indexNode.pointer[i] = NULL;
			thisNode->node.indexNode.key[0] = parent->node.indexNode.key[thisPoint - 1];

			parent->node.indexNode.key[thisPoint - 1] = broNode->node.indexNode.key[i - 1];

			broNode->node.indexNode.key[i - 1] = 0;
			broNode->full = 0;
		}
	}
}
void DeleteKey1(Node *thisNode)
{
	/*
	DeleteKey1 �Լ�
	���� ����� ��й� �� �����ϴ� �Լ�

	Node *thisNode : �� ���� �� ������ �ؾ��ϴ� ���
	*/
	int i, j;
	int thisPoint;
	Node *parent;
	Node *broNode;
	Node *tempNode;
	// ���� ��尡 ������尡 �ƴ϶�� ����
	if (thisNode->type != LEAF) return;
	// �θ� ��带 ����
	parent = thisNode->parent;
	// ���� ������尡 ��Ʈ�̸� ����
	if (parent == NULL) return;
	// ���� ��尡 �θ� ����� ���° ����Ʈ�� ���� Ȯ��
	for (thisPoint = 0; thisPoint < DEGREE - 1; thisPoint++)
		if (parent->node.indexNode.pointer[thisPoint] == thisNode) break;
	/*
	�����尡 �θ� �����
	0��° ����Ʈ���� �ƴϸ� ���� ����Ʈ���� ��й� �� ����
	0��° ����Ʈ���� ������ ����Ʈ���� ��й� �� ����
	���õ� ����Ʈ���� ��������� �Ѵ�.
	*/
	broNode = (thisPoint != 0) ? parent->node.indexNode.pointer[thisPoint - 1] : parent->node.indexNode.pointer[thisPoint + 1];
	// ������忡 Key ���� Ȯ���Ѵ�.
	for (i = 0; i < DEGREE; i++)
		if (broNode->node.leafNode.key[i] == 0) break;
	// ���� �������� ����縦 ������� 50% �̻��� �ȵ� �� ������ �Ѵ�.
	if ((DEGREE % 2 == 0 && DEGREE / 2 >= i) || (DEGREE % 2 == 1 && DEGREE / 2 + 1 >= i)) // ����
	{
		/*
		�����尡 0��° ����Ʈ���� broNode�� thisNode �̸��� �ٲ۴�.
		�̴� ������ �� ��带 �ϳ� �����ߵǴµ� �� �� �����ʿ� �ִ� ��带 ������.
		���ʿ� �ִ� ��带 ������� ������峢�� ����� �����͵��� ����������.
		���� �̸��� �״�� ����� ��� ������ ��尡 �������� ������ �̸��� �ٲ۴�.
		thisPoint �� broNode�� ����Ű�� �ε����ν� ����Ѵ�.
		*/
		if (thisPoint == 0)
		{
			tempNode = thisNode;
			thisNode = broNode;
			broNode = tempNode;
		}
		else
			thisPoint--;
		// thisNode�� �������� ������ ���� ������� �����͸� broNode�� �����Ѵ�.
		broNode->node.leafNode.next = thisNode->node.leafNode.next;
		// thisNode �� ������ �ִ� Key �� Data �� broNode �� �ű��.
		for (j = 0; j < DEGREE; j++)
		{
			// thisNode�� ������ �ִ� ������ ���̻� ������ ����������.
			if (thisNode->node.leafNode.key[j] == 0) break;
			// ���� ����Ű���ִ� Key �� Data �� broNode�� �ű��.
			InsertKey1(thisNode->node.leafNode.key[j], thisNode->node.leafNode.data[j], broNode);
		}
		// ������ �ϰԵǸ� �� ���� �׻� FULL ���°� �ȴ�.
		broNode->full = FULL;
		// thisNode�� �����͸� �����ϱ� ���� �θ��带 �����Ѵ�.
		for (i = thisPoint + 1; i < DEGREE - 1; i++)
			parent->node.indexNode.pointer[i] = parent->node.indexNode.pointer[i + 1];

		parent->node.indexNode.pointer[i] = NULL;
		for (i = thisPoint; i < DEGREE - 2; i++)
			parent->node.indexNode.key[i] = parent->node.indexNode.key[i + 1];
		parent->node.indexNode.key[i] = 0;
		// ���� �Ǿ��� ������ �׻� �θ���� FULL ���°� �ƴϰԵȴ�.
		parent->full = 0;
		// thisNode�� �����Ѵ�.
		free(thisNode);
		// �ε�����尡 ��� ����Ʈ���� ������ �ִ��� Ȯ���Ѵ�.
		for (i = 0; i < DEGREE; i++)
			if (parent->node.indexNode.pointer[i] == NULL) break;
		// �ε�����尡 root �̰� ����Ʈ���� �ϳ� �ۿ� ���ٸ� index ��尡 �ʿ� ����
		if (parent == root)
		{
			if ((i == 1))
			{
				root = parent->node.indexNode.pointer[0];
				root->parent = NULL;
				free(parent);
			}
			return;
		}
		// ������ ¦���̰� 50% �̻� ���� �̰ų� ������ Ȧ���̰� 66% �̻� ���� �Ǹ� ����
		if ((DEGREE % 2 == 0 && DEGREE / 2 <= i) || (DEGREE % 2 == 1 && DEGREE / 2 + 1 <= i)) return;
		//50%�� ������ �ȵǸ�
		DeleteKey2(parent);
	}
	else // ��й�
	{
		/*
		����Ʈ���� 0�϶��� �����ʰ� ��й踦
		0�� �ƴҶ��� ���ʰ� ��й踦 �ؾ��ϱ� ������ ����
		���� ��й�� �θ� �ε�������� ����Ʈ�� ���� �ٲ��� �ʱ� ������
		�ε�������� 50%������ �˻����� �ʾƵ� ��
		*/
		if (thisPoint == 0)
		{
			/*
			�����ʿ� �ִ� ������忡�� �� �ϳ��� �����´�.
			�ϳ��� �������� ������ ���� �Ǵ� ���� �ϳ��� �̷������ ������
			50%�� ������ �ȵȴٸ� �װ��� �ϳ��� ������ ���̴�.
			*/
			InsertKey1(broNode->node.leafNode.key[0], broNode->node.leafNode.data[0], thisNode);
			/*
			�θ����� �ε����� ������忡�� ������ ������ �����Ѵ�.
			�����庸�� ������尡 �����ʿ� �ֱ� ������
			������ ������忡�� ������ ���� ũ�� �����̴�.
			*/
			parent->node.indexNode.key[thisPoint] = broNode->node.leafNode.key[0];
			// ������带 �����Ѵ�.
			for (i = 0; i < DEGREE - 1; i++)
			{
				broNode->node.leafNode.key[i] = broNode->node.leafNode.key[i + 1];
				broNode->node.leafNode.data[i] = broNode->node.leafNode.data[i + 1];
			}
			broNode->node.leafNode.key[i] = 0;
			broNode->node.leafNode.data[i] = 0;
			// �ϳ��� ���ŵǾ��� ������ ������ FULL ���°� �ƴϴ�.
			broNode->full = 0;
		}
		else
		{
			// ���ʿ� ��������̹Ƿ� ���� ��忡�� ������ ������ ���� �˻��Ѵ�.
			for (i = 0; i < DEGREE; i++)
				if (broNode->node.leafNode.key[i] == 0) break;
			i--;
			// �˻��� Key �� Data �� �����忡 �߰� ��Ų��.
			InsertKey1(broNode->node.leafNode.key[i], broNode->node.leafNode.data[i], thisNode);
			// ������ Key �� Data �� �����ϰ� ������ FULL ���°� �ƴ�
			broNode->node.leafNode.key[i] = 0;
			broNode->node.leafNode.data[i] = 0;
			broNode->full = 0;
			// ������带 ����Ű�� �ε����� �ٲپ��ش�.
			parent->node.indexNode.key[thisPoint - 1] = broNode->node.leafNode.key[i - 1];
		}
	}
}
void DeleteKey(float deleteKey)
{
	/*
	DeleteKey �Լ�
	Ʈ�� ���� ���� Ű�� ���� �Ѵ�.
	������ �� ��尡 50%�̻� ������ ���� �ʴ´ٸ� DeleteKey1 �� �����Ѵ�.
	int deleteKey : ������ Ű
	*/
	int i, j;
	Node *thisNode;
	// �������� �ʴ� Ű��� ���� �� �� ����
	if ((thisNode = FindKey(deleteKey, 1, root)) == NULL) return;
	// key ���� ���� Key �� ��� ��ġ�� �ִ��� Ȯ��
	for (i = 0; i < DEGREE; i++)
		if (fabsf(thisNode->node.leafNode.key[i] - deleteKey) < EPS) break;
	// �� ���� �ִ� Key �� ����
	thisNode->node.leafNode.key[i] = 0;
	thisNode->node.leafNode.data[i] = 0;

	// ��� �ִ� ���� �޲ٱ� ���� �ٽ� ����
	for (j = i; j < DEGREE - 1; j++)
	{
		thisNode->node.leafNode.key[j] = thisNode->node.leafNode.key[j + 1];
		thisNode->node.leafNode.data[j] = thisNode->node.leafNode.data[j + 1];
	}
	thisNode->node.leafNode.key[j] = 0;
	thisNode->node.leafNode.data[j] = 0;
	//�����ٸ� �� ���´� ������ FULL �� �ƴ�
	thisNode->full = 0;
	// ���� Key �� ������� �ִ��� Ȯ��
	for (i = 0; i < DEGREE; i++)
		if (thisNode->node.leafNode.key[i] == 0) break;
	// ������ ¦���̰� 50% �̻� ����, ������ Ȧ���̰� 66% �̻� ���� �Ǹ� ����
	if ((DEGREE % 2 == 0 && DEGREE / 2 <= i) || (DEGREE % 2 == 1 && DEGREE / 2 + 1 <= i)) return;
	//50%�� ������ �ȵǸ� DeleteKey1 �� �̿��Ͽ� ��й� �� ���� ����
	DeleteKey1(thisNode);
}
void initBPlusTree()
{
	/*
	3 �̸����� �ϰ� �Ǹ� ���԰�����
	��Ʈ�� �ƴ� �ε�������� ����Ʈ�� ������ 1���� �� ��찡 ����
	���� ������ ���� ����
	*/

	if (DEGREE < 3)
	{
		printf("������ 3�̸��� �ɼ� �����ϴ�.\n");
		exit(1);
	}

	// ��Ʈ ���� �� �ʱ�ȭ
	root = (Node *)malloc(sizeof(Node));
	memset((char *)root, 0, sizeof(Node));
	root->type = LEAF;
}
void addStack(Node *thisNode)
{
	// ������ ����á�ٸ� ����
	if (StackPoint == MAX)
	{
		printf("������ ����á���ϴ�. ���� ũ�⸦ �÷��� �ٽ� �õ��ϼ��� ~\n");
		return;
	}
	Stack[StackPoint++] = thisNode;
}
Node *getStack()
{
	// ������ ����ִٸ� ����
	return (StackPoint == 0) ? NULL : Stack[--StackPoint];
}
Node *FindKey(float AddKey, int inverse, Node *thisNode)
{
	/*
	node* FindKey(int AddKey)
	Ű�� �̹� �����ϴ��� Ȯ��
	inverse �� 0�� ��
	���� �����Ѵٸ� NULL �� ��ȯ
	���� �������� �ʴ´ٸ� ������ �˻��� ������带 ��ȯ
	inverse �� 0�� �ƴҶ� �ݴ�� ��ȯ
	*/
	int i;
	while (1)
	{
		// ��尡 ������ �ƴҶ�
		if (thisNode->type != LEAF)
		{
			// �߰��� key ������ ū ���� ã��
			for (i = 0; i < DEGREE - 1; i++)
				if (thisNode->node.indexNode.key[i] >= AddKey || thisNode->node.indexNode.key[i] == 0) break;

			// thisNode ��ü
			thisNode = (i == DEGREE - 1) ? thisNode->node.indexNode.pointer[DEGREE - 1] : thisNode = thisNode->node.indexNode.pointer[i];
		}
		else
		{
			// key ���� ���� Key �� �ִ��� Ȯ��
			for (i = 0; i < DEGREE; i++)
				if (fabsf(thisNode->node.leafNode.key[i] - AddKey) < EPS)
					return (inverse == 0) ? NULL : thisNode;

			return (inverse == 0) ? thisNode : NULL;
		}
	}
}

void insertDB(Block*& blocks, int count) {
	//insert hashTable into DB as binary format
	int numOfRecords = BLOCKSIZE / sizeof(Students);
	FILE * DBFile = fopen("Students.DB", "wb");
	fseek(DBFile, 0, SEEK_SET);
	fwrite((char*)blocks, sizeof(Block), count / numOfRecords + 1, DBFile);
}

void insertDB2(Block2*& blocks2, int count) {
	//insert hashTable into DB as binary format
	int numOfRecords = BLOCKSIZE / sizeof(Professors);
	FILE * DBFile = fopen("Professorss.DB", "wb");
	fseek(DBFile, 0, SEEK_SET);
	fwrite((char*)blocks2, sizeof(Block), count / numOfRecords + 1, DBFile);
}

int getInputData(Block*& blocks, string input_str) {

	//Get input data from .csv
	ifstream input_data(input_str.c_str());
	string buf;
	Tokenizer tokenizer; //include "Tokenize.h"
	int numOfRecords = BLOCKSIZE / sizeof(Students);
	tokenizer.setDelimiter(","); //parsing Delimiter = ","
	getline(input_data, buf);
	tokenizer.setString(buf);
	int count = atoi(tokenizer.next().c_str()); //the num of Students

	blocks = new Block[count / numOfRecords + 1];

	//initialize blocks
	for (int j = 0; j < count / numOfRecords + 1; j++) {
		for (int i = 0; i < numOfRecords; i++) {
			strcpy(blocks[j].records[i].name, "");
			blocks[j].records[i].studentID = 0;
			blocks[j].records[i].score = 0;
			blocks[j].records[i].advisorID = 0;
		}
	}

	//read inputFile and then put value into blocks
	for (int j = 0; j < count / numOfRecords + 1; j++) {
		for (int i = 0; i < numOfRecords; i++) {

			string temp;
			getline(input_data, buf);
			tokenizer.setString(buf);
			if (input_data.eof()) break;

			strncpy(blocks[j].records[i].name, tokenizer.next().c_str(), sizeof(blocks[j].records[i].name));
			blocks[j].records[i].studentID = atoi(tokenizer.next().c_str());
			blocks[j].records[i].score = (float)atof(tokenizer.next().c_str());
			blocks[j].records[i].advisorID = atoi(tokenizer.next().c_str());
		}
	}

	return count;
}

int getInputData2(Block2*& block2, string input_str) {

	//Get input data from .csv
	ifstream input_data(input_str.c_str());
	string buf;
	Tokenizer tokenizer; //include "Tokenize.h"
	int numOfRecords = BLOCKSIZE / sizeof(Professors);
	tokenizer.setDelimiter(","); //parsing Delimiter = ","
	getline(input_data, buf);
	tokenizer.setString(buf);
	int count = atoi(tokenizer.next().c_str()); //the num of Professors

	block2 = new Block2[count / numOfRecords + 1];

	//initialize blocks
	for (int j = 0; j < count / numOfRecords + 1; j++) {
		for (int i = 0; i < numOfRecords; i++) {
			strcpy(block2[j].records[i].name, "");
			block2[j].records[i].professorID = 0;
			block2[j].records[i].salary = 0;
		}
	}

	//read inputFile and then put value into blocks
	for (int j = 0; j < count / numOfRecords + 1; j++) {
		for (int i = 0; i < numOfRecords; i++) {

			string temp;
			getline(input_data, buf);
			tokenizer.setString(buf);
			if (input_data.eof()) break;

			strncpy(block2[j].records[i].name, tokenizer.next().c_str(), sizeof(block2[j].records[i].name));
			block2[j].records[i].professorID = atoi(tokenizer.next().c_str());
			block2[j].records[i].salary = atoi(tokenizer.next().c_str());
		}
	}

	return count;
}

int writeIndexFile(FILE *& fout)
{
	fseek(fout, 0, SEEK_SET);
	Node * p = root;

	if (p->type == INDEX)
		for (; p->node.indexNode.pointer[0] != NULL; p = p->node.indexNode.pointer[0]);

	while (1)
	{
		fwrite((void*)p, sizeof(Node), 1, fout);

		if (p->node.leafNode.next != NULL)
			p = p->node.leafNode.next;
		else
			break;
	}
	return 0;
}

int writeIndexFile2(FILE *& fout)
{
	fseek(fout, 0, SEEK_SET);
	Node * p = root;

	if (p->type == INDEX)
		for (; p->node.indexNode.pointer[0] != NULL; p = p->node.indexNode.pointer[0]);

	while (1)
	{
		fwrite((void*)p, sizeof(Node), 1, fout);

		if (p->node.leafNode.next != NULL)
			p = p->node.leafNode.next;
		else
			break;
	}
	return 0;
}

//if (p->type == LEAF)
//{
//	for (i = 0; i < DEGREE; i++)
//	{
//		if (p->node.indexNode.key[i] != 0)
//			printf("%.1f ", p->node.leafNode.key[i]);
//	}
//	printf("\n");
//}
//else
//{
//	for (i = 0; i < DEGREE - 1; i++)
//	{
//		if (p->node.indexNode.key[i] != 0)
//			printf("%.1f ", p->node.indexNode.key[i]);
//	}
//	printf("\n");
//	for (i = 0; i < DEGREE - 1; i++)
//	{
//		if (p->node.indexNode.pointer[i] != NULL)
//		{
//			traverse(p->node.indexNode.pointer[i], depth + 1);
//		}
//	}
//	if (p->node.indexNode.pointer[i] != NULL)
//	{
//		traverse(p->node.indexNode.pointer[i], depth + 1);
//	}
//	printf("\n");
//}






//for(i=0; i<sizeof(in)/4; i++)
//{
// InsertKey(in[i], rand());
//}

//for (i = 0; i<10000; i++)
//{
//	InsertKey(i, rand());
//}
//for (i = 0; i<10000; i++)
//{
//	DeleteKey(i);
//}
//for (float i = 1; i <= 100; i++)
//{
//	InsertKey(i, 999);
//}
//printf("SelectKey(19) ����\n");
//SelectKey(19);
//printf("SelectKey(7) ����\n");
//SelectKey(7);
//printf("DeleteKey(7) ����\n");
//DeleteKey(7);
//printf("SelectKey(7) ����\n");
//SelectKey(7);
//printf("SelectKey(6) ����\n");
//SelectKey(6);
//printf("InsertKey(6, 1234) ����\n");
//InsertKey(6, 1234);
//printf("SelectKey(6) ����\n");
//SelectKey(6);




//DEGREE 7�� �ϰ�
//float in[33] = { 69, 7, 150, 19, 70, 20, 128, 18, 42, 120, 140, 16, 100, 26, 145, 15, 110, 30, 40, 132, 138, 50, 43, 130, 41, 59, 54, 122, 124, 35, 37, 39, 129 };
//for (int i = 0; i < 33; i++)
//{
//	InsertKey(in[i], 999);
//}
