#include "CustomMap.h"

struct BLOCK {
	int x, y;
	int cost;//	设置我为激活块的话，会减少多少的自由度
	bool isEmpty;
	bool operator<(const BLOCK b) const {
		if (isEmpty == b.isEmpty)return cost < b.cost;//	代价低的优先
		return isEmpty < b.isEmpty;//	空闲块的概率调低一点，消耗太快可能导致随机地图过程收敛过快
	}
};

CMap::CMap():Map::Map() {
	palette = nullptr;
	colors = nullptr;
	numbers = nullptr;
	goalColors = nullptr;
};

CMap::CMap(const CMap* map) :Map::Map(map) {
	//cout << "ok";
}

CMap::~CMap() {
	
}

void CMap::setSize(int w_,int h_) {
	if (colors != nullptr) {
		for (int i = 1; i <= h; i++)delete[]colors[i];
		for (int i = 1; i <= h; i++)delete[]numbers[i];
		for (int i = 1; i <= h; i++)delete[]goalColors[i];
	}
	w = w_; h = h_;
	colors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		colors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			colors[i][j] = -1;
		}
	}

	numbers = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		numbers[i] = new int[w + 1];
	}

	goalColors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		goalColors[i] = new int[w + 1];
	}
}

void CMap::setPalette(vector<ImVec4>* palette_ptr) {
	if (palette != nullptr) {
		delete palette;
	}
	palette = new ImVec4[palette_ptr->size()];
	colorCnt = int(palette_ptr->size());
	for (int i = 0; i < colorCnt; i++) {
		palette[i] = (*palette_ptr)[i];
	}
}

vector<ImVec4> CMap::getPalette() {
	vector<ImVec4> palette_;
	for (int i = 0; i < colorCnt; i++) {
		palette_.push_back(palette[i]);
	}
	return palette_;
}

void CMap::randomDraw() {
	for (int i = 1; i <= h; i++) {
		for (int j = 1; j <= w; j++) {
			colors[i][j] = rnd(0, colorCnt - 1);
		}
	}
}

void CMap::randomStep(int maxstep, int minstep) {
	for (int i = 1; i <= h; i++) {
		for (int j = 1; j <= w; j++) {
			numbers[i][j] = rnd(minstep, maxstep);
		}
	}
}

string& CMap::getNow() {
	return now;
}

void CMap::setGoalward(int ward_) {
	goalward = ward_;
}

void CustomManager::Random() {
	CMap* tmp = GetrandomMap(w, h, &palette, maxstep, minstep);
	setBegin(tmp);
	setEnd(begin);
	toEnd();
	end->goalward = end->ward;
	begin->goalward = end->ward;
	begin->ans = end->now;
	delete tmp;
}

void CustomManager::R_Random() {
	//system("cls");
	end->ward = 0;
	setBegin(end);	//	设置由玩家指定的目标图像作为起始图像
	toBegin();		//	对起始图像进行逆燃烧操作，直到逆燃烧迭代结束
}

bool CMap::R_Burn(set<pair<int,int>> &select, set<pair<int,int>> &avail) {
	//cout << select.size() << "]:";
	//for (auto [x, y] : select) {
	//	cout << "(" << x << "," << y << ")";
	//}cout << "\n";
	int w_ = w, h_ = h;
	
	vector<vector<int>> originColor(h_ + 1, vector<int>(w_ + 1)), originNumber(h_ + 1, vector<int>(w_ + 1)), vis(h_ + 1, vector<int>(w_ + 1, 0));
	for (int x = 1; x <= w_; x++) {//	记录每个格子原来颜色和数字
		for (int y = 1; y <= h_; y++) {
			originColor[y][x] = colors[y][x];// = getColors_index(x, y);
			originNumber[y][x] = numbers[y][x];// = getNumbers(x, y);
		}
	}

	//	将接收到的pos重定向
	auto relocatePos = [&](int x, int y) ->pair<int, int> {
		if (ward == 0) return { y,x };
		else if (ward == 1) return { x,w - y + 1 };
		else if (ward == 2) return { h - y + 1,w - x + 1 };
		else return { h - x + 1,y };
	};

	bool isVaildMove = 0;

	if (ward & 1)swap(w_, h_);
	for (int x = 1; x <= w_; x++) {
		for (int y = h_; y >= 2; y--) {
			auto [rx, ry] = relocatePos(x, y);
			if (select.count({ x,y })) {
				if (originColor[rx][ry] == -1) {
					if (!avail.count({ x,y - 1 }) && getColors_index(x, y - 1) != -1) {//	如果我这个空闲块被设成激活块了，我得看下一个格子的颜色确定我的颜色，如果下个格子不在可激活块列表里，那我就别惹事了，老实选和它一个颜色就行
						originColor[rx][ry] = getColors_index(x, y - 1);
					}
					else {//	否则我就能随便选色了
						if (rnd(1, 100) > 70) {//	70%的概率将颜色设置成左右的
							if (x == 1) {
								auto [rxr, ryr] = relocatePos(x + 1, y);
								originColor[rx][ry] = getColors_index(rxr, ryr);
							}
							else if (x == w_) {
								auto [rxl, ryl] = relocatePos(x - 1, y);
								originColor[rx][ry] = getColors_index(rxl, ryl);
							}
							else {
								auto [rxl, ryl] = relocatePos(x - 1, y);
								auto [rxr, ryr] = relocatePos(x + 1, y);
								originColor[rx][ry] = rnd(0, 1) ? getColors_index(rxl, ryl) : getColors_index(rxr, ryr);//尽量还是指定成左右的吧
							}
						}
						else {//	30%的概率从色盘里随机选一个色
							originColor[rx][ry] = rnd(0, colorCnt - 1);
						}
						//cout << "随机选色:设置为" << originColor[rx][ry] << "\t";
					}
				}
				auto [nx, ny] = relocatePos(x, y - 1);
				if (originNumber[nx][ny] == 1 && originColor[nx][ny] != -1 && originColor[rx][ry] != originColor[nx][ny]) {
					select.insert({ x,y-1 });
				}
				setColors(x, y - 1, originColor[rx][ry]);
				setNumbers(x, y - 1, originNumber[rx][ry] + 1);
				vis[nx][ny] = 1;
				//cout << "位于" << rx << "," << ry << "的块被设置为激活块后向上移动了\n";
				isVaildMove = 1;
				if (!vis[rx][ry]) {
					setNumbers(x, y, 1);
					setColors(x, y, -1);
				}

			}
			else if (originNumber[rx][ry] > 1) {
				auto [nx, ny] = relocatePos(x, y - 1);
				if (originNumber[nx][ny] == 1 && (originColor[nx][ny] != -1 && originColor[rx][ry] != originColor[nx][ny] || rnd(1,100) < 5 && avail.count({ x,y - 1 }))) {//一定概率（5%）将可激活块激活了
					select.insert({ x,y-1 });
				}
				setColors(x, y - 1, originColor[rx][ry]);
				setNumbers(x, y - 1, originNumber[rx][ry] + 1);
				vis[nx][ny] = 1;
				//cout << "位于" << rx << "," << ry << "的激活块向上移动了\n";
				isVaildMove = 1;
				if (!vis[rx][ry]) {
					setNumbers(x, y, 1);
					setColors(x, y, -1);
				}
			}
		}
	}
	if (isVaildMove) {
		operationInsert('B');
		//cout << "逆燃烧成功\n";
		return 1;
	}
	return 0;
}

CustomManager::CustomManager(){
	palette.push_back(ImVec4(1, 0, 0, 1));
	palette.push_back(ImVec4(0, 1, 0, 1));
	palette.push_back(ImVec4(0, 0, 1, 1));
	Random();
}
CustomManager::~CustomManager() {
	delete begin, end;
}

void CustomManager::reset() {
	w = 10;//3~20
	h = 10;//3~20
	maxstep = 1;//1~400
	minstep = 10;//1~400
	palette.clear();
	palette.push_back(ImVec4(1, 0, 0, 1));
	palette.push_back(ImVec4(0, 1, 0, 1));
	palette.push_back(ImVec4(0, 0, 1, 1));
	Random();
}

void CustomManager::setBegin(CMap* map) {
	delete begin;
	begin = new CMap(map);
}

void CustomManager::setEnd(CMap* map) {
	delete end;
	end = new CMap(map);
}

CMap*& CustomManager::getBegin() {
	return begin;
}

CMap*& CustomManager::getEnd() {
	return end;
}

