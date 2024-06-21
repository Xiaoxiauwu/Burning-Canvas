#include "data.h"

Map::Map() {
	palette = nullptr;
	numbers = nullptr;
	ID = 0;
	w = 0;
	h = 0;
	goalColors = nullptr;
	colors = nullptr;
	colorCnt = 0;
	static int CNT = 0;
	CNT++;
}
Map::Map(std::string s) {
	load("Map\\"+s);
}

Map::Map(const Map* mp) {
	ward = mp->ward;
	name = mp->name;
	ID = mp->ID;
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

	goalward = mp->goalward;

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
void Map::load(std::string s) {
	Json::Value R;
	string rawInfo = readFileIntoString(s.c_str());
	R = readJsonFromString(DECODE(rawInfo));
	if (R == Json::nullValue) {
		ID = -1;
		return;
	}
	if (R["ID"] == Json::nullValue) {
		ID = -1;
		return;
	}
	//std::ifstream ifs((L"Map\\" + s).c_str());
	name = R["name"].asString();
	//cout << encode::UTF8_To_String(name);
	ID = R["ID"].asInt();
	w = R["x"].asInt();
	h = R["y"].asInt();
	colorCnt = R["colorCnt"].asInt();

	palette = new ImVec4[colorCnt];
	for (int i = 0; i < colorCnt; i++) {
		int r, g, b;
		r = R["palette"][i][0].asInt();
		g = R["palette"][i][1].asInt();
		b = R["palette"][i][2].asInt();
		palette[i] = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1);
	}

	colors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		colors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			colors[i][j]=R["colors"][i-1][j-1].asInt();
		}
	}

	numbers = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		numbers[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			numbers[i][j] = R["numbers"][i-1][j-1].asInt();
		}
	}

	if (R["goalward"] == Json::nullValue)R["goalward"] = 0;
	goalward = R["goalward"].asInt();

	goalColors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		goalColors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			goalColors[i][j] = R["goalColors"][i-1][j-1].asInt();
		}
	}

	ans = R["ans"].asString();
}

