#include "data.h"

Map::Map() {}
Map::Map(std::wstring s) {
	load(s);
}

Map::Map(const Map* mp) {
	ward = mp->ward;
	name = mp->name;
	w = mp->w;
	h = mp->h;
	colorCnt = mp->colorCnt;
	palette = new ImVec4[colorCnt];
	for (int i = 0; i < colorCnt; i++) {
		palette[i] = mp->palette[i];
	}

	colors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		colors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			colors[i][j] = mp->colors[i][j];
		}
	}

	numbers = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		numbers[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			numbers[i][j] = mp->numbers[i][j];
		}
	}

	goalColors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		goalColors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			goalColors[i][j] = mp->goalColors[i][j];
		}
	}

	now = mp->now;
	ans = mp->ans;
}
void Map::load(std::wstring s) {
	std::ifstream ifs((L"Map\\" + s).c_str());
	ifs >> name >> w >> h;

	ifs >> colorCnt;

	palette = new ImVec4[colorCnt];
	for (int i = 0; i < colorCnt; i++) {
		int r, g, b;
		ifs >> r >> g >> b;
		palette[i] = ImVec4(r / 255.0, g / 255.0, b / 255.0, 1);
	}

	colors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		colors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			ifs >> colors[i][j];
		}
	}

	numbers = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		numbers[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			ifs >> numbers[i][j];
		}
	}

	goalColors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		goalColors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			ifs >> goalColors[i][j];
		}
	}

	ifs >> ans;

	ifs.close();
}

std::string Map::getName() {
	return name;
}

void Map::getSize(int& x, int& y) {
	x = w; y = h;
}

void Map::operationInsert(char move) {
	now += move;
	bool modify = 1;
	while (modify) {
		modify = 0;
		if (now.length() >= 2) {
			string last = now.substr(now.length() - 2);
			if (last == "AC" || last == "CA" || last == "RR") {
				now = now.substr(0, now.length() - 2);
				modify = 1;
				continue;
			}
			if (last == "AA" || last == "CC") {
				now = now.substr(0, now.length() - 2)+'R';
				modify = 1;
				continue;
			}
			if (last == "RA") {
				now = now.substr(0, now.length() - 2) + 'C';
				modify = 1;
				continue;
			}
			if (last == "RC") {
				now = now.substr(0, now.length() - 2) + 'A';
				modify = 1;
				continue;
			}
		}
	}
	cout << now << "\n";
}

void Map::anticlockwise() {
	cout << ward;
	ward++;
	ward %= 4;
	operationInsert('A');
}

void Map::clockwise() {
	ward--;
	ward = (ward + 4) % 4;
	operationInsert('C');
}

bool Map::Burn() {
	int w_ = w, h_ = h;
	if (ward & 1)swap(w_, h_);
	bool change = 0;
	for (int y = h_; y >= 1; y--) {
		for (int x = 1; x <= w_; x++) {
			if (getNumbers(x, y) > 1)change = 1;
			if (y == h_) {
				setNumbers(x, y, 1);
				continue;
			}
			if (getNumbers(x, y) > 1) {
				setColors(x, y + 1, getColors_index(x, y));
				setNumbers(x, y + 1, getNumbers(x, y) - 1);
				setNumbers(x, y, 1);
			}
		}
	}
	if (change) {
		operationInsert('B');
		return 1;
	}
	return 0;
}

HintState Map::Hint() {
	if (now == ans)return SUBMIT;
	else if (now.length() > ans.length())return RETREAT;
	else {
		if (now == "") {
			if (ans[0] == 'A')return ANTICLOCK;
			else if (ans[0] == 'C' || ans[0] == 'R')return CLOCK;
			else return BURN;
		}
		for (int i = 0; i < now.length() - 1; i++) {
			if (now[i] != ans[i]) {
				return RETREAT;
			}
		}
		char ans_ = ans[now.length() - 1];
		char now_ = now[now.length() - 1];
		if (ans_ == now_ && ans.length() > now.length()) {
			char nxt = ans[now.length()];
			if (nxt == 'R')return CLOCK;
			else if (nxt == 'A')return ANTICLOCK;
			else if (nxt == 'C')return CLOCK;
			else return BURN;
		}

		if (ans_ == 'R') {
			if (now_ == 'A')return ANTICLOCK;
			else if (now_ == 'C')return CLOCK;
			else return RETREAT;
		}
		if (now_ == 'R') {
			if (ans_ == 'A')return CLOCK;
			else if (ans_ == 'C')return ANTICLOCK;
			else return RETREAT;
		}
		if (now_ == 'B')return RETREAT;
		return CLOCK;
	}
}