void CustomManager::toEnd() {
	while (true) {
		int op = rnd(1, 3);
		if (op == 1) {
			end->clockwise();
		}
		else if (op == 2) {
			end->anticlockwise();
		}
		else {
			end->Burn();
		}
		bool nofound = 1;
		int w_, h_;
		end->getSize(w_, h_);
		if (end->getward() & 1)swap(w_, h_);
		for (int y = 1; y <= h_; y++) {
			for (int x = 1; x <= w_; x++) {
				if (end->getNumbers(x, y) > 1) {
					nofound = 0; break;
				}
			}
			if (!nofound)break;
		}
		if (end->getward() & 1)swap(w_, h_);
		if (nofound)break;
	}
	int ww = rnd(0, 3);//	设置干扰方向
	while (end->getward() != ww) {
		end->clockwise();
	}
}

//	0：正常	-1：未找到连续快	-2：色盘太小	-3：地图存在透明块		
int CustomManager::checkMap()
{
	if (palette.size() <= 1) {
		//	色盘出错，太小了
		return -2;
	}
	bool continueBlock = 0;
	for (int y = 2; y < end->h; y++) {
		for (int x = 2; x < end->w; x++) {
			continueBlock |= (end->getColors_index(x, y) == end->getColors_index(x + 1, y));
			continueBlock |= (end->getColors_index(x, y) == end->getColors_index(x - 1, y));
			continueBlock |= (end->getColors_index(x, y) == end->getColors_index(x, y + 1));
			continueBlock |= (end->getColors_index(x, y) == end->getColors_index(x, y - 1));
			if (end->getColors_index(x, y) == -1||
				end->getColors_index(x + 1, y) == -1 ||
				end->getColors_index(x - 1, y) == -1 ||
				end->getColors_index(x, y + 1) == -1 ||
				end->getColors_index(x, y - 1) == -1
				) {
				//	地图出错，存在透明块
				return -3;
			}
		}
	}
	continueBlock |= (end->getColors_index(1, 1) == end->getColors_index(2, 1));
	continueBlock |= (end->getColors_index(1, 1) == end->getColors_index(1, 2));
	continueBlock |= (end->getColors_index(1, end->h) == end->getColors_index(2, end->h));
	continueBlock |= (end->getColors_index(1, end->h) == end->getColors_index(1, end->h - 1));
	continueBlock |= (end->getColors_index(end->w, 1) == end->getColors_index(end->w, 2));
	continueBlock |= (end->getColors_index(end->w, 1) == end->getColors_index(end->w - 1, 1));
	continueBlock |= (end->getColors_index(end->h, end->h) == end->getColors_index(end->h - 1, end->h));
	continueBlock |= (end->getColors_index(end->h, end->h) == end->getColors_index(end->h, end->h - 1));
	if (continueBlock) {
		//没问题
		return 0;
	}
	else {
		//未找到连续块
		return -1;
	}
	//	未定义错误，不应该运行到这的
	return -4;
}

bool CustomManager::checkPalette()
{
	for (int i = 0; i < palette.size() - 1; i++)
		for (int j = i+1; j < palette.size(); j++) {
			if (CIEDE2000(palette[i].x, palette[i].y, palette[i].z, palette[j].x, palette[j].y, palette[j].z) < 8) {
				return false;
			}
			//cout << CIEDE2000(palette[i].x, palette[i].y, palette[i].z, palette[j].x, palette[j].y, palette[j].z) << "\n";
		}
	return true;
}

void CustomManager::RateRandom(int level)
{
	reset();
	if (level == 0) {
		w = 5;
		h = 5;
		minstep = rnd(1, 6);
		maxstep = rnd(2, 6);
		palette = randomPalette(5);
	}
	else if (level == 1) {
		w = rnd(6, 10);
		h = w;
		minstep = rnd(1, 10);
		maxstep = rnd(2, 10);
		palette = randomPalette(rnd(6, 10));
	}
	else if (level == 2) {
		w = rnd(7, 15);
		h = rnd(7, 15);
		minstep = rnd(1, 20);
		maxstep = rnd(2, 20);
		palette = randomPalette(rnd(10, 12));
	}
	else if (level == 3) {
		w = rnd(5, 21);
		h = rnd(5, 21);
		minstep = rnd(1, 40);
		maxstep = rnd(2, 40);
		palette = randomPalette(rnd(2, 14));
	}
	Random();
	EndCurrentToBeginGoal();
}

vector<ImVec4> CustomManager::randomPalette(int x)
{
	vector<ImVec4> origin;
	origin.push_back({ 0.0f					,0.0f					,0.0f					,1.0f });
	origin.push_back({ 0.9882352941176471f	,0.9372549019607843f	,0.3176470588235294f	,1.0f });
	origin.push_back({ 0.7725490196078432f	,0.9098039215686274f	,0.8823529411764706f	,1.0f });
	origin.push_back({ 1.0f					,0.8431372549019608f	,0.9215686274509803f	,1.0f });
	origin.push_back({ 0.7372549019607844f	,0.3607843137254902f	,0.5058823529411764f	,1.0f });
	origin.push_back({ 0.7137254901960784f	,0.43529411764705883f	,0.36470588235294116f	,1.0f });
	origin.push_back({ 0.43137254901960786f	,0.34509803921568627f	,0.35294117647058826f	,1.0f });
	origin.push_back({ 0.9176470588235294f	,0.00392156862745098f	,0.0784313725490196f	,1.0f });
	origin.push_back({ 0.34509803921568627f	,0.18823529411764706f	,0.14901960784313725f	,1.0f });
	origin.push_back({ 0.9411764705882353f	,0.5647058823529412f	,0.27450980392156865f	,1.0f });
	origin.push_back({ 0.2549019607843137f	,0.44313725490196076f	,0.2549019607843137f	,1.0f });
	origin.push_back({ 0.592156862745098f	,0.1803921568627451f	,0.2f					,1.0f });
	origin.push_back({ 0.6980392156862745f	,0.5647058823529412f	,0.7647058823529411f	,1.0f });
	origin.push_back({ 0.1843137254901961f	,0.1843137254901961f	,0.2235294117647059f	,1.0f });
	origin.push_back({ 0.7137254901960784f	,0.8549019607843137f	,0.9568627450980393f	,1.0f });
	shuffle(origin.begin(), origin.end(), myrand);

	return vector<ImVec4>(origin.begin(), origin.begin() + MAX(2, MIN(x, origin.size())));
}

