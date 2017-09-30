#ifndef __JSON_H__
#define __JSON_H__

typedef enum
{
	J_unkonw=0,
	J_string,
	J_bool,
	J_number,
	J_null,
	J_array,
	J_object,
}J_Type;

typedef struct
{
	J_Type type;
	union
	{
		int i_num;
		double f_num;
		int jbool;
		void* p;
	}value;
}J_V;

typedef struct
{
	char* p;
}J_K;

typedef struct _J_KV J_KV;
typedef J_KV J_EV;
struct _J_KV
{
	J_K* k;
	J_V* v;
	J_KV* next;
};

void* J_parser(char* str);

int objectAddNString(void* json, char* key, char* value, int len);
int objectAddString(void* json, char* key, char* value);
int objectAddInt(void* json, char* key, int num);
int objectAddBool(void* json, char* key, int jbool);
int objectAddNull(void* json, char* key);
int objectAddObject(void* json, char* key, void* value);
int objectAddArray(void* json, char* key, void* value);

int arrayAppendNString(void* array, char* value, int len);
int arrayAppendString(void* array, char* value);
int arrayAppendInt(void* array, int num);
int arrayAppendBool(void* array, int jbool);
int arrayAppendNull(void* array);
int arrayAppendObject(void* array, char* value);
int arrayAppendArray(void* array, char* value);

void* newJson(J_Type type);


void jsonTraver(void* p);


extern int tmem;

#endif
