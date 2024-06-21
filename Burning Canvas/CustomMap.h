#pragma once
#include "GLOBAL.h"
//#include "Tool.h"
#include "data.h"

enum DrawerState {
	DRAWDOT,
	ERASE,
	FILL,//
	LINE,
	SQURA,
	SQURAF,
	CIRC,
	CIRCF,
	SELECTOR,
	MOVE
};

class CMap : public Map {
public:
	CMap();
	CMap(const CMap* map);
	~CMap();
	void setSize(int w_, int h_);
	void setPalette(vector<ImVec4>* palette_ptr);
	vector<ImVec4> getPalette();
	void randomDraw();
	void randomStep(int maxstep, int minstep);
	string& getNow();
	void setGoalward(int ward_);

	bool R_Burn(set<pair<int,int>>& select, set<pair<int, int>>& avail);
	void LoadMapFromJson(Json::Value R);
	friend class CustomManager;
};

class CustomManager {
public:
	CustomManager();
	~CustomManager();
	void reset();
	void Random();
	void R_Random();
	CMap* GetrandomMap(int w, int h, vector<ImVec4>* palette, int maxstep, int minstep);
	void LoadMapFromImage(IMAGE_ img);
	void setBegin(CMap* begin);
	void setEnd(CMap* end);
	void toBegin();
	void toEnd();
	CMap*& getBegin();
	CMap*& getEnd();
	void SaveMap(string path, string name, int ID, bool isRR = 0);
	void SaveMap(string path, CMap* map);
	void EndCurrentToBeginGoal();// ��end��ͼ�ĵ�ǰ��ͼ����Ϊbegin��ͼ��Ŀ��ͼ��
	int w = 10;//3~20
	int h = 10;//3~20
	vector<ImVec4> palette;
	int maxstep = 1;//2~400
	int minstep = 10;//1~400

	//MapDisplay* md;//DEBUG
	//int cx, cy;//DEBUG
	int step=10;//2~399
	int checkMap();//��⴫���ͼ�Ƿ�Ϸ�
	bool checkPalette();//��⴫��ɫ���Ƿ�Ϸ�
	void RateRandom(int level);//	RATEģʽ�������Ѷ����ɵ�ͼ
	vector<ImVec4> randomPalette(int x);//	�ӱ�׼ɫ�̿��л�ȡ���ɫ�̣���֤���������ɫ���ڽӽ��������
	//bool vaild=0;//�Ƿ�Ϊ�Ϸ���ͼ
private:
	CMap* begin=nullptr;
	CMap* end=nullptr;
};

struct DrawerStep {
	CMap* map;
	vector<ImVec4> palette;
	int colorSelectedIndex;
};

class Drawer {
public:
	Drawer();
	~Drawer();
	int getIndex();//ɫ��
	void setIndex(int i);

	DrawerState getState();
	DrawerState getSubState();
	void setState(DrawerState state_);
	void setSubState(DrawerState state_);
	
	void colorsShift(int index);
	void removeInvaildColors();
	void draw(int x, int y, int index);
	void line(int index);
	void squra(int index);
	void squraf(int index);
	void circ(int index);
	void circf(int index);
	void fill(int x, int y, int index);

	void setBeginPos(ImVec2 pos);
	ImVec2 getBeginPos();
	void setEndPos(ImVec2 pos);
	ImVec2 getEndPos();
	bool getVaild();
	void resetVaild();
	
	void saveCurrentStep();
	bool canBack();
	bool canNext();
	void save();
	void retreat();
	void resave();
	void reretreat();
	void clearNextStep();

	void clearMemory();
	float cx, cy;//heigth/2,width/2
	float size;//height*0.8
	float k;//10->1
	CMap* map;//
	vector<ImVec4> palette;//
	bool displaySubToolBar;
private:
	ImVec2 beginPos, endPos;//
	DrawerStep tmpStep;
	stack<DrawerStep> back, forward;//	����ջ������ջ
	bool vaild;
	int colorSelectedIndex;
	DrawerState state = MOVE;
	DrawerState substate = LINE;//	���߼���state
};

std::vector<ImVec4> kMeans(CMap* MAP, int K, std::vector<ImVec4> userProvided = std::vector<ImVec4>(0), int maxIterations = 1000, double epsilon = 0.0005);

Json::Value SaveMapAsJson(CMap* map);