void CustomManager::toBegin() {
	//最大期望燃烧次数2-399
	int cnt = 0;
	int w_ = w, h_ = h;
	//	顺时针操作后，激活块边界的映射函数
	auto clock = [&](int& x, int& y,int ward) {
		int _x = x, _y = y;
		if (ward == 0) {
			y = h - _x + 1;
			x = _y;
		}
		else if (ward == 1) {
			y = w - _x + 1;
			x = _y;
		}
		else if (ward == 2) {
			y = w - _x + 1;
			x = _y;
		}
		else {
			y = h - _x + 1;
			x = _y - h + w;
		}
		};
	//	逆时针操作后，激活块边界的映射函数
	auto anticlock = [&](int& x, int& y,int ward) {
		int _x = x, _y = y;
		if (ward == 0) {
			y = _x;
			x = w - _y + 1;
		}
		else if (ward == 1) {
			y = _x - w + h;
			x = w - _y + 1;
		}
		else if (ward == 2) {
			y = _x - w + h;
			x = h - _y + 1;
		}
		else {
			y = _x;
			x = h - _y + 1;
		}
		};
	//	通过当前地图ward方向，以及相对坐标，映射出真实数据位置下标
	auto relocatePos = [&](int x, int y, int ward) ->pair<int, int> {//DEBUG
		if (ward == 0) return { y,x };
		else if (ward == 1) return { x,w - y + 1 };
		else if (ward == 2) return { h - y + 1,w - x + 1 };
		else return { h - x + 1,y };
		};
	//	上一个函数的反函数
	auto rerelocatePos = [&](int x, int y, int ward)->pair<int, int> {
		//if (ward == 0) return { x,y };
		//else if (ward == 1) return { y,w - x + 1 };
		//else if (ward == 2) return { w - x + 1,h - y + 1 };
		//else return { h - y + 1,x };
		if (ward == 0) return { x,y };
		else if (ward == 1) return { y,w - x + 1 };
		else if (ward == 2) return { w - x + 1,h - y + 1 };
		else return { h - y + 1,x };
		};

	vector<multiset<BLOCK>> vaildSet(4);		//	各个方向下的可激活块
	vector<vector<vector<int>>> movingCost(4);	//	各个方向下的代价
	bool canMove[4] = { 0 };					//	各个方向是否能够逆燃烧

	for (int y = 1; y <= h_; y++) {
		for (int x = 1; x <= w_; x++) {
			begin->setNumbers(x, y, 1);
		}
	}
	begin->ward = 0;							//	ward方向强制置0，消除上一轮end的结果造成的影响
	begin->getNow() = "";
	int nowx = rnd(1, w), nowy = rnd(1, h);
	int Xmin, Xmax, Ymin, Ymax;					//	要维护这个，这个是激活块窗口的边界信息
	//begin->setNumbers(nowx, nowy, 2);//	启动

	bool nobegin = 1;	//	还未设置任何激活块的标志
	int strict = 0;		//	迭代次数限制，超过迭代次数表示程序出现未知原因陷入死循环（按理说不可能出现死循环）

	while (cnt<=step&&strict<444) {	//	迭代次数限制，超过设定次数或者迭代次数就退出
		//	限制迭代次数
		strict++;
		//int op = rnd(1, 3);
		//if (op == 1) {
		//	//cout << "原先POS：[" << Xmin << "," << Ymin << "],顺时针旋转后：[";
		//	clock(Xmin, Ymin, begin->getward());
		//	clock(Xmax, Ymax, begin->getward());
		//	begin->clockwise();
		//	//cout << Xmin << "," << Ymin << "]";
		//	cout << "顺时针旋转了\n";
		//}
		//else if (op == 2) {
		//	//cout << "原先POS：[" << Xmin << "," << Ymin << "],逆时针旋转后：[";
		//	anticlock(Xmin, Ymin, begin->getward());
		//	anticlock(Xmax, Ymax, begin->getward());
		//	begin->anticlockwise();
		//	//cout << Xmin << "," << Ymin << "]";
		//	cout << "逆时针旋转了\n";
		//}
		//else 
		
		//	进行一个小剪枝，直接从可逆燃烧方向直接选方向进行燃烧，100%可以烧，随机效率会大大提高
		vector<int> vaildWard;
		for (int i = 0; i < 4; i++) {
			if (canMove[i]) {
				vaildWard.push_back(i);
			}
		}
		if (!vaildWard.empty()) {
			int chooseWard = vaildWard[rnd(0, vaildWard.size() - 1)];
			while (begin->getward() != chooseWard) {
				clock(Xmin, Ymin, begin->getward());
				clock(Xmax, Ymax, begin->getward());
				begin->clockwise();
			}
		}

		{
			if (!nobegin && canMove[(begin->getward() + 0)%4]) {
				//这里随机选取一些可激活块，然后进行逆燃烧
				set<pair<int,int>> select, avail;
				for (auto& block : vaildSet[begin->getward()]) {
					avail.insert({ block.x,block.y });
				}
				for (int i = 0; i < rnd(0, 3); i++) {
					auto it = vaildSet[begin->getward()].rbegin();
					int num = norm_rnd(vaildSet[begin->getward()].size(), 2.58);
					for (int k = 0; k < num; k++) {
						it++;
					}
					//begin->setNumbers(it->x, it->y, MAX(begin->getNumbers(it->x, it->y), 2));
					//	更新激活块窗口边界
					Xmin = MIN(Xmin, it->x);
					Xmax = MAX(Xmax, it->x);
					Ymin = MIN(Ymin, it->y);
					Ymax = MAX(Ymax, it->y);
					select.insert({ it->x,it->y });
					//auto [rx, ry] = relocatePos(it->x, it->y, begin->getward());
					//cout << "设置了" << rx << " " << ry << "为新的激活块\n";
				}
				//cnt++;//能进到这步，肯定是能逆燃烧的 // = 

				//cout << "[进行逆燃烧:（"<< begin->ward <<"）\n";
				//if (begin->getward() & 1)swap(w_, h_);
				//
				//cout << "当前代价矩阵：\n";
				//for (int y = 1; y <= h_; y++) {
				//	for (int x = 1; x <= w_; x++) {
				//		auto [rx, ry] = relocatePos(x, y, (begin->getward() + 0) % 4);
				//		cout << movingCost[begin->ward][rx][ry] << "\t\n"[x == w_];
				//	}
				//}
				//
				//cout << "当前颜色下标矩阵：" << "\n";
				//for (int y = 1; y <= h_; y++) {
				//	for (int x = 1; x <= w_; x++) {
				//		cout << " ["[begin->getNumbers(x, y)>1] << begin->getColors_index(x, y) << " ]"[begin->getNumbers(x, y) > 1] << "\t\n"[x == w_];
				//	}
				//}
				//cout << "当前数字矩阵：" << "\n";
				//for (int y = 1; y <= h_; y++) {
				//	for (int x = 1; x <= w_; x++) {
				//		cout << begin->getNumbers(x, y) << "\t\n"[x == w_];
				//	}
				//}
				//if (begin->getward() & 1)swap(w_, h_);
				cnt+=begin->R_Burn(select, avail);
				//cout << "逆燃烧结束\n";
				//if (begin->getward() & 1)swap(w_, h_);
				//cout << "当前颜色下标矩阵：" << "\n";
				//for (int y = 1; y <= h_; y++) {
				//	for (int x = 1; x <= w_; x++) {
				//		cout << " ["[begin->getNumbers(x, y) > 1] << begin->getColors_index(x, y) << " ]"[begin->getNumbers(x, y) > 1] << "\t\n"[x == w_];
				//	}
				//}
				//cout << "当前数字矩阵：" << "\n";
				//for (int y = 1; y <= h_; y++) {
				//	for (int x = 1; x <= w_; x++) {
				//		cout << begin->getNumbers(x, y) << "\t\n"[x == w_];
				//	}
				//}
				//if (begin->getward() & 1)swap(w_, h_);
				//cout << "]\n";
			}
		}
		//if (begin->getward() & 1)swap(w_, h_);
		//	初始化各个方向的信息
		//for (int i = 0; i < 4; i++) {
		//	canMove[i] = 1;
		//	//while (!vaildSet[i].empty()) {
		//	//	vaildSet[i].pop();
		//	//}
		//	vaildSet[i].clear();
		//	movingCost[i] = vector<vector<int>>(h_ + 2, vector<int>(w_ + 2, -99999));//初始化代价为-INF
		//}

		if (nobegin) {
			int nowx = rnd(1, w_), nowy = rnd(1, h_);
			Xmin = nowx, Xmax = nowx, Ymin = nowy, Ymax = nowy;
			nobegin = 0;
		}
		//int baseWard = begin->getward();
		//	收集各个方向的 可激活块
		//	最终版，绝对不会再改了
		for (int i = 0; i < 4; i++) {
			if (begin->getward() & 1)swap(w_, h_);
			canMove[begin->getward()] = 1;
			vaildSet[begin->getward()].clear();
			movingCost[begin->getward()] = vector<vector<int>>(MAX(w_, h_) + 2, vector<int>(MAX(w_, h_) + 2, -99999));//初始化代价为-INF

			for (int x = 1; x <= w_; x++) {
				for (int y = 1; y <= h_; y++) {
					auto [rx, ry] = relocatePos(x, y, begin->getward());
					auto [nx, ny] = relocatePos(x, y-1, begin->getward());
					if (y != 1) {
						movingCost[begin->getward()][rx][ry] = movingCost[begin->getward()][nx][ny] + 1;
						if ((begin->getColors_index(x, y - 1) == -1 ||
							begin->getColors_index(x, y) == -1 ||
							begin->getColors_index(x, y - 1) == begin->getColors_index(x, y))) {
							movingCost[begin->getward()][rx][ry] = 0;
						}
						if (begin->getNumbers(x, y) > 1) {
							canMove[begin->getward()] &= (movingCost[begin->getward()][nx][ny] >= 0 || begin->getColors_index(x, y - 1) == begin->getColors_index(x, y));
						}
					}
					else {
						if (begin->getNumbers(x, y) > 1) {
							canMove[begin->getward()] = 0;
						}
					}
					//cout << movingCost[(begin->getward() + 0) % 4][y][x] << "\t\n"[y == h_];//DEBUG
					if (begin->getNumbers(x, y) == 1 && movingCost[begin->getward()][rx][ry] >= 0) {
						int cost = 0;
						cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
						cost += MAX(0, (y)-Ymax) + MAX(0, Ymin - (y));
						cost += MAX(0, (y - movingCost[begin->getward()][rx][ry]) - Ymax) + MAX(0, Ymin - (y - movingCost[begin->getward()][rx][ry]));
						//auto [rrx, rry] = rerelocatePos(x, y, 2);
						vaildSet[begin->getward()].insert({ x,y,cost,(begin->getColors_index(x, y) == -1) });
					}
				}
			}
			if (begin->getward() & 1)swap(w_, h_);
			clock(Xmin, Ymin, begin->getward());
			clock(Xmax, Ymax, begin->getward());
			begin->clockwise();
			//cout << "顺时针旋转了\n";
		}//	下面几个注释折叠块是之前写的几个版本
		
		// 
		//	最终版本（？）版本3
		//cout << "我现在在"<< begin->getward() <<"方向（正方向），想要往" << (begin->getward() + 1) % 4 << "方向（右）移动，我的代价矩阵消息为：\n";
		//for (int y = 1; y <= h_; y++) {
		//	for (int x = w_; x >= 1; x--) {
		//		if (x != w_) {
		//			movingCost[(begin->getward() + 1) % 4][y][x] = movingCost[(begin->getward() + 1) % 4][y][x + 1] + 1;
		//			if ((begin->getColors_index(x + 1, y) == -1 ||
		//				begin->getColors_index(x, y) == -1 ||
		//				begin->getColors_index(x + 1, y) == begin->getColors_index(x, y))) {
		//				movingCost[(begin->getward() + 1) % 4][y][x] = 0;
		//			}
		//			
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 1) % 4] &=
		//					(movingCost[(begin->getward() + 1) % 4][y][x + 1] >= 0 ||
		//					begin->getColors_index(x + 1, y) == begin->getColors_index(x, y));
		//			}
		//		}
		//		else {
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 1) % 4] = 1;
		//			}
		//		}
		//		cout << movingCost[(begin->getward() + 1) % 4][y][x] << "\t\n"[x == 1];//DEBUG
		//		//cout << "[" << x << "," << y << "]的信息为：" << (begin->numbers[y][x - 1] == 1) << "," << begin->colors[y][x - 1] << "," << begin->colors[y][x] << "\n";
		//		if (begin->getNumbers(x, y) == 1 && movingCost[(begin->getward() + 1) % 4][y][x] >= 0) {
		//			//	计算代价
		//			int cost = 0;
		//			cost += MAX(0, (x)-Xmax) + MAX(0, Xmin - (x));
		//			cost += MAX(0, y - Ymax) + MAX(0, Ymin - y);
		//			cost += MAX(0, (x + movingCost[(begin->getward() + 1) % 4][y][x]) - Xmax) +
		//				MAX(0, Xmin - (x + movingCost[(begin->getward() + 1) % 4][y][x]));
		//			//auto [rrx, rry] = rerelocatePos(x, y, 3);
		//			//	加入到可激活块的优先队列，按照cost排序，同cost下非空闲块优先
		//			vaildSet[(begin->getward() + 1) % 4].insert({ x,y,cost,(begin->getColors_index(x, y) == -1) });
		//		}
		//	}
		//}
		//cout << "我现在在" << begin->getward() << "方向（正方向），想要往" << (begin->getward() + 3) % 4 << "方向（左）移动，我的代价矩阵消息为：\n";
		//for (int y = 1; y <= h_; y++) {
		//	for (int x = 1; x <= w_; x++) {
		//		if (x != 1) {
		//			movingCost[(begin->getward() + 3) % 4][y][x] = movingCost[(begin->getward() + 3) % 4][y][x - 1] + 1;
		//			if ((begin->getColors_index(x - 1, y) == -1 ||
		//				begin->getColors_index(x, y)==-1 ||
		//				begin->getColors_index(x - 1, y) == begin->getColors_index(x, y))) {
		//				movingCost[(begin->getward() + 3) % 4][y][x] = 0;
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 3) % 4] &= (movingCost[(begin->getward() + 3) % 4][y][x - 1] >= 0 || begin->getColors_index(x - 1, y) == begin->getColors_index(x, y));
		//			}
		//		}
		//		else {
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 3) % 4] = 0;
		//			}
		//		}
		//		cout << movingCost[(begin->getward() + 3) % 4][y][x] << "\t\n"[x == w_];//DEBUG
		//		if (begin->getNumbers(x, y) == 1 && movingCost[(begin->getward() + 3) % 4][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, (x)-Xmax) + MAX(0, Xmin - (x));
		//			cost += MAX(0, y - Ymax) + MAX(0, Ymin - y);
		//			cost += MAX(0, (x - movingCost[(begin->getward() + 3) % 4][y][x]) - Xmax) + MAX(0, Xmin - (x - movingCost[(begin->getward() + 3) % 4][y][x]));
		//			//auto [rrx, rry] = rerelocatePos(x, y, 1);
		//			vaildSet[(begin->getward() + 3) % 4].insert({ x,y,cost,(begin->getColors_index(x, y) == -1) });
		//		}
		//	}
		//}
		//cout << "我现在在" << begin->getward() << "方向（正方向），想要往" << (begin->getward() + 2) % 4 << "方向（下）移动，我的代价矩阵消息为：\n";
		//for (int x = 1; x <= w_; x++) {
		//	for (int y = h_; y >= 1; y--) {
		//		if (y != h_) {
		//			movingCost[(begin->getward() + 2) % 4][y][x] = movingCost[(begin->getward() + 2) % 4][y + 1][x] + 1;
		//			if ((begin->getColors_index(x, y+1) == -1 ||
		//				begin->getColors_index(x, y) == -1 ||
		//				begin->getColors_index(x, y + 1) == begin->getColors_index(x, y))) {
		//				movingCost[(begin->getward() + 2) % 4][y][x] = 0;
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 2) % 4] &= (movingCost[(begin->getward() + 2) % 4][y + 1][x] >= 0 || begin->getColors_index(x, y + 1) == begin->getColors_index(x, y));
		//			}
		//		}
		//		else {
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 2) % 4] = 0;
		//			}
		//		}
		//		cout << movingCost[(begin->getward() + 2) % 4][y][x] << "\t\n"[y == 1];//DEBUG
		//		if (begin->getNumbers(x, y) == 1 && movingCost[(begin->getward() + 2) % 4][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
		//			cost += MAX(0, (y)-Ymax) + MAX(0, Ymin - (y));
		//			cost += MAX(0, (y + movingCost[0][y][x]) - Ymax) + MAX(0, Ymin - (y + movingCost[(begin->getward() + 2) % 4][y][x]));
		//			//auto [rrx, rry] = rerelocatePos(x, y, 0);
		//			vaildSet[(begin->getward() + 2) % 4].insert({ x,y,cost,(begin->getColors_index(x, y) == -1) });
		//		}
		//	}
		//}
		//cout << "我现在在" << begin->getward() << "方向（正方向），想要往" << (begin->getward() + 0) % 4 << "方向（上）移动，我的代价矩阵消息为：\n";
		//for (int x = 1; x <= w_; x++) {
		//	for (int y = 1; y <= h_; y++) {
		//		if (y != 1) {
		//			movingCost[(begin->getward() + 0) % 4][y][x] = movingCost[(begin->getward() + 0) % 4][y - 1][x] + 1;
		//			if ((begin->getColors_index(x, y-1) == -1 ||
		//				begin->getColors_index(x, y) == -1 || 
		//				begin->getColors_index(x, y - 1) == begin->getColors_index(x, y))) {
		//				movingCost[(begin->getward() + 0) % 4][y][x] = 0;
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 0) % 4] &= (movingCost[(begin->getward() + 0) % 4][y - 1][x] >= 0 || begin->getColors_index(x, y - 1) == begin->getColors_index(x, y));
		//			}
		//		}
		//		else {
		//			if (begin->getNumbers(x, y) > 1) {
		//				canMove[(begin->getward() + 0) % 4] = 0;
		//			}
		//		}
		//		cout << movingCost[(begin->getward() + 0) % 4][y][x] << "\t\n"[y == h_];//DEBUG
		//		if (begin->getNumbers(x, y) == 1 && movingCost[(begin->getward() + 0) % 4][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
		//			cost += MAX(0, (y)-Ymax) + MAX(0, Ymin - (y));
		//			cost += MAX(0, (y - movingCost[(begin->getward() + 0) % 4][y][x]) - Ymax) + MAX(0, Ymin - (y - movingCost[(begin->getward() + 0) % 4][y][x]));
		//			//auto [rrx, rry] = rerelocatePos(x, y, 2);
		//			vaildSet[(begin->getward() + 0) % 4].insert({ x,y,cost,(begin->getColors_index(x, y) == -1) });
		//		}
		//	}
		//}
		//if (begin->getward() & 1)swap(w_, h_);
		//版本2：
		//for (int y = 1; y <= h_; y++) {
		//	for (int x = w_; x >= 1; x--) {
		//		if (x != w_) {
		//			movingCost[3][y][x] = movingCost[3][y][x + 1] + 1;
		//			if ((begin->colors[y][x + 1] == -1 || begin->colors[y][x] == -1 || begin->colors[y][x + 1] == begin->colors[y][x])) {
		//				movingCost[3][y][x] = 0;
		//			}
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[3] &= (movingCost[3][y][x + 1] >= 0 || begin->colors[y][x + 1] == begin->colors[y][x]);///
		//			}
		//		}
		//		else {
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[3] = 1;///
		//			}
		//		}
		//		//cout << "[" << x << "," << y << "]的信息为：" << (begin->numbers[y][x - 1] == 1) << "," << begin->colors[y][x - 1] << "," << begin->colors[y][x] << "\n";
		//		if (begin->numbers[y][x] == 1 && movingCost[3][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, (x) - Xmax) + MAX(0, Xmin - (x));
		//			cost += MAX(0, y - Ymax) + MAX(0, Ymin - y);
		//			cost += MAX(0, (x + movingCost[3][y][x]) - Xmax) + MAX(0, Xmin - (x + movingCost[3][y][x]));
		//			auto [rrx, rry] = rerelocatePos(x, y, 3);
		//			vaildSet[3].insert({ rrx,rry,cost,(begin->colors[y][x] == -1) });
		//		}
		//	}
		//}
		//for (int y = 1; y <= h_; y++) {
		//	for (int x = 1; x <= w_; x++) {
		//		if (x != 1) {
		//			movingCost[1][y][x] = movingCost[1][y][x - 1] + 1;
		//			if ((begin->colors[y][x - 1] == -1 || begin->colors[y][x] == -1 || begin->colors[y][x - 1] == begin->colors[y][x])) {
		//				movingCost[1][y][x] = 0;
		//			}
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[1] &= (movingCost[1][y][x - 1] >= 0|| begin->colors[y][x - 1] == begin->colors[y][x]);
		//			}
		//		}
		//		else {
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[1] = 0;
		//			}
		//		}
		//		if (begin->numbers[y][x] == 1 && movingCost[1][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, (x) - Xmax) + MAX(0, Xmin - (x));
		//			cost += MAX(0, y - Ymax) + MAX(0, Ymin - y);
		//			cost += MAX(0, (x - movingCost[1][y][x]) - Xmax) + MAX(0, Xmin - (x - movingCost[1][y][x]));
		//			auto [rrx, rry] = rerelocatePos(x, y, 1);
		//			vaildSet[1].insert({ rrx,rry,cost,(begin->colors[y][x] == -1) });
		//		}
		//	}
		//}
		//for (int x = 1; x <= w_; x++) {
		//	for (int y = h_; y >= 1; y--) {
		//		if (y != h_) {
		//			movingCost[0][y][x] = movingCost[0][y + 1][x] + 1;
		//			if ((begin->colors[y + 1][x] == -1 || begin->colors[y][x] == -1 || begin->colors[y + 1][x] == begin->colors[y][x])) {
		//				movingCost[0][y][x] = 0;
		//			}
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[0] &= (movingCost[0][y + 1][x] >= 0|| begin->colors[y + 1][x] == begin->colors[y][x]);
		//			}
		//		}
		//		else {
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[0] = 0;
		//			}
		//		}
		//		if (begin->numbers[y][x] == 1 && movingCost[0][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
		//			cost += MAX(0, (y) - Ymax) + MAX(0, Ymin - (y));
		//			cost += MAX(0, (y + movingCost[0][y][x]) - Ymax) + MAX(0, Ymin - (y + movingCost[0][y][x]));
		//			auto [rrx, rry] = rerelocatePos(x, y, 0);
		//			vaildSet[0].insert({ rrx,rry,cost,(begin->colors[y][x] == -1) });
		//		}
		//	}
		//}
		//for (int x = 1; x <= w_; x++) {
		//	for (int y = 1; y <= h_; y++) {
		//		if (y != 1) {
		//			movingCost[2][y][x] = movingCost[2][y - 1][x] + 1;
		//			if ((begin->colors[y - 1][x] == -1 || begin->colors[y][x] == -1 || begin->colors[y - 1][x] == begin->colors[y][x])) {
		//				movingCost[2][y][x] = 0;
		//			}
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[2] &= (movingCost[2][y - 1][x] >= 0 || begin->colors[y - 1][x] == begin->colors[y][x]);
		//			}
		//		}
		//		else {
		//			if (begin->numbers[y][x] > 1) {
		//				canMove[2] = 0;
		//			}
		//		}
		//		if (begin->numbers[y][x] == 1 && movingCost[2][y][x] >= 0) {
		//			int cost = 0;
		//			cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
		//			cost += MAX(0, (y) - Ymax) + MAX(0, Ymin - (y));
		//			cost += MAX(0, (y - movingCost[2][y][x]) - Ymax) + MAX(0, Ymin - (y - movingCost[2][y][x]));
		//			auto [rrx, rry] = rerelocatePos(x, y, 2);
		//			vaildSet[2].insert({ rrx,rry,cost,(begin->colors[y][x] == -1) });
		//		}
		//	}
		//}
		
		//版本1：
		//for (int y = 1; y <= h_; y++) {
		//	for (int x = w_; x >= 1; x--) {
		//		//auto [rx, ry] = relocatePos(x, y, begin->getward());
		//		if (canMove[(begin->getward() + 3) % 4]) {
		//			if (x > 1 && begin->getNumbers(x - 1, y) == 1 && (begin->getColors_index(x - 1, y) == -1 || begin->getColors_index(x, y) == -1 || begin->getColors_index(x - 1, y) == begin->getColors_index(x, y))) {
		//				int cost = 0;
		//				cost += MAX(0, (x - 1) - Xmax) + MAX(0, Xmin - (x - 1));
		//				cost += MAX(0, y - Ymax) + MAX(0, Ymin - y);
		//				vaildSet[(begin->getward() + 3) % 4].insert({ x - 1,y,cost,(begin->getColors_index(x - 1, y) == -1) });
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				if (x == w_) {
		//					canMove[(begin->getward() + 3) % 4] = 0;
		//				}
		//				else {
		//					canMove[(begin->getward() + 3) % 4] &= (vaildSet[(begin->getward() + 3) % 4].size() > 0);
		//				}
		//			}
		//		}
		//	}
		//}
		//for (int y = 1; y <= h_; y++) {
		//	for (int x = 1; x <= w_; x++) {
		//		auto [rx, ry] = relocatePos(x, y, begin->getward());
		//		if (canMove[(begin->getward() + 1) % 4]) {
		//			if (x < w_ && begin->getNumbers(x + 1, y) == 1 && (begin->getColors_index(x + 1, y) == -1 || begin->getColors_index(x, y) == -1 || begin->getColors_index(x + 1, y) == begin->getColors_index(x, y))) {
		//				int cost = 0;
		//				cost += MAX(0, (x + 1) - Xmax) + MAX(0, Xmin - (x + 1));
		//				cost += MAX(0, y - Ymax) + MAX(0, Ymin - y);
		//				vaildSet[(begin->getward() + 1) % 4].insert({ x + 1,y,cost,(begin->getColors_index(x + 1, y) == -1) });
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				if (x == 1) {
		//					canMove[(begin->getward() + 1) % 4] = 0;
		//				}
		//				else {
		//					canMove[(begin->getward() + 1) % 4] &= (vaildSet[(begin->getward() + 1) % 4].size() > 0);
		//				}
		//			}
		//		}
		//	}
		//}
		//for (int x = 1; x <= w_; x++) {
		//	for (int y = h_; y >= 1; y--) {
		//		auto [rx, ry] = relocatePos(x, y, begin->getward());
		//		if (canMove[(begin->getward() + 2) % 4]) {
		//			if (y > 1 && begin->getNumbers(x, y - 1) == 1 && (begin->getColors_index(x, y - 1) == -1 || begin->getColors_index(x, y) == -1 || begin->getColors_index(x, y - 1) == begin->getColors_index(x, y))) {
		//				int cost = 0;
		//				cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
		//				cost += MAX(0, (y - 1) - Ymax) + MAX(0, Ymin - (y - 1));
		//				vaildSet[(begin->getward() + 2) % 4].insert({ x,y - 1,cost,(begin->getColors_index(x, y - 1) == -1) });
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				if (y == h_) {
		//					canMove[(begin->getward() + 2) % 4] = 0;
		//				}
		//				else {
		//					canMove[(begin->getward() + 2) % 4] &= (vaildSet[(begin->getward() + 2) % 4].size() > 0);
		//				}
		//			}
		//		}
		//	}
		//}
		//for (int x = 1; x <= w_; x++) {
		//	for (int y = 1; y <= h_; y++) {
		//		auto [rx, ry] = relocatePos(x, y, begin->getward());
		//		if (canMove[(begin->getward() + 0) % 4]) {
		//			if (y < h_ && begin->getNumbers(x, y + 1) == 1 && (begin->getColors_index(x, y + 1) == -1 || begin->getColors_index(x, y) == -1 || begin->getColors_index(x, y + 1) == begin->getColors_index(x, y))) {
		//				int cost = 0;
		//				cost += MAX(0, x - Xmax) + MAX(0, Xmin - x);
		//				cost += MAX(0, (y + 1) - Ymax) + MAX(0, Ymin - (y + 1));
		//				vaildSet[(begin->getward() + 0) % 4].insert({ x,y + 1,cost,(begin->getColors_index(x, y + 1) == -1) });
		//			}
		//			if (begin->getNumbers(x, y) > 1) {
		//				if (y == 1) {
		//					canMove[(begin->getward() + 0) % 4] = 0;
		//				}
		//				else {
		//					canMove[(begin->getward() + 0) % 4] &= (vaildSet[(begin->getward() + 0) % 4].size() > 0);
		//				}
		//			}
		//		}
		//	}
		//}

		//if (begin->getward() & 1)swap(w_, h_);
		//	都没有可激活块的话，就算随机到收敛了
		//cout << "当前代价矩阵：0\n";
		//for (int y = h_; y >= 1; y--) {
		//	for (int x = 1; x <= w_; x++) {
		//		cout << movingCost[0][y][x] << "\t\n"[x == w_];
		//	}
		//}
		//cout << "当前代价矩阵：[0]\n";
		//for (int y = h_; y >= 1; y--) {
		//	for (int x = 1; x <= w_; x++) {
		//		auto [rx, ry] = relocatePos(x, y, (begin->getward() + 0) % 4);
		//		cout << movingCost[0][ry][rx] << "\t\n"[x == w_];
		//	}
		//}

		//cout << "当前颜色下标矩阵：{0}\n";
		//for (int y = h_; y >= 1; y--) {
		//	for (int x = 1; x <= w_; x++) {
		//		auto [rx, ry] = relocatePos(x, y, 0);
		//		cout << begin->colors[rx][ry] << "\t\n"[x == w_];
		//	}
		//}

		//cout << "[" << vaildSet[0].size() << " " << vaildSet[1].size() << " " << vaildSet[2].size() << " " << vaildSet[3].size() << "\n";
		//for (auto b : vaildSet[0]) {
			//cout << "[" << b.x << " " << b.y << "]";
		//}
		//cout << canMove[0] << " " << canMove[1] << " " << canMove[2] << " " << canMove[3] << "]\n";

		//	如果四个方向上都没有新的可激活块，或者都不能移动，那么就结束逆燃烧迭代操作
		if (vaildSet[0].empty() && vaildSet[1].empty() && vaildSet[2].empty() && vaildSet[3].empty())break;
		if (!canMove[0] && !canMove[1] && !canMove[2] && !canMove[3])break;
	}

	if (begin->getward() & 1)swap(w_, h_);
	//	规范化，找到所有剩余的空闲块，随便指派个颜色
	for (int y = 1; y <= h_; y++) {
		for (int x = 1; x <= w_; x++) {
			if (begin->getColors_index(x, y) == -1) {
				int color = rnd(0, palette.size() - 1);
				begin->setColors(x, y, color);
				//cout << color << " ";
			}
			if (y == h_) {
				begin->setNumbers(x, y, norm_rnd(cnt, 2.58) + 1);//	干扰数，这些数会在第一次操作时牺牲掉
			}
		}
	}
	if (begin->getward() & 1)swap(w_, h_);

	int chooseWard = rnd(0, 3);//	初始方向干扰
	while (begin->getward() != chooseWard) {
		clock(Xmin, Ymin, begin->getward());
		clock(Xmax, Ymax, begin->getward());
		begin->clockwise();
	}

	//	直接逆向操作序列即可获得答案序列
	std::reverse(begin->getNow().begin(), begin->getNow().end());
	for (char& c : begin->getNow()) {//	还要反转一下顺时针逆时针的方向
		if (c == 'C')c = 'A';
		else if (c == 'A')c = 'C';
	}
	//cout << begin->now;
	end->now = begin->now;//	保存答案序列信息
	if (begin->getward() != 0) {//	保存终点方向信息
		end->goalward = 4 - begin->getward();
	}
	else {
		end->goalward = 0;
	}
	//if (strict == 800) {
	//}
}

CMap* CustomManager::GetrandomMap(int w,int h,vector<ImVec4>* palette,int maxstep,int minstep) {
	CMap* tmp=new CMap();
	tmp->setSize(w, h);
	tmp->setPalette(palette);
	tmp->randomDraw();
	tmp->randomStep(maxstep, minstep);
	return tmp;
}

void CustomManager::LoadMapFromImage(IMAGE_ img) {
	CMap* map_ = new CMap();
	//cout << img.w << " " << img.h;
	map_->setSize(img.w, img.h);
	palette.clear();

	for (int i = 0; i < img.h; ++i) {
		for (int j = 0; j < img.w; ++j) {
			int offset = i * img.w + j;
			palette.push_back(ImGui::ColorConvertU32ToFloat4(img.data[offset * 4 + 0] + img.data[offset * 4 + 1] * 256 + img.data[offset * 4 + 2] * 256 * 256 + 0xFF000000));
			map_->setColors(j + 1, i + 1, offset);
		}
	}
	map_->setPalette(&palette);

	setBegin(map_);
	delete map_;
}

//将地图保存为json格式
void CustomManager::SaveMap(string path,string name,int ID, bool isRR) {
	Json::Value R;
	R["name"] = name;
	R["ID"] = ID;
	R["colorCnt"] = palette.size();
	//初始图方向默认为0
	//目标的旋转方向，有这个的话，在展示目标图时会旋转一下矩阵
	R["goalward"] = end->goalward;

	int w_, h_;
	begin->getSize(w_, h_);
	if (begin->getward() & 1)swap(w_, h_);
	R["x"] = w_;
	R["y"] = h_;
	//if (isRR)
		end->ward = begin->ward;

	R["palette"] = Json::arrayValue;
	for (int i = 0; i < R["colorCnt"].asInt(); i++) {
		Json::Value c = Json::arrayValue;
		c.append(int(palette[i].x * 255));
		c.append(int(palette[i].y * 255));
		c.append(int(palette[i].z * 255));
		R["palette"].append(c);
	}

	R["colors"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(begin->getColors_index(j, i));
			//row.append(begin->colors[i][j]);
		}
		R["colors"].append(row);
	}

	R["numbers"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(begin->getNumbers(j, i));
			//row.append(begin->numbers[i][j]);
		}
		R["numbers"].append(row);
	}
	//
	R["goalColors"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(end->getColors_index(j, i));
			//row.append(end->colors[i][j]);
		}
		R["goalColors"].append(row);
	}

	R["ans"] = end->getNow();

	Json::StreamWriterBuilder writebuild;
    writebuild["emitUTF8"] = true;  //utf8支持,加这句,utf8的中文字符会编程\uxxx

    string document = Json::writeString(writebuild, R);

	//	如果用户非要删掉Map文件夹的话
	WIN32_FIND_DATA findData;
	if (FindFirstFile(L"Map", &findData) == INVALID_HANDLE_VALUE) {
		CreateDirectoryA("Map", NULL);
	}
	//	其实可以把下面的writeFile函数修改成：“自动遍历路径，检查是否存在目录，不存在就自己创一个”，
	//	但是用到writeFile写到别的文件夹的操作好像就只有这里，不值得为这个功能专门重新设计writeFile

	writeFileFromString(path, ENCODE(document));
}

