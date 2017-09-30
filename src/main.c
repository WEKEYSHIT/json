#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "type.h"

char* getError();
u32 (getFileSize)(FILE* fp);

#define JSON_FILE "json.txt"

extern int newCountK, newMemK,newCountV, newMemV,newCountKV, newMemKV;

void calMem()
{
	int totalK = newCountK*sizeof(J_K)+newMemK;
	int totalV = newCountV*sizeof(J_V)+newMemV;
	int totalKV = newCountKV*sizeof(J_KV);
	int total = totalK + totalV + totalKV;
	printf("\nnewCountK %d newMemK %d totalK %d\n", newCountK, newMemK, totalK);
	printf("newCountV %d newMemV %d totalV %d\n", newCountV, newMemV, totalV);
	printf("newCountKV %d totalKV %d\n", newCountKV, totalKV);
	printf("total %d\n", total);
}

void jsonGenTest(void * j)
{
	void* json = newJson(J_object);
	

	void* arr = newJson(J_array);

	objectAddString(json, "a", "2");
	objectAddString(json, "b", "2");
	objectAddInt(json, "num1", 123);
	objectAddBool(json, "bool", 1);
	objectAddNull(json, "NULL");
	arrayAppendString(arr, "324");
	arrayAppendString(arr, "");
	arrayAppendObject(arr, j);
	arrayAppendNull(arr);

	objectAddObject(json, "json", j);

	objectAddObject(json, "j1", newJson(J_object));
	objectAddArray(json, "ARRAY", arr);
	jsonTraver(json);
	calMem();
}

u32 getFileSize(FILE* fp)
{
	u32 fileLen;
	if(fp == NULL)
		return 0;
	fseek(fp,0,SEEK_END);
	fileLen = ftell(fp);
	fseek(fp,0,SEEK_SET);
	return fileLen;
}

int main(int argc, char* argv[])
{
	char* json_txt = NULL;
	char* json_err = NULL;
	int jlen = 0;
	void* jret = NULL;
	char* jerror = NULL;
#if 1
	FILE* fp = fopen(JSON_FILE, "r");
	if(!fp)
	{
		printf("open %s error!\n", JSON_FILE);
		getchar();
		return -1;
	}
	jlen = getFileSize(fp);
	json_txt = malloc(jlen + 1);
	json_err = malloc(jlen + 100);
	jlen = fread(json_txt, 1, jlen, fp);
	json_txt[jlen] = 0;
	fclose(fp);
	printf("json data:\n%s\n", json_txt);
	jret = J_parser(json_txt);
	jerror = getError();
	if(jerror)
	{
		int errOff = jerror-json_txt;
		memcpy(json_err, json_txt, errOff);
		sprintf(json_err + errOff, "<---|%c|---> %s\n", *jerror, jerror+1);
		printf("json erro:\n%s\n", json_err);
	}
	else
	{
		printf("=======================================================\n");
		jsonTraver(jret);
		printf("\n=======================================================\n");
	}
	printf("json parse: %p %d %d\n", jret, tmem, jlen+1);
#endif
	jsonGenTest(jret);
	printf("\n=======================================================\n");
	printf("json parse: %p %d %d\n", jret, tmem, jlen+1);
	if(json_txt) free(json_txt);
	if(json_err) free(json_err);
	getchar();
	return 0;
}