#pragma once
#include"GLOBAL.h"
#include<string>

enum HintState {
	NONE,
	CLOCK,
	ANTICLOCK,
	BURN,
	RETREAT,
	SUBMIT
};

class Map {
public:
	Map();
	Map(std::wstring name);
	Map(const Map* mp);
	~Map();
	void load(std::wstring name);
	std::string getName();
	void getSize(int& x, int& y);
	ImVec4 getColors(int x, int y);
	ImVec4 getGoalColors(int x, int y);
	int getColors_index(int x, int y);
	int getNumbers(int x, int y);
	int getward();
	void setColors(int x, int y, int c);
	void setNumbers(int x, int y, int n);
	void clockwise();
	void anticlockwise();
	bool Burn();
	HintState Hint();
	bool Submit();
	void operationInsert(char move);
private:
	//Map* last_status;
	std::string ans,now;
	int ward = 0;//0 ~ 3
	std::string name;
	int w, h;
	int colorCnt;
	ImVec4* palette;
	int** colors;
	int** numbers;
	int** goalColors;
};

class MapDisplay{
public:
	MapDisplay();
	MapDisplay(Map* mp);
	void setCenter(int x, int y);
	void setSize(int sz);
	int getSize();
	void setMap(Map* _map);
	void display();
	void displayFinish();
	~MapDisplay();
	Map* myMap;
private:
	int cx, cy;
	int size;
	bool nonumber;
};