//	保存地图到指定路径
void CustomManager::SaveMap(string path, CMap* map)
{
	Json::Value R;
	R["name"] = map->name;
	R["ID"] = map->ID;
	R["colorCnt"] = map->colorCnt;
	R["goalward"] = map->goalward;
	R["x"] = map->w;
	R["y"] = map->h;

	R["palette"] = Json::arrayValue;
	for (int i = 0; i < R["colorCnt"].asInt(); i++) {
		Json::Value c = Json::arrayValue;
		c.append(int(map->palette[i].x * 255));
		c.append(int(map->palette[i].y * 255));
		c.append(int(map->palette[i].z * 255));
		R["palette"].append(c);
	}

	R["colors"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(map->colors[i][j]);
			//row.append(begin->colors[i][j]);
		}
		R["colors"].append(row);
	}

	R["numbers"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(map->numbers[i][j]);
			//row.append(begin->numbers[i][j]);
		}
		R["numbers"].append(row);
	}
	//
	R["goalColors"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(map->goalColors[i][j]);
			//row.append(end->colors[i][j]);
		}
		R["goalColors"].append(row);
	}

	R["ans"] = map->ans;

	Json::StreamWriterBuilder writebuild;
	writebuild["emitUTF8"] = true;  //utf8支持,加这句,utf8的中文字符会编程\uxxx

	string document = Json::writeString(writebuild, R);

	//	如果用户非要删掉Map文件夹的话
	WIN32_FIND_DATA findData;
	if (FindFirstFile(L"Map", &findData) == INVALID_HANDLE_VALUE) {
		CreateDirectoryA("Map", NULL);
	}
	//	其实可以把下面的writeFile函数修改成：“自动遍历路径，检查是否存在目录，不存在就自己创一个”，
	//	但是用到writeFile写到别的文件夹的操作好像就只有这里，不值得为这个功能专门重新设计writeFile

	writeFileFromString(path, ENCODE(document));
}

