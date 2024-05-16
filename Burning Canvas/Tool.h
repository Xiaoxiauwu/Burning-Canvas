#pragma once
#include "GLOBAL.h"
#include <string>
using namespace std;
void setFont(ImFont* F);

void drawRect(int x, int y, int w, int h, ImVec4 color);
void drawText(float x, float y, float size, ImVec4 color, string text);

class ScoreBoard {
public:
	ScoreBoard();
	~ScoreBoard();
	void Reset();
	int GetTime();
	int calcScore();
	void step();
	void regret();
	void hint();
	void submit();
private:
	int baseTime;
	int stepCount;
	int regretCount;
	int hintCount;
	int submitFailedCount;
};

string sec2time(int sec);