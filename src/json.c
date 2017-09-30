#include "json.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int tmem = 0;

#define newArray(arr, len) char* arr = malloc(len)
#define newObj(OBJ) newObject(sizeof(OBJ))

static J_V* J_newV(J_Type type, void* value);

static void* newObject(int size)
{
	tmem += size;
	return malloc(size);
}
static void* newStr(char* begin, char* end)
{
	int len = end? end - begin : strlen(begin);
	char* m = malloc(len + 1);
	memcpy(m, begin, len);
	m[len] = 0;
	tmem += len+1;
	return m;
}

/* ********************************************
 * e: empty
 *
 * ×óµÝ¹é£º
 * A--> A c | r
 * ÓÒµÝ¹é£º
 * A--> r R
 * R--> c R | e
 *
 * JSON Óï·¨:
 * ËµÃ÷£ºE:extend K:key V:value KV:key-value
 * JSON     --> J_object | J_array
 * J_object --> {ObjBody}
 * ObjBody  --> KV EKV | e
 * EKV      --> ,KV EKV | e
 * KV       --> K:V
 * K        --> J_string
 * V        --> J_number | J_string | J_boolean | J_array | J_object
 * J_number --> +digit decimal | -digit decimal | digit decimal
 * decimal  --> .digit | e
 * J_string --> "..."
 * J_boolean--> true | false
 * J_array  --> [ArrayBody]
 * ArrayBody--> V EV | e
 * EV       --> ,V EV | e
**********************************************/

char* ep = NULL;

char* getError()
{
	return ep;
}

#define SET_EP(STR) do{if(!ep){ ep = str;printf("error line: %d\n",__LINE__);}}while(0)

static void* J_matchArray(char* str, char** o_rstr);
static void* J_matchObject(char* str, char** o_rstr);

#define SKIP_SPACE(STR) while((STR)[0]==' '||(STR)[0]=='	') (STR)++
#define SKIP_EMPTYLINE(STR) while((STR)[0]=='\n' || (STR)[0]==' ' || (STR)[0]=='	' ) (STR)++

static const char* jboolean[] = {"false","true"};

static J_Type J_typeCheck(char ch)
{
	switch(ch)
	{
		case '"':
			return J_string;
		case '[':
			return J_array;
		case 't':
		case 'f':
			return J_bool;
		case 'n':
			return J_null;
		case '{':
			return J_object;
		default:
			if ((ch >= '0' && ch <= '9') || ch == '-')
				return J_number;
			break;
	}
	return J_unkonw;
}

static char* J_matchNull(char* str)
{
	if(memcmp(str, "null", 4) == 0)
		return str+4;
	return NULL;
}

static char* J_matchBool(char* str)
{
	if(memcmp(str, "true", 4) == 0)
		return str+4;
	if(memcmp(str, "false", 5) == 0)
		return str+5;
	SET_EP(str);
	return NULL;
}

static char* J_matchString(char* str)
{
	do
	{
		str++;
		if(*str == '\\')
		{
			if(str[1]=='\n')
				break;
			str+=2;
		}
		if(*str == '"')
			return str+1;
	}while(*str != '\n');
	SET_EP(str);
	return NULL;
}

static char* J_matchNumber(char* str)
{
	char* pstr=str;
	while(*str>='0' && *str<='9') str++;
	if(str[0] == '.')
	{
		pstr=str;
		while(*str>='0' && *str<='9') str++;
	}
	if(str == pstr)
	{
		SET_EP(str);
		return NULL;
	}
	return str;
}

static J_K* J_matchK(char* str, char** o_rstr)
{
	char* rstr = NULL;
	J_K* j_k = newObj(J_K);
	if(J_typeCheck(*str) != J_string)
	{
		SET_EP(str);
		*o_rstr = rstr;
		return NULL;
	}
	*o_rstr = rstr = J_matchString(str);
	if(rstr)
		j_k->p = newStr(str+1, rstr-1);
	return j_k;
}