void CustomManager::EndCurrentToBeginGoal()
{
	for (int y = 1; y <= h; y++)
		for (int x = 1; x <= w; x++) {
			begin->goalColors[y][x] = end->colors[y][x];
		}
}

Drawer::Drawer() {
	cx=0, cy=0;
	size=0;
	k=10;
	colorSelectedIndex = -1;
	displaySubToolBar = 0;
	beginPos = { 0,0 };
	endPos = { 0,0 };
	vaild = 0;

	map = new CMap();
	tmpStep.map = nullptr;
	//palette.push_back({ 1,0,0,1 });
}

Drawer::~Drawer() {
	clearMemory();
	delete map;
}

int Drawer::getIndex() {
	return colorSelectedIndex;
}

void Drawer::setIndex(int i) {
	colorSelectedIndex = i;
}

DrawerState Drawer::getState() {
	return state;
}

void Drawer::setState(DrawerState state_) {
	state = state_;
}

DrawerState Drawer::getSubState() {
	return substate;
}

void Drawer::setSubState(DrawerState state_) {
	substate = state_;
}

void Drawer::draw(int x, int y, int index) {
	int w, h; 
	map->getSize(w, h);
	if (x >= 1 && x <= w && y >= 1 && y <= h) {
		if (map->getColors_index(x, y) != index) {
			map->setColors(x, y, index);
			vaild |= 1;
		}
	}
}