bool operator!=(const ImVec4 a, const ImVec4 b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

bool Map::Submit() {
	if (ans == now)return 1;
	else {
		if ((ward & 1) && w != h)return 0;
		for (int y = h; y >= 1; y--) {
			for (int x = 1; x <= w; x++) {
				if (getNumbers(x, y) > 1 || getColors(x, y) != getGoalColors(x, y))return 0;
			}
		}
	}
	return 1;
}

ImVec4 Map::getColors(int x, int y) {
	if (ward == 0)return palette[colors[y][x]];
	if (ward == 1)return palette[colors[x][w - y + 1]];
	if (ward == 2)return palette[colors[h - y + 1][w - x + 1]];
	return palette[colors[h - x + 1][y]];
}
int Map::getColors_index(int x, int y) {
	if (ward == 0)return colors[y][x];
	if (ward == 1)return colors[x][w - y + 1];
	if (ward == 2)return colors[h - y + 1][w - x + 1];
	return colors[h - x + 1][y];
}
ImVec4 Map::getGoalColors(int x, int y) {
	return palette[goalColors[x][y]];
}
int Map::getNumbers(int x, int y) {
	if (ward == 0)return numbers[y][x];
	if (ward == 1)return numbers[x][w - y + 1];
	if (ward == 2)return numbers[h - y + 1][w - x + 1];
	return numbers[h - x + 1][y];
}
int Map::getward() {
	return ward;
}
void Map::setColors(int x, int y,int c) {
	if (ward == 0) {
		colors[y][x]=c;
	}
	else if (ward == 1) {
		colors[x][w - y + 1]=c;
	}
	else if (ward == 2) {
		colors[h - y + 1][w - x + 1]=c;
	}
	else colors[h - x + 1][y]=c;
}
void Map::setNumbers(int x, int y,int n) {
	if (ward == 0) {
		numbers[y][x] = n;
	}
	else if (ward == 1) {
		numbers[x][w - y + 1] = n;
	}
	else if (ward == 2) {
		numbers[h - y + 1][w - x + 1] = n;
	}
	else numbers[h - x + 1][y] = n;
}
Map::~Map() {
	delete[] palette;
	delete[] colors;
	delete[] numbers;
	delete[] goalColors;
}

MapDisplay::MapDisplay() {}
MapDisplay::MapDisplay(Map* mp) {
	myMap = mp;
}
void MapDisplay::setCenter(int x, int y) {
	cx = x, cy = y;
}

void MapDisplay::setSize(int sz) {
	size = sz;
}

int MapDisplay::getSize() {
	return size;
}

void MapDisplay::setMap(Map* _map) {
	myMap = _map;
}

void MapDisplay::display() {
	ImVec4 tmpcolor;
	std::string tmpnum;
	int w, h;
	myMap->getSize(w, h);
	if (myMap->getward() & 1)swap(w, h);
	int maxLen = max(h, w);
	float k = 1.0 * size / maxLen;//放缩比例系数
	float kw = k * w;
	float kh = k * h;
	int ks = kw / w;
	
	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			tmpcolor = myMap->getColors(x,y);
			drawRect(cx - kw / 2 + (x - 1) * ks, cy - kh / 2 + (y - 1) * ks, ks, ks, tmpcolor);
		}
	}
	if (nonumber)return;

	ImVec4 fontcolor = ImVec4(0, 0, 0, 1);

	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			tmpnum = std::to_string(myMap->getNumbers(x,y));
			drawText(int(cx - kw / 2 + (x - 1) * ks + 0.5 * ks), int(cy - kh / 2 + (y - 1) * ks + 0.5 * ks), ks*0.7, fontcolor, tmpnum);
		}
	}
	if (myMap->getward() & 1)swap(w, h);
}
void MapDisplay::displayFinish() {
	ImVec4 tmpcolor;
	std::wstring tmpnum;
	int w, h;
	myMap->getSize(w, h);
	int maxLen = max(h, w);
	float k = 1.0 * size / maxLen;//放缩比例系数
	float kw = k * w;
	float kh = k * h;
	int ks = kw / w;

	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			tmpcolor = myMap->getGoalColors(y,x);
			drawRect(cx - kw / 2 + (x - 1) * ks, cy - kh / 2 + (y - 1) * ks, ks, ks, tmpcolor);
		}
	}
}
MapDisplay::~MapDisplay() {
	//delete myMap;
}