void Map::loadDefault() {
	name = "默认头像";
	ID = -1;
	w = 12;
	h = 12;
	colorCnt = 12;

	palette = new ImVec4[colorCnt];
	palette[0] = ImVec4(255 / 255.0f, 126 / 255.0f, 0 / 255.0f, 1);
	palette[1] = ImVec4(130 / 255.0f, 130 / 255.0f, 130 / 255.0f, 1);
	palette[2] = ImVec4(0 / 255.0f, 183 / 255.0f, 239 / 255.0f, 1);
	palette[3] = ImVec4(139 / 255.0f, 0 / 255.0f, 139 / 255.0f, 1);
	palette[4] = ImVec4(199 / 255.0f, 26 / 255.0f, 35 / 255.0f, 1);
	palette[5] = ImVec4(77 / 255.0f, 109 / 255.0f, 243 / 255.0f, 1);
	palette[6] = ImVec4(255 / 255.0f, 163 / 255.0f, 177 / 255.0f, 1);
	palette[7] = ImVec4(94 / 255.0f, 51 / 255.0f, 33 / 255.0f, 1);
	palette[8] = ImVec4(245 / 255.0f, 76 / 255.0f, 245 / 255.0f, 1);
	palette[9] = ImVec4(255 / 255.0f, 242 / 255.0f, 0 / 255.0f, 1);
	palette[10] = ImVec4(14 / 255.0f, 125 / 255.0f, 45 / 255.0f, 1);
	palette[11] = ImVec4(165 / 255.0f, 230 / 255.0f, 122 / 255.0f, 1);

	colors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		colors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			colors[i][j] = 0;
		}
	}

	numbers = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		numbers[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			numbers[i][j] = 0;
		}
	}

	goalward = 0;

	goalColors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		goalColors[i] = new int[w + 1];
	}
	goalColors[1][1] = 0; goalColors[1][2] = 0; goalColors[1][3] = 0; goalColors[1][4] = 0; goalColors[1][5] = 0; goalColors[1][6] = 0; goalColors[1][7] = 0; goalColors[1][8] = 0; goalColors[1][9] = 0; goalColors[1][10] = 0; goalColors[1][11] = 0; goalColors[1][12] = 0;
	goalColors[2][1] = 0; goalColors[2][2] = 0; goalColors[2][3] = 0; goalColors[2][4] = 0; goalColors[2][5] = 0; goalColors[2][6] = 2; goalColors[2][7] = 5; goalColors[2][8] = 5; goalColors[2][9] = 0; goalColors[2][10] = 0; goalColors[2][11] = 0; goalColors[2][12] = 0;
	goalColors[3][1] = 0; goalColors[3][2] = 0; goalColors[3][3] = 0; goalColors[3][4] = 0; goalColors[3][5] = 2; goalColors[3][6] = 5; goalColors[3][7] = 5; goalColors[3][8] = 5; goalColors[3][9] = 5; goalColors[3][10] = 0; goalColors[3][11] = 0; goalColors[3][12] = 0;
	goalColors[4][1] = 0; goalColors[4][2] = 0; goalColors[4][3] = 0; goalColors[4][4] = 0; goalColors[4][5] = 2; goalColors[4][6] = 5; goalColors[4][7] = 5; goalColors[4][8] = 5; goalColors[4][9] = 5; goalColors[4][10] = 0; goalColors[4][11] = 0; goalColors[4][12] = 0;
	goalColors[5][1] = 0; goalColors[5][2] = 0; goalColors[5][3] = 0; goalColors[5][4] = 0; goalColors[5][5] = 2; goalColors[5][6] = 6; goalColors[5][7] = 6; goalColors[5][8] = 6; goalColors[5][9] = 0; goalColors[5][10] = 0; goalColors[5][11] = 0; goalColors[5][12] = 0;
	goalColors[6][1] = 0; goalColors[6][2] = 0; goalColors[6][3] = 0; goalColors[6][4] = 0; goalColors[6][5] = 1; goalColors[6][6] = 6; goalColors[6][7] = 6; goalColors[6][8] = 6; goalColors[6][9] = 0; goalColors[6][10] = 0; goalColors[6][11] = 0; goalColors[6][12] = 0;
	goalColors[7][1] = 4; goalColors[7][2] = 4; goalColors[7][3] = 4; goalColors[7][4] = 4; goalColors[7][5] = 1; goalColors[7][6] = 6; goalColors[7][7] = 6; goalColors[7][8] = 7; goalColors[7][9] = 4; goalColors[7][10] = 7; goalColors[7][11] = 4; goalColors[7][12] = 4;
	goalColors[8][1] = 4; goalColors[8][2] = 4; goalColors[8][3] = 4; goalColors[8][4] = 11; goalColors[8][5] = 11; goalColors[8][6] = 9; goalColors[8][7] = 9; goalColors[8][8] = 11; goalColors[8][9] = 7; goalColors[8][10] = 7; goalColors[8][11] = 4; goalColors[8][12] = 4;
	goalColors[9][1] = 4; goalColors[9][2] = 4; goalColors[9][3] = 10; goalColors[9][4] = 11; goalColors[9][5] = 9; goalColors[9][6] = 9; goalColors[9][7] = 9; goalColors[9][8] = 9; goalColors[9][9] = 11; goalColors[9][10] = 10; goalColors[9][11] = 4; goalColors[9][12] = 4;
	goalColors[10][1] = 4; goalColors[10][2] = 4; goalColors[10][3] = 10; goalColors[10][4] = 11; goalColors[10][5] = 11; goalColors[10][6] = 11; goalColors[10][7] = 11; goalColors[10][8] = 11; goalColors[10][9] = 11; goalColors[10][10] = 10; goalColors[10][11] = 4; goalColors[10][12] = 4;
	goalColors[11][1] = 4; goalColors[11][2] = 4; goalColors[11][3] = 10; goalColors[11][4] = 10; goalColors[11][5] = 10; goalColors[11][6] = 7; goalColors[11][7] = 7; goalColors[11][8] = 10; goalColors[11][9] = 10; goalColors[11][10] = 10; goalColors[11][11] = 4; goalColors[11][12] = 4;
	goalColors[12][1] = 4; goalColors[12][2] = 4; goalColors[12][3] = 10; goalColors[12][4] = 10; goalColors[12][5] = 10; goalColors[12][6] = 10; goalColors[12][7] = 10; goalColors[12][8] = 10; goalColors[12][9] = 10; goalColors[12][10] = 10; goalColors[12][11] = 4; goalColors[12][12] = 4;
		//0   0   0   0   0   0   0   0   0   0   0   0
		//0   0   0   0   0   2   5   5   0   0   0   0
		//0   0   0   0   2   5   5   5   5   0   0   0
		//0   0   0   0   2   5   5   5   5   0   0   0
		//0   0   0   0   2   6   6   6   0   0   0   0
		//0   0   0   0   1   6   6   6   0   0   0   0
		//4   4   4   4   1   6   6   7   4   7   4   4
		//4   4   4   11	11	9   9   11	7   7   4   4
		//4   4   10	11	9   9   9   9   11	10	4   4
		//4   4   10	11	11	11	11	11	11	10	4   4
		//4   4   10	10	10	7   7   10	10	10	4   4
		//4   4   10	10	10	10	10	10	10	10	4   4
	//ans = "CBABABCBCBCBCBABBABABBABCBABRBBBA";//没必要
}