void Drawer::line(int index) {
	for (ImVec2 pos : prettyLine(beginPos, endPos)) {
		draw(pos.x, pos.y, index);
	}
}

void Drawer::squra(int index) {
	float l, r, u, d;
	l = MIN(beginPos.x, endPos.x);
	r = MAX(beginPos.x, endPos.x);
	u = MIN(beginPos.y, endPos.y);
	d = MAX(beginPos.y, endPos.y);
	for (ImVec2 pos : prettyLine({ l,u }, { r,u })) {
		draw(pos.x, pos.y, index);
	}
	for (ImVec2 pos : prettyLine({ l,d }, { r,d })) {
		draw(pos.x, pos.y, index);
	}
	for (ImVec2 pos : prettyLine({ l,u }, { l,d })) {
		draw(pos.x, pos.y, index);
	}
	for (ImVec2 pos : prettyLine({ r,u }, { r,d })) {
		draw(pos.x, pos.y, index);
	}
}

void Drawer::squraf(int index) {
	float l, r, u, d;
	l = MIN(beginPos.x, endPos.x);
	r = MAX(beginPos.x, endPos.x);
	u = MIN(beginPos.y, endPos.y);
	d = MAX(beginPos.y, endPos.y);
	for (int x = l; x <= r; x++)
		for (int y = u; y <= d; y++) {
			draw(x, y, index);
		}
}