static J_V* J_matchV(char* str, char** o_rstr)
{
	char* rstr = NULL;
	J_V* r_jv;
	switch(J_typeCheck(str[0]))
	{
		case J_string:
			r_jv = newObj(J_V);
			r_jv->type = J_string;
			*o_rstr = rstr = J_matchString(str);
			if(rstr)
			{
				newArray(arr, rstr-str-1);
				strncpy(arr, str+1, rstr-str-1);
				r_jv->value.p = newStr(str+1, rstr-1);
				return r_jv;
			}
			break;
		case J_bool:
			r_jv = newObj(J_V);
			r_jv->type = J_bool;
			*o_rstr = rstr = J_matchBool(str);
			if(rstr)
			{
				r_jv->value.jbool = str[0] == 't';
				return r_jv;
			}
			break;
		case J_number:
			r_jv = newObj(J_V);
			r_jv->type = J_number;
			*o_rstr = rstr = J_matchNumber(str);
			if(rstr)
			{
				r_jv->value.i_num = atoi(str);
				return r_jv;
			}
			break;
		case J_null:
			r_jv = newObj(J_V);
			r_jv->type = J_null;
			*o_rstr = rstr = J_matchNull(str);
			if(rstr)
			{
				r_jv->value.p = NULL;
				return r_jv;
			}
			break;
		case J_array:
			r_jv = J_matchArray(str, &rstr);
			*o_rstr = rstr;
			if(rstr)
				return r_jv;
			break;
		case J_object:
			r_jv = J_matchObject(str, &rstr);
			*o_rstr = rstr;
			if(rstr)
				return r_jv;
			break;
		default:
			break;
	}
	SET_EP(str);
	*o_rstr = rstr;
	return NULL;
}

static J_KV* J_matchKV(char* str, char** o_rstr)
{
	char* rstr;
	J_KV* r_kv = newObj(J_KV);
	r_kv->k = J_matchK(str, &rstr);
	if(!rstr)
	{
		SET_EP(str);
		*o_rstr = rstr;
		return r_kv;
	}
	SKIP_SPACE(rstr);
	if(*rstr++ != ':')
	{
		SET_EP(rstr);
		*o_rstr = NULL;
		return r_kv;
	}
	SKIP_SPACE(rstr);
	r_kv->v = J_matchV(rstr, &rstr);
	*o_rstr = rstr;
	return r_kv;
}

static J_KV* J_matchEKV(char* str, char** o_rstr)
{
	J_KV* j_kv = NULL;
	char* rstr = str;
	if(*rstr != ',')
	{
		*o_rstr = rstr;
		return j_kv;
	}
	rstr++;
	SKIP_EMPTYLINE(rstr);
	j_kv = J_matchKV(rstr, &rstr);
	if(!rstr)
	{
		SET_EP(rstr);
		*o_rstr = rstr;
		return j_kv;
	}
	SKIP_SPACE(rstr);
	j_kv->next = J_matchEKV(rstr, &rstr);
	*o_rstr = rstr;
	return j_kv;
}

static J_KV* J_matchObjBody(char* str, char** o_rstr)
{
	char* rstr;
	J_KV* j_kv = J_matchKV(str, &rstr);
	if(!rstr)
	{
		*o_rstr = str+1;
		return j_kv;
	}
	SKIP_EMPTYLINE(rstr);
	j_kv->next = J_matchEKV(rstr, &rstr);
	*o_rstr = rstr;
	return j_kv;
}

static void* J_matchObject(char* str, char** o_rstr)
{
	char* rstr = str;
	J_V* json = newObj(J_V);
	json->type = J_object;
	rstr++;
	SKIP_EMPTYLINE(rstr);
	if(*rstr == '}')
	{
		json->value.p = NULL;
		*o_rstr = rstr + 1;
		return json;
	}
	json->value.p = J_matchObjBody(rstr, &rstr);
	if(rstr)
	{
		SKIP_EMPTYLINE(rstr);
		if(*rstr != '}')
		{
			SET_EP(rstr);
			*o_rstr = NULL;
			return json;
		}
		*o_rstr = rstr+1;
		return json;
	}
	SET_EP(rstr);
	*o_rstr = NULL;
	return json;
}