std::string& Map::getName() {
	return name;
}

int Map::getID() {
	return ID;
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
	//cout << now << "\n";
}

void Map::anticlockwise() {
	//cout << ward;
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
	while (isEditing);//	被占用时先等待//	应该不需要等太久

	isEditing = 1;
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
		isEditing = 0;
		return 1;
	}
	isEditing = 0;
	return 0;
}

HintState Map::Hint() {
	if (now == ans)return SUBMIT;//	如果玩家操作序列与标准答案相同，那么提示玩家可以提交了
	else if (now.length() > ans.length())return RETREAT;//	如果操作序列长度大于答案序列，那么提示玩家应该撤回
	else {
		if (now == "") {//	如果玩家没进行任何操作就提示，那么就检查答案序列第一个字符，告诉玩家下一步操作
			if (ans[0] == 'A')return ANTICLOCK;
			else if (ans[0] == 'C' || ans[0] == 'R')return CLOCK;
			else return BURN;
		}
		for (int i = 0; i < now.length() - 1; i++) {//	从头开始一一对比检查，如果发现不相等的情况，直接告诉用户应该撤回了
			if (now[i] != ans[i]) {
				return RETREAT;
			}
		}
		char ans_ = ans[now.length() - 1];//	分别获取玩家操作序列的最后一项，以及在答案中，这一项本应该是什么
		char now_ = now[now.length() - 1];
		if (ans_ == now_ && ans.length() > now.length()) {//	如果玩家和答案能对上，那么告诉玩家下一步要做什么
			char nxt = ans[now.length()];
			if (nxt == 'R')return CLOCK;
			else if (nxt == 'A')return ANTICLOCK;
			else if (nxt == 'C')return CLOCK;
			else return BURN;
		}
		//	否则就要告诉玩家该通过什么操作，才能与答案一致
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
		if (now_ == 'B')return RETREAT;//	玩家操作为燃烧，答案为旋转，那么玩家只能撤回了
		return CLOCK;//	不然的话，玩家操作就不是燃烧，通过让玩家顺时针操作，总有办法能让用户和答案对上
		// （比方说答案如果是燃烧，那么显然现在用户的操作一定不是燃烧，用户只进行顺时针操作的话，该项就会在C,R,A,"",之间来回切换）
		// （通过这种方式减少了特判，本来后面还得写一些特判的，但是这种方式可以让玩家把操作序列变成前面能判断的状态）
	}
}

