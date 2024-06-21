#pragma once
#include "GLOBAL.h"
//#include <string>
//#include "json\json.h"

using namespace std;

namespace encode
{
	std::string wideToMulti(std::wstring_view sourceStr, UINT pagecode);
	std::string wideToUtf8(std::wstring_view sourceWStr);
	std::string wideToOme(std::wstring_view sourceWStr);
	std::string string_To_UTF8(const std::string& str);
	std::string UTF8_To_String(const std::string& s);
	std::wstring UTF8_To_Wstring(const std::string& s);
}

void setFont(ImFont* F);

void drawRect(float x, float y, float w, float h, ImVec4 color, bool noRelated = 0);
void drawText(float x, float y, float size, ImVec4 color, string text, bool iscenter = 1);
ImVec2 getTextSize_autoNewLine(float size, string text, float width);
void drawText_autoNewLine(float x, float y, float size, ImVec4 color, string text, float width, string tip = "", bool iscenter = 1);
void drawText_RAINBOW(float x, float y, float size, string text, bool iscenter = 1, string tip = "");
ImVec2 getTextSize(float size, string text);

template<typename T, typename T2, typename T3>
T constrict(T v, T2 minv, T3 maxv) {
	return MAX(MIN(maxv, v), minv);
};

template<typename T>
bool sgn(T v) {
	if (abs(v) < 1e-9)return 0;
	return v > 0;
}

template<typename T>
long double dis(T a, T b) {
	long double d = 0;
	for (int i = 0; i < a.size(); i++) {
		d += sqrtl((a[i] - b[i]) * (a[i] - b[i]));
	}
	return d;
}
vector<ImVec2> prettyLine(ImVec2 a, ImVec2 b);
vector<ImVec2> prettyCirc(ImVec2 a, ImVec2 b);
vector<ImVec2> prettyCircF(ImVec2 a, ImVec2 b);


//	计分板

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
	void lock();
	void unlock();
private:
	int baseTime;
	int stepCount;
	int regretCount;
	int hintCount;
	int submitFailedCount;
	bool locked;
	int _score;
	int _time;
};

string sec2time(int sec);

//	JSON文件操作与简易加密解密

bool checkFile(const wstring& filename);
void writeFileFromString(const string& filename, const string& body);
string readFileIntoString(const char* filename);
Json::Value readJsonFromString(const string& mystr);
Json::Value readJsonFile(const string& filename);
void writeJsonFile(const string& filename, const Json::Value& root);
string ENCODE(string &origin);
string DECODE(string encoded);

string JSONToEJSON(Json::Value& R);
Json::Value EJSONToJSON(string& ejson);

//	账号系统管理
bool checkName(string s);
class UserManager {
public:
	UserManager();
	~UserManager();
	void LoadInfo();
	void SaveInfo();

	int Register(string name,string password);
	bool Login(string name,string password);
	void Quit();
	void ClearHistory();
	void DestoryUser();

	Json::Value& getRawData();
	void updateMapInfo(map<int, string> exist);

	string getName();
	string getID();
	float getRating();
	ImVec4 getRatingColor(float rating);
	string getRatingName(float rating);
	void saveHistory(int mapID, int time, int penelty);
	string nulluser = "未登录";
private:
	Json::Value R;
	string CurrentUser = "未登录";

	void setDefaultUserData();
};

string trans(string s);
extern std::mt19937 myrand;
int rnd(int l, int r);
long long rnd(long long l, long long r);
int norm_rnd(int range, double k);

ImVec2 GetMousePos_WithRestrict(float Max_x, float Max_y, float Min_x = 0, float Min_y = 0, bool noRelated = 0);
ImVec2 GetMousePos_InWindow();
std::ostream& operator<<(std::ostream& COUT, ImVec2 vec);
std::ostream& operator<<(std::ostream& COUT, ImVec4 vec);
bool PaintBoard(const char* label, const ImVec2& size_arg);

//	图像加载器

struct IMAGE_{
	void* image;
	int w, h;
	unsigned char* data;
};

class ImageLoader {
public:
	ImageLoader();
	~ImageLoader();
	void Init(ID3D11Device* pd3dDevice_);
	void* LoadImageToTexture(std::string index,const char* path);
	void ScaleImage(std::string baseImg, std::string saveAs, int w, int h);
	void remove(std::string index);
	map<std::string,IMAGE_> IMG;
	float percent = 0;
private:
	ID3D11Device* pd3dDevice;
};

class AudioLoader {
public:
	static AudioLoader& get() {
		static AudioLoader instance;
		return instance;
	}
	~AudioLoader();
	void addButton(wstring path,string as);
	void addBGM(wstring path, string as);
	void playButton(string as);
	void playBGM(string as);

	void SetButtonVolume(int v);
	void SetBGMVolume(int v);
private:
	AudioLoader();
	string playing="";
	map<string, wstring> BGMs;
	map<string, wstring> Buttons;
};

bool ImageButton(const char* label, const ImVec2& size_arg, ImTextureID tex, bool noDefault = 0, ImVec2 size_tex = { 0,0 }, int ID = -1);

double CIEDE2000(float R1, float G1, float B1, float R2, float G2, float B2);