void Drawer::circ(int index) {
	for (ImVec2 pos : prettyCirc(beginPos, endPos)) {
		draw(pos.x, pos.y, index);
	}
}

void Drawer::circf(int index) {
	for (ImVec2 pos : prettyCircF(beginPos, endPos)) {
		draw(pos.x, pos.y, index);
	}
}

//	颜色下标轮换
void Drawer::colorsShift(int index) {
	int w, h;
	map->getSize(w, h);
	if (map->getGoalward() & 1)swap(w, h);

	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			if (map->getColors_index(x, y) > index) {
				map->setColors(x, y, map->getColors_index(x, y) - 1);
			}
			else if (map->getColors_index(x, y) == index) {
				map->setColors(x, y, palette.size());
				vaild |= 1;
			}
		}
	}
	if (colorSelectedIndex >= index) {
		colorSelectedIndex--;
	}
}

void Drawer::removeInvaildColors() {
	int w, h;
	map->getSize(w, h);
	if (map->getGoalward() & 1)swap(w, h);

	for (int y = 1; y <= h; y++) {
		for (int x = 1; x <= w; x++) {
			if (map->getColors_index(x, y) >= palette.size()) {
				map->setColors(x, y, -1);
				vaild |= 1;
			}
		}
	}
}

void Drawer::fill(int x, int y, int index) {
	if (map->getColors_index(x, y) == index)return;
	vaild |= 1;
	int w, h;
	map->getSize(w, h);
	if (map->getGoalward() & 1)swap(w, h);
	int fillcolor = map->getColors_index(x, y);

	std::queue<pair<int, int>> q;
	set<pair<int, int>> find;
	q.push({ x,y });
	find.insert({ x,y });
	while (q.size()) {
		pair<int, int> now = q.front(); q.pop();
		int x = now.first, y = now.second;

		if (x - 1 >= 1 && !find.count({ x - 1,y }) && map->getColors_index(x - 1, y) == fillcolor) q.push({ x - 1,y }), find.insert({ x - 1,y });
		if (y - 1 >= 1 && !find.count({ x,y - 1 }) && map->getColors_index(x, y - 1) == fillcolor) q.push({ x,y - 1 }), find.insert({ x,y - 1 });
		if (x + 1 <= w && !find.count({ x + 1,y }) && map->getColors_index(x + 1, y) == fillcolor) q.push({ x + 1,y }), find.insert({ x + 1,y });
		if (y + 1 <= h && !find.count({ x,y + 1 }) && map->getColors_index(x, y + 1) == fillcolor) q.push({ x,y + 1 }), find.insert({ x,y + 1 });
	}

	for (pair<int, int> pos : find) {
		map->setColors(pos.first, pos.second, index);
	}
}
//===========
void Drawer::setBeginPos(ImVec2 pos) {
	beginPos = pos;
}

void Drawer::setEndPos(ImVec2 pos) {
	endPos = pos;
}

ImVec2 Drawer::getBeginPos() {
	return beginPos;
}

ImVec2 Drawer::getEndPos() {
	return endPos;
}

bool Drawer::getVaild() {
	return vaild;
}

void Drawer::resetVaild() {
	vaild = 0;
}

void Drawer::saveCurrentStep() {//	记录当前转态
	if (tmpStep.map != nullptr) {
		delete tmpStep.map;
	}
	tmpStep.map = new CMap(map);
	tmpStep.palette = palette;
	tmpStep.colorSelectedIndex = colorSelectedIndex;
}

bool Drawer::canBack() {//	是否可撤回
	return back.size();
}

bool Drawer::canNext() {//	是否可重做
	return forward.size();
}

void Drawer::save() {//	保存当前转态到撤回栈
	DrawerStep now;
	now.map = new CMap(tmpStep.map);
	now.palette = tmpStep.palette;
	now.colorSelectedIndex = tmpStep.colorSelectedIndex;
	back.push(now);
}

void Drawer::resave() {//	保存当前转态到重做栈
	DrawerStep now;
	now.map = map;
	now.palette = palette;
	now.colorSelectedIndex = colorSelectedIndex;
	forward.push(now);
	map = new CMap(map);
}

void Drawer::retreat() {//	撤回
	if (back.empty()) {
		return;
	}
	resave();
	map = back.top().map;
	palette = back.top().palette;
	colorSelectedIndex = back.top().colorSelectedIndex;
	back.pop();
}