bool operator!=(const ImVec4 a, const ImVec4 b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

bool Map::Submit() {
	//cout <<"===\n" << ans <<"\n" <<now ;
	if (ans == now)return 1;
	else {
		if (w != h && (ward & 1) != (goalward & 1)) {
			//cout << "badward";
			return 0;
		}
		int h_=h, w_=w;
		if (ward & 1)swap(h_, w_);
		for (int y = h_; y >= 1; y--) {
			for (int x = 1; x <= w_; x++) {
				if (getNumbers(x, y) > 1 || getColors_index(x, y) != getGoalColors_index(x, y)) {
					//cout << "badcolor at:(" << x << "," << y << ")";
					//cout << "[" << getNumbers(x, y) << "," << getColors_index(x, y) << "," << getGoalColors_index(x, y) << "]";
					return 0;
				}
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
	if (ward == 0)return colors[y][x];//Burning Canvas.exe 
	if (ward == 1)return colors[x][w - y + 1];
	if (ward == 2)return colors[h - y + 1][w - x + 1];
	return colors[h - x + 1][y];
}
ImVec4 Map::getGoalColors(int x, int y) {
	if (goalward == 0)return palette[goalColors[y][x]];
	if (goalward == 1)return palette[goalColors[x][w - y + 1]];
	if (goalward == 2)return palette[goalColors[h - y + 1][w - x + 1]];
	return palette[goalColors[h - x + 1][y]];
	//return palette[goalColors[x][y]];
}
int Map::getGoalColors_index(int x, int y) {
	if (goalward == 0)return goalColors[y][x];
	if (goalward == 1)return goalColors[x][w - y + 1];
	if (goalward == 2)return goalColors[h - y + 1][w - x + 1];
	return goalColors[h - x + 1][y];
	//return palette[goalColors[x][y]];
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
int Map::getGoalward() {
	return goalward;
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

MapDisplay::MapDisplay() {
	size = 0;
	nonumber = 0;
	myMap = nullptr;
	cy = cx = 0;
}
MapDisplay::MapDisplay(Map* mp) {
	size = 0;
	nonumber = 0;
	cy = cx = 0;
	myMap = mp;
}
void MapDisplay::setCenter(int x, int y) {
	cx = x, cy = y;
}

void MapDisplay::setSize(float sz) {
	size = sz;
}

int MapDisplay::getSize() {
	return size;
}

void MapDisplay::setMap(Map* _map) {
	myMap = _map;
}

void MapDisplay::display(bool nonumber) {
	if (myMap == nullptr||myMap->isEditing)return;//	DEBUG
	myMap->isEditing = 1;
	ImVec4 tmpcolor;
	std::string tmpnum;
	int w, h;
	myMap->getSize(w, h);
	if (myMap->getward() & 1)swap(w, h);
	int maxLen = max(h, w);
	float k = 1.0 * size / maxLen;//放缩比例系数
	float kw = k * w;
	float kh = k * h;
	float ks = kw / w;
	
	float H, S, V;
	bool isdark;
	ImVec4 Background[2] = { {1,1,1,1},{235.0 / 255,235.0 / 255,235.0 / 255,1} };
	//if (!nonumber) { 
	//	Background[time(0) % 2000 > 1000] = { 1,0,0,1 };
	//	Background[time(0) % 2000 <= 1000] = { 1,0,0,1 };
	//}

	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			tmpcolor = (myMap->getColors_index(x, y) != -1) ? myMap->getColors(x, y) : Background[0];//Background[(x + y + myMap->getward() + 1) % 2];
			drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), ks, ks, tmpcolor);
			if (myMap->getColors_index(x, y) == -1) {
				drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), ks/2, ks/2, Background[1]);
				drawRect((cx - kw / 2 + (x - 1) * ks)+ks/2, (cy - kh / 2 + (y - 1) * ks)+ks/2, ks/2, ks/2, Background[1]);
			}

			if (nonumber)continue;
			ImGui::ColorConvertRGBtoHSV(tmpcolor.x, tmpcolor.y, tmpcolor.z, H, S, V);
			isdark = (V < 0.4);
			tmpnum = std::to_string(myMap->getNumbers(x,y));

			//if (tmpnum != "1") {
			//	drawRect(
			//		int(cx - kw / 2 + (x - 1) * ks),
			//		int(cy - kh / 2 + (y - 1) * ks),
			//		ks, ks, ImVec4(0, 0, 0, 1)
			//	);
			//	drawRect(
			//		int(cx - kw / 2 + (x - 1) * ks + ks * 0.1),
			//		int(cy - kh / 2 + (y - 1) * ks + ks * 0.1),
			//		ks * 0.8, ks * 0.8, tmpcolor
			//	);
			//}

			drawText(
				(float)(cx - kw / 2 + (x - 1) * ks + 0.5 * ks), (float)(cy - kh / 2 + (y - 1) * ks + 0.5 * ks),
				ks * 0.7 * (1 - (tmpnum.length() == 3)*0.2),
				ImVec4(isdark, isdark, isdark, 1),
				tmpnum
			);
		}
	}
	if (showGrid) {
		for (int y = 1; y <= h; y++) {
			for (int x = 1; x <= w; x++) {
				showUnitBoard(x, y);
			}
		}
	}
	if (myMap->getward() & 1)swap(w, h);
	myMap->isEditing = 0;
}