static J_EV* J_matchEV(char* str, char** o_rstr)
{
	J_EV* j_ev = NULL;
	char* rstr = str;
	if(*rstr != ',')
	{
		*o_rstr = rstr;
		return j_ev;
	}
	j_ev = newObj(J_EV);
	rstr++;
	SKIP_EMPTYLINE(rstr);
	j_ev->v = J_matchV(rstr, &rstr);
	if(!rstr)
	{
		SET_EP(rstr);
		*o_rstr = rstr;
		return j_ev;
	}
	SKIP_EMPTYLINE(rstr);
	j_ev->next = J_matchEV(rstr, &rstr);
	*o_rstr = rstr;
	return j_ev;
}

static J_EV* J_matchArrayBody(char* str, char** o_rstr)
{
	J_EV* j_ev = newObj(J_EV);
	char* rstr;
	j_ev->v = J_matchV(str, &rstr);
	if(!rstr)
	{
		SET_EP(str);
		*o_rstr = rstr;
		return j_ev;
	}
	SKIP_EMPTYLINE(rstr);
	j_ev->next = J_matchEV(rstr, &rstr);
	*o_rstr = rstr;
	return j_ev;
}

static void* J_matchArray(char* str, char** o_rstr)
{
	char* rstr = str;
	J_V* j_v = newObj(J_V);;
	j_v->type = J_array;
	rstr++;
	SKIP_EMPTYLINE(rstr);
	if(*rstr == ']')
	{
		*o_rstr = rstr+1;
		j_v->value.p = NULL;
		return j_v;
	}
	j_v->value.p = J_matchArrayBody(rstr, &rstr);
	if(!rstr)
	{
		SET_EP(rstr);
		*o_rstr = rstr;
		return j_v;
	}
	SKIP_EMPTYLINE(rstr);
	if(*rstr == ']')
	{
		*o_rstr = rstr+1;
		return j_v;
	}
	*o_rstr = NULL;
	SET_EP(rstr);
	return j_v;
}

static J_V* J_matchJson(char* str, char** o_rstr)
{
	switch(J_typeCheck(*str))
	{
		case J_array:
		case J_object:
		case J_null:
			return J_matchV(str, o_rstr);
		default:
			break;
	}
	SET_EP(str);
	*o_rstr = NULL;
	return NULL;
}

void* J_parser(char* str)
{
	J_V* json;
	char* rstr = str;
	SKIP_EMPTYLINE(rstr);
	json = J_matchJson(rstr, &rstr);
	if(!rstr)
		return json;
	SKIP_EMPTYLINE(rstr);
	if(*rstr != 0)
		return json;
	return json;
}


/*****************************************************************************/

int newCountV = 0;
int newMemV = 0;
int newCountK = 0;
int newMemK = 0;
int newCountKV= 0;

static J_K* J_newK(char* key)
{
	J_K* k = newObj(J_K);
	k->p = newStr(key, NULL);
	newMemK += strlen(key) + 1;
	newCountK++;
	return k;
}

static J_V* J_newV(J_Type type, void* value)
{
	J_V* v = NULL;
	newCountV++;
	switch(type)
	{
		case J_null:
			v = newObj(J_V);
			v->type = type;
			break;
		case J_object:
			if(value)
			{
				
				newCountV--;
				return value;
			}
			v = newObj(J_V);
			v->type = type;
			v->value.p = NULL;
			break;
		case J_array:
			if(value)
			{
				newCountV--;
				return value;
			}
			v = newObj(J_V);
			v->type = type;
			v->value.p = NULL;
			break;
		case J_string:
			v = newObj(J_V);
			v->type = type;
			v->value.p = newStr(value, NULL);
			newMemV+=strlen(v->value.p)+1;
			break;
		case J_number:
			v = newObj(J_V);
			v->type = type;
			v->value.i_num = *(int*)value;
			break;
		case J_bool:
			v = newObj(J_V);
			v->type = type;
			v->value.jbool = *(int*)value;
			break;
		default:
			newCountV--;
			break;
	}
	return v;
}