void Drawer::reretreat() {//	重做
	if (forward.empty()) {
		return;
	}
	saveCurrentStep();
	save();
	map = forward.top().map;
	palette = forward.top().palette;
	colorSelectedIndex = forward.top().colorSelectedIndex;
	forward.pop();
}

void Drawer::clearNextStep() {//	清空重做栈
	while (forward.size()) {
		delete forward.top().map;
		forward.pop();
	}
}

void Drawer::clearMemory() {//		清空Drawer所有内存
	while (back.size()) {
		delete back.top().map;
		back.pop();
	}
	while (forward.size()) {
		delete forward.top().map;
		forward.pop();
	}
	//	一个坑：delete后，指针不是空指针，所以要手动指向空指针
	if (tmpStep.map != nullptr) {
		delete tmpStep.map;
		tmpStep.map = nullptr;
	}
}

double distEuclidean(const ImVec4& a, const ImVec4& b) {
	ImVec4 tmp = (a - b) * (a - b);
	return sqrt(tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z);
}

std::vector<ImVec4> kMeans(CMap* MAP, int K, std::vector<ImVec4> userProvided, int maxIterations, double epsilon) {
	// 初始化随机数生成器
	std::random_device rd;
	std::mt19937 gen(time(0));
	int w, h;
	MAP->getSize(w, h);
	std::uniform_int_distribution<int> distr(0, w * h - 1);
	// 选择初始聚类中心
	std::vector<ImVec4> samples;
	std::vector<ImVec4> centroids;
	std::vector<ImVec4> data = MAP->getPalette();

	//  kmeans算法，O(k)级别的数据预处理算法，就用这个吧。然后再给函数加个接口，由用户自定义选k个初始色
	if (userProvided.empty()) {
		for (int i = 0; i < K; ++i) {
			int idx = distr(gen);
			samples.push_back(data[idx]);//
		}
	}
	else {
		for (int i = 0; i < userProvided.size() && i < K; i++) {
			samples.push_back(userProvided[i]);
		}
		for (int i = userProvided.size(); i < K; ++i) {
			int idx = distr(gen);
			samples.push_back(data[idx]);
		}
	}


	//  kmeans++算法，很慢，O(k^2*n*m)级别的数据预处理算法，打算弃用，让用户自输入吧
	//int idx = distr(gen);
	//samples.push_back(ImGui::ColorConvertU32ToFloat4(img.data[idx * 4 + 0] + img.data[idx * 4 + 1] * 256 + img.data[idx * 4 + 2] * 256 * 256));
	//ImVec4 newCen;
	//long double dis;
	//for (int i = 1; i < K; ++i) {
	//    dis = LDBL_MAX;
	//    newCen = { 0,0,0,0 };
	//    for (int idx = 0; idx < img.h * img.w; idx++) {
	//        long double dis_ = 0;
	//        ImVec4 now = ImGui::ColorConvertU32ToFloat4(img.data[idx * 4 + 0] + img.data[idx * 4 + 1] * 256 + img.data[idx * 4 + 2] * 256 * 256);
	//        for (ImVec4 exist : samples) {
	//            dis_ += distEuclidean(now, exist);
	//        }
	//        if (dis_ < dis) {
	//            dis = dis_;
	//            newCen = now;
	//        }
	//    }
	//    samples.push_back(newCen);
	//}

	std::vector<std::vector<int>> assignments;
	centroids = samples;
	// 迭代直到收敛或达到最大迭代次数
	for (int iter = 0; iter < maxIterations; ++iter) {
		assignments.assign(h, std::vector<int>(w));
		// 为每个像素分配最近的聚类中心
		for (int i = 0; i < h; ++i) {
			for (int j = 0; j < w; ++j) {
				double minDist = DBL_MAX;
				int minIdx = -1;
				for (int k = 0; k < K; ++k) {
					int offset = i * w + j;
					double dist = distEuclidean(data[offset], centroids[k]);
					if (dist < minDist) {
						minDist = dist;
						minIdx = k;
					}
				}
				assignments[i][j] = minIdx;
			}
		}
		// 计算新的聚类中心
		std::vector<ImVec4> newCentroids(K);
		for (int k = 0; k < K; ++k) {
			int count = 0;
			ImVec4 sum = ImVec4(0, 0, 0, 0);
			for (int i = 0; i < h; ++i) {
				for (int j = 0; j < w; ++j) {
					if (assignments[i][j] == k) {
						int offset = i * w + j;
						sum = sum + data[offset];
						++count;
					}
				}
			}
			if (count > 0) {
				newCentroids[k].x = sum.x / count;
				newCentroids[k].y = sum.y / count;
				newCentroids[k].z = sum.z / count;
			}
		}

		// 检查是否收敛
		double diff = 0;
		for (int k = 0; k < K; ++k) {
			ImVec4 tmp = newCentroids[k] - centroids[k];
			diff += sqrt(tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z);
		}
		if (diff <= epsilon) {
			centroids = newCentroids;
			//std::cout << iter << "轮\n";
			break; // 收敛，结束迭代
		}
		else {
			centroids = newCentroids;
		}
	}

	for (ImVec4& color : centroids) {
		color.w = 1.0;
	}

	MAP->setSize(w, h);
	MAP->setPalette(&centroids);
	for (int i = 0; i < h; ++i) {
		for (int j = 0; j < w; ++j) {
			MAP->setColors(j + 1, i + 1, assignments[i][j]);
			//cout << centroids[assignments[i][j]] << " \n"[j == w - 1];
		}
	}

	return centroids;
}

Json::Value SaveMapAsJson(CMap* map)
{
	Json::Value R;
	R["name"] = map->name;
	R["ID"] = map->ID;
	R["colorCnt"] = map->colorCnt;
	R["goalward"] = map->goalward;
	R["ward"] = map->ward;
	R["x"] = map->w;
	R["y"] = map->h;

	R["palette"] = Json::arrayValue;
	for (int i = 0; i < R["colorCnt"].asInt(); i++) {
		Json::Value c = Json::arrayValue;
		c.append(int(map->palette[i].x * 255));
		c.append(int(map->palette[i].y * 255));
		c.append(int(map->palette[i].z * 255));
		R["palette"].append(c);
	}

	R["colors"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(map->colors[i][j]);
			//row.append(begin->colors[i][j]);
		}
		R["colors"].append(row);
	}

	R["numbers"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(map->numbers[i][j]);
			//row.append(begin->numbers[i][j]);
		}
		R["numbers"].append(row);
	}
	//
	R["goalColors"] = Json::arrayValue;
	for (int i = 1; i <= R["y"].asInt(); i++) {
		Json::Value row = Json::arrayValue;
		for (int j = 1; j <= R["x"].asInt(); j++) {
			row.append(map->goalColors[i][j]);
			//row.append(end->colors[i][j]);
		}
		R["goalColors"].append(row);
	}

	R["ans"] = map->ans;
	return R;
}

void CMap::LoadMapFromJson(Json::Value R)
{
	if (R == Json::nullValue) {
		ID = -1;
		return;
	}
	if (R["ID"] == Json::nullValue) {
		ID = -1;
		return;
	}
	name = R["name"].asString();
	ID = R["ID"].asInt();
	w = R["x"].asInt();
	h = R["y"].asInt();
	colorCnt = R["colorCnt"].asInt();

	if (palette != nullptr)delete palette;
	palette = new ImVec4[colorCnt];
	for (int i = 0; i < colorCnt; i++) {
		int r, g, b;
		r = R["palette"][i][0].asInt();
		g = R["palette"][i][1].asInt();
		b = R["palette"][i][2].asInt();
		palette[i] = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1);
	}

	if (colors != nullptr)delete[] colors;
	colors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		colors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			colors[i][j] = R["colors"][i - 1][j - 1].asInt();
		}
	}

	if (numbers != nullptr)delete[] numbers;
	numbers = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		numbers[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			numbers[i][j] = R["numbers"][i - 1][j - 1].asInt();
		}
	}

	if (R["goalward"] == Json::nullValue)R["goalward"] = 0;
	goalward = R["goalward"].asInt();
	ward = R["ward"].asInt();

	if (goalColors != nullptr)delete[] goalColors;
	goalColors = new int* [h + 1];
	for (int i = 1; i <= h; i++) {
		goalColors[i] = new int[w + 1];
		for (int j = 1; j <= w; j++) {
			goalColors[i][j] = R["goalColors"][i - 1][j - 1].asInt();
		}
	}

	ans = R["ans"].asString();
}