void MapDisplay::displayFinish() {
	if (myMap == nullptr || myMap->isEditing)return;//	DEBUG
	myMap->isEditing = 1;
	ImVec4 tmpcolor;
	std::wstring tmpnum;
	int w, h;
	myMap->getSize(w, h);
	int maxLen = max(h, w);
	float k = 1.0 * size / maxLen;//放缩比例系数
	float kw = k * w;
	float kh = k * h;
	float ks = kw / w;

	if (myMap->getGoalward() & 1)swap(w, h),swap(kw,kh);
	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			tmpcolor = myMap->getGoalColors(x,y);
			drawRect(cx - kw / 2 + (x - 1) * ks, cy - kh / 2 + (y - 1) * ks, ks, ks, tmpcolor);

			if (showGrid) {
				drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), ks, 1, gridColor);
				drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), 1, ks, gridColor);
				drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 0) * ks), ks, 1, gridColor);
				drawRect((cx - kw / 2 + (x - 0) * ks), (cy - kh / 2 + (y - 1) * ks), 1, ks, gridColor);
			}
		}
	}
	myMap->isEditing = 0;
	//if (showGrid) {
	//	for (int y = 1; y <= h; y++) {
	//		for (int x = 1; x <= w; x++) {
	//			showUnitBoard(x, y);
	//		}
	//	}
	//}
}

void MapDisplay::showUnitBoard(int x, int y) {
	//if (myMap == nullptr || myMap->isEditing)return;//	DEBUG
	myMap->isEditing = 1;
	int w, h;
	myMap->getSize(w, h);
	if (myMap->getward() & 1)swap(w, h);
	int maxLen = max(h, w);
	float k = 1.0 * size / maxLen;//放缩比例系数
	float kw = k * w;
	float kh = k * h;
	float ks = kw / w;
	//drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), ks, ks, ImVec4(0, 0, 0, 1));
	drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), ks, 1, gridColor);
	drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), 1, ks, gridColor);
	drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 0) * ks), ks, 1, gridColor);
	drawRect((cx - kw / 2 + (x - 0) * ks), (cy - kh / 2 + (y - 1) * ks), 1, ks, gridColor);
	myMap->isEditing = 0;
}

void MapDisplay::showUnitColor(int x, int y, ImVec4 color) {
	if (myMap == nullptr || myMap->isEditing)return;//	DEBUG
	myMap->isEditing = 1;
	int w, h;
	myMap->getSize(w, h);
	if (myMap->getward() & 1)swap(w, h);
	int maxLen = max(h, w);
	float k = 1.0 * size / maxLen;//放缩比例系数
	float kw = k * w;
	float kh = k * h;
	float ks = kw / w;
	drawRect((cx - kw / 2 + (x - 1) * ks), (cy - kh / 2 + (y - 1) * ks), ks, ks, color);
	myMap->isEditing = 0;
}

bool MapDisplay::isShowGrid() {
	return showGrid;
}

void MapDisplay::setShowGrid(bool v) {
	showGrid = v;
}

void MapDisplay::setGridColor(ImVec4 color) {
	gridColor = color;
}

MapDisplay::~MapDisplay() {
	//delete myMap;
}