static int objectAddKV(void* json, char* key, void* value, J_Type valueType)
{
	J_V* jv = json;
	J_KV* j_kv;
	if(!jv || jv->type != J_object)
		return -1;
	newCountKV++;
	j_kv = newObj(J_KV);
	j_kv->k = J_newK(key);
	j_kv->v = J_newV(valueType, value);
	j_kv->next = jv->value.p;
	jv->value.p = j_kv;
	return 0;
}

int objectAddNString(void* json, char* key, char* value, int len)
{
	newArray(str, len+1);
	strncpy(str, value, len);
	return objectAddString(json, key, str);
}

int objectAddString(void* json, char* key, char* value)
{
	return objectAddKV(json, key, value, J_string);
}

int objectAddInt(void* json, char* key, int num)
{
	return objectAddKV(json, key, &num, J_number);
}

int objectAddBool(void* json, char* key, int jbool)
{
	return objectAddKV(json, key, &jbool, J_bool);
}

int objectAddNull(void* json, char* key)
{
	return objectAddKV(json, key, NULL, J_null);
}

int objectAddObject(void* json, char* key, void* value)
{
	return objectAddKV(json, key, value, J_object);
}

int objectAddArray(void* json, char* key, void* value)
{
	return objectAddKV(json, key, value, J_array);
}

static int arrayAppend(void* array, void* value, J_Type type)
{
	J_V* jv = array;
	J_EV* j_ev;
	if(!jv || jv->type != J_array)
		return -1;
	newCountKV++;
	j_ev = newObj(J_EV);
	j_ev->v = J_newV(type, value);
	j_ev->next = jv->value.p;
	jv->value.p = j_ev;
	return 0;
}

int arrayAppendNString(void* array, char* value, int len)
{
	newArray(str, len+1);
	strncpy(str, value, len);
	return arrayAppendString(array, str);
}

int arrayAppendString(void* array, char* value)
{
	return arrayAppend(array, value, J_string);
}

int arrayAppendInt(void* array, int num)
{
	return arrayAppend(array, &num, J_number);
}

int arrayAppendBool(void* array, int jbool)
{
	return arrayAppend(array, &jbool, J_bool);
}

int arrayAppendNull(void* array)
{
	return arrayAppend(array, NULL, J_null);
}

int arrayAppendObject(void* array, char* value)
{
	return arrayAppend(array, value, J_object);
}
int arrayAppendArray(void* array, char* value)
{
	return arrayAppend(array, value, J_array);
}

void* newJson(J_Type type)
{
	J_V* json = NULL;
	switch(type)
	{
		case J_null:
		case J_object:
		case J_array:
			json = J_newV(type, NULL);
			break;
		default:
			break;
	}
	return json;
}

/*****************************************************************************/

static void jsonAarrayShow(J_EV* ev)
{
	if(!ev)
		return;
	while(ev)
	{
		jsonTraver(ev->v);
		ev=ev->next;
		if(ev)	printf(", ");
	}
}

static void jsonObjShow(J_KV* kv)
{
	if(!kv)
		return;
	while(kv)
	{
		printf("'%s' : ", kv->k->p);
		jsonTraver(kv->v);
		kv = kv->next;
		if(kv)	printf(", \n");
	}
}

void jsonTraver(void* p)
{
	J_V* json = p;
	if(!json)
		return;
	switch(json->type)
	{
		case J_object:
			printf("{");
			jsonObjShow(json->value.p);
			printf("}");
			break;
		case J_array:
			printf("[");
			jsonAarrayShow(json->value.p);
			printf("]");
			break;
		case J_number:
			printf("%d", json->value.i_num);
			break;
		case J_string:
			printf("'%s'", (char*)json->value.p);
			break;
		case J_bool:
			printf("'%s'", jboolean[json->value.jbool!=0]);
			break;
		case J_null:
			printf("null");
			break;
		default:
			printf("unknow!\n");
			break;
	}
}

