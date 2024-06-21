#pragma once
#include"GLOBAL.h"


#include"Tool.h"
//#include<string>
class Map;

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
	Map(std::string name);
	Map(const Map* mp);
	~Map();
	void load(std::string name);
	void loadDefault();
	std::string& getName();
	int getID();
	void getSize(int& x, int& y);
	ImVec4 getColors(int x, int y);
	ImVec4 getGoalColors(int x, int y);
	int getColors_index(int x, int y);
	int getGoalColors_index(int x, int y);
	int getNumbers(int x, int y);
	int getward();
	int getGoalward();
	void setColors(int x, int y, int c);
	void setNumbers(int x, int y, int n);
	void clockwise();
	void anticlockwise();
	bool Burn();
	HintState Hint();
	bool Submit();
	void operationInsert(char move);
	std::string ans,now;//DEBUG 等会记得放回去
//
	//Map* last_status;
	bool isEditing = 0;//	正在被编辑
//protected:	//	全都公开
	int ward = 0;//0 ~ 3
	int goalward = 0;
	std::string name;
	int ID;
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
	void setSize(float sz);
	int getSize();
	void setMap(Map* _map);
	void display(bool nonumber = 0);
	void displayFinish();
	void showUnitBoard(int x, int y);
	void showUnitColor(int x, int y, ImVec4 color);
	bool isShowGrid();
	void setShowGrid(bool v);
	void setGridColor(ImVec4 color);
	~MapDisplay();
	Map* myMap;
private:
	int cx, cy;
	float size;
	bool nonumber;
	bool showGrid = 0;
	ImVec4 gridColor = ImVec4(0.5, 0.5, 0.5, 1);
};