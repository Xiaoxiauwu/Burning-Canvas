#include "Tool.h"

ImFont* Font_tiny;

namespace encode
{
    std::string wideToMulti(std::wstring_view sourceStr, UINT pagecode)
    {
        auto newLen = WideCharToMultiByte(pagecode, 0, sourceStr.data(), (int)sourceStr.size(),
            nullptr, 0, nullptr, nullptr);

        std::string targetStr;
        targetStr.resize(newLen);
        WideCharToMultiByte(pagecode, 0, sourceStr.data(), (int)sourceStr.size(),
            &targetStr[0], (int)targetStr.size(), nullptr, nullptr);
        return targetStr;
    }

    std::string wideToUtf8(std::wstring_view sourceWStr)
    {
        return wideToMulti(sourceWStr, 65001);
    }
    std::string wideToOme(std::wstring_view sourceWStr)
    {
        return wideToMulti(sourceWStr, CP_OEMCP);
    }
    std::string string_To_UTF8(const std::string& str)
    {
        if (str.empty())//
        {
            return std::string();
        }
        int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

        wchar_t* pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴
        ZeroMemory(pwBuf, nwLen * 2 + 2);

        ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.length(), pwBuf, nwLen);

        int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

        char* pBuf = new char[nLen + 1];
        ZeroMemory(pBuf, nLen + 1);

        ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

        std::string retStr(pBuf);

        delete[]pwBuf;
        delete[]pBuf;

        pwBuf = NULL;
        pBuf = NULL;

        return retStr;
    }
    std::string UTF8_To_String(const std::string& s)
    {
        if (s.empty())
        {
            return std::string();
        }

        std::wstring result;

        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
        wchar_t* buffer = new wchar_t[n];

        ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer, n);

        result = buffer;
        delete[] buffer;

        std::string result2;
        int len = WideCharToMultiByte(CP_ACP, 0, result.c_str(), (int)result.size(), NULL, 0, NULL, NULL);
        char* buffer2 = new char[len + 1];
        WideCharToMultiByte(CP_ACP, 0, result.c_str(), (int)result.size(), buffer2, len, NULL, NULL);
        buffer2[len] = '\0';
        result2.append(buffer2);
        delete[] buffer2;

        return result2;
    }
    std::wstring UTF8_To_Wstring(const std::string& s) {
        if (s.empty())
        {
            return std::wstring();
        }

        std::wstring result;

        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
        wchar_t* buffer = new wchar_t[n];

        ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer, n);

        result = buffer;
        return result;
    }

    //std::wstring utf8_to_wstring(const char* utf8) {
    //    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    //    return converter.from_bytes(utf8);
    //}
    //std::string utf8_to_string(const char* utf8) {
    //    return wideToUtf8(utf8_to_wstring(utf8));
    //}
}

void setFont(ImFont *F) {
    Font_tiny = F;
}

void drawRect(float x, float y, float w, float h, ImVec4 color, bool noRelated) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        IMVEC2_ADD2(ImGui::GetWindowPos() * (1||!noRelated), ImVec2((float)x + w, (float)-ImGui::GetScrollY() * (!noRelated) + y + h)),
        IMVEC2_ADD2(ImGui::GetWindowPos() * (1||!noRelated), ImVec2((float)x, (float)-ImGui::GetScrollY() * (!noRelated) + y)),
        IMVEC4_CVT_COLU32(color),
        0,//ImGui::GetStyle().FrameRounding,
        NULL
    );
}

void drawText(float x, float y, float size, ImVec4 color, string text, bool iscenter) {
    ImGui::PushFont(Font_tiny);
    float Size = ImGui::GetFontSize();
    ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    float k = size / Size;
    ImGui::SetWindowFontScale(k);
    ImGui::SetCursorPos({ x - k * Rect.x / 2 * iscenter, y - k * Rect.y / 2 * iscenter });
    ImGui::TextColored(color, text.c_str());
    ImGui::PopFont();
    ImGui::SetWindowFontScale(1.0);
}

ImVec2 getTextSize_autoNewLine(float size, string text,float width) {
    ImGui::PushFont(Font_tiny);
    float Size = ImGui::GetFontSize();
    float k = size / Size;
    ImGui::SetWindowFontScale((float)k);

    ImGui::PushTextWrapPos(width);
    ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    ImGui::PopTextWrapPos();

    ImGui::PopFont();
    ImGui::SetWindowFontScale(1.0);
    return Rect;
    //ImGui::PushFont(Font_tiny);
    //float Size = ImGui::GetFontSize();
    //ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    //float k = size / Size;
    //ImGui::PopFont();
    //return ImVec2(Rect.x * k, Rect.y * k);
}

void drawText_autoNewLine(float x, float y, float size, ImVec4 color, string text, float width, string tip, bool iscenter) {
    ImGui::PushFont(Font_tiny);
    float Size = ImGui::GetFontSize();
    ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    float k = size / Size;
    ImGui::SetWindowFontScale(k);
    //double padding = 0;
    //double paddingY = 0;
    //for (wchar_t c : encode::UTF8_To_Wstring(text)) {
    //    ImVec2 Rect_ = ImGui::CalcTextSize(encode::wideToUtf8(wstring(L"") + c).c_str());
    //    padding += (double)Rect_.x;// * (double)k;
    //    if (padding > width||c == L'\n') {
    //        padding = Rect_.x;
    //        paddingY += Rect_.y;
    //    }
    //    ImGui::SetCursorPos({ (float)(x - k * Rect.x / 2 * iscenter + padding - Rect_.x), (float)(y - k * Rect.y / 2 * iscenter + paddingY) });
    //    ImGui::TextColored(color, encode::wideToUtf8(wstring(L"") + c).c_str());
    //    if (ImGui::IsItemHovered()) {
    //        ImGui::PopFont();// 防止字体大小被干扰
    //        ImGui::SetTooltip(tip.c_str());
    //        ImGui::PushFont(Font_tiny);
    //    }
    //}

    ImGui::PushTextWrapPos(width);
    Rect = ImGui::CalcTextSize(text.c_str());
    ImGui::SetCursorPos({ (float)(x - Rect.x / 2 * iscenter ), (float)(y - Rect.y / 2 * iscenter) });
    ImGui::TextColored(color, text.c_str());
    ImGui::PopTextWrapPos();

    //ImGui::Text("")

    if (ImGui::IsItemHovered() && tip != u8" " && tip != u8"") {
        ImGui::PopFont();// 防止字体大小被干扰
        ImGui::SetTooltip(tip.c_str());
        ImGui::PushFont(Font_tiny);
    }

    ImGui::PopFont();
    ImGui::SetWindowFontScale(1.0);
}

vector<ImVec4> rainbowColor = {
    ImVec4(255/255.0,0 / 255.0,0 / 255.0,255 / 255.0),
    ImVec4(255 / 255.0,140.0 / 255.0 ,0,255 / 255.0),
    ImVec4(255 / 255.0,255 / 255.0,0 / 255.0,255 / 255.0),
    ImVec4(0 / 255.0,255 / 255.0,0 / 255.0,255 / 255.0),
    ImVec4(3.0 / 255.0,168.0 / 255.0 ,158.0 / 255.0,255 / 255.0),
    ImVec4(0 / 255.0,0 / 255.0,255 / 255.0,255 / 255.0),
    ImVec4(170.0 / 255.0 ,0 / 255.0,170.0 / 255.0,255 / 255.0)
};
void drawText_RAINBOW(float x, float y, float size, string text, bool iscenter, string tip) {
    ImGui::PushFont(Font_tiny);
    float Size = ImGui::GetFontSize();
    ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    double k = (double)size / (double)Size;
    ImGui::SetWindowFontScale((float)k);
    double padding = 0;
    int cnt = 0;
    //text = encode::UTF8_To_String(text);
    //wstring encode::
    for (wchar_t c : encode::UTF8_To_Wstring(text)) {
        ImGui::SetCursorPos({ (float)(x - k * Rect.x / 2 * iscenter + padding), (float)(y - k * Rect.y / 2 * iscenter) });
        ImGui::TextColored(rainbowColor[cnt % 7], encode::wideToUtf8(wstring(L"") + c).c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::PopFont();// 防止字体大小被干扰
            ImGui::SetTooltip(tip.c_str());
            ImGui::PushFont(Font_tiny);
        }

        ImVec2 Rect_ = ImGui::CalcTextSize(encode::wideToUtf8(wstring(L"") + c).c_str());
        padding += (double)Rect_.x;// * (double)k;
        cnt++;
        if (Rect.x < 0.001)cnt--;
    }
    ImGui::PopFont();
    ImGui::SetWindowFontScale(1.0);
}

ImVec2 getTextSize(float size, string text) {
    ImGui::PushFont(Font_tiny);
    float Size = ImGui::GetFontSize();
    float k = size / Size;
    ImGui::SetWindowFontScale((float)k);
    ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    ImGui::PopFont();
    ImGui::SetWindowFontScale(1.0);
    return ImVec2(Rect.x, Rect.y);
    //ImGui::PushFont(Font_tiny);
    //float Size = ImGui::GetFontSize();
    //ImVec2 Rect = ImGui::CalcTextSize(text.c_str());
    //float k = size / Size;
    //ImGui::PopFont();
    //return ImVec2(Rect.x * k, Rect.y * k);
}

vector<ImVec2> prettyLine(ImVec2 a, ImVec2 b) {
    vector<ImVec2> out;
    float x1, y1, x2, y2;
    x1 = a.x;
    y1 = a.y;
    x2 = b.x;
    y2 = b.y;
    if (abs(x1 - x2) > abs(y1 - y2)) {
        if (x1 > x2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        for (int x = x1; x <= x2; x++) {
            //if (x >= w)return;
            out.push_back({ float(x), (x1 == x2) ? y1 : (1.0f*(y2 - y1) * (x - x1) / (x2 - x1) + y1+0.5f) });
        }
    }
    else {
        if (y1 > y2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        for (int y = y1; y <= y2; y++) {
            out.push_back({ (y1 == y2) ? x1 : (1.0f*(x2 - x1) * (y - y1) / (y2 - y1) + x1+0.5f), float(y) });
        }
    }
    return out;
}

vector<ImVec2> prettyCirc(ImVec2 a, ImVec2 b) {
    float x1, y1, x2, y2;
    x1 = MIN(a.x, b.x);
    y1 = MIN(a.y, b.y);
    x2 = MAX(a.x, b.x);
    y2 = MAX(a.y, b.y);

    if (x1 == x2 || y1 == y2) {
        return prettyLine(a, b);
    }
    if (x2 - x1 == 1 || y2 - y1 == 1) {
        vector<ImVec2> tmp;
        for (int x = x1; x <= x2; x++)
            for (int y = y1; y <= y2; y++) {
                tmp.push_back({ 1.0f * x,1.0f * y });
            }
        return tmp;
    }

    long double A = (x1 - x2) / 2.0;
    long double B = (y1 - y2) / 2.0;
    set<pair<int,int>> out_;
    for (int x = x1; x <= x2; x++) {
        int ix = x - (x1 + x2) / 2;
        out_.insert({ x,sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2+0.5 });
        out_.insert({ x,-sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2+0.5 });
    }
    swap(A, B);
    swap(x1, y1);
    swap(x2, y2);
    for (int x = x1; x <= x2; x++) {
        int ix = x - (x1 + x2) / 2;
        if (!out_.count({ sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 - 1 ,x }) && !out_.count({ sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 + 1 ,x }))
            out_.insert({ sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 ,x });
        if (!out_.count({ -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 - 1 ,x }) && !out_.count({ -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 + 1 ,x }))
            out_.insert({ -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 ,x });
    }

    vector<ImVec2> out;
    for (auto& pos : out_) {
        out.push_back({ 1.0f * pos.first,1.0f * pos.second });
    }
    return out;
}

vector<ImVec2> prettyCircF(ImVec2 a, ImVec2 b) {
    float x1, y1, x2, y2;
    x1 = MIN(a.x, b.x);
    y1 = MIN(a.y, b.y);
    x2 = MAX(a.x, b.x);
    y2 = MAX(a.y, b.y);

    if (x1 == x2 || y1 == y2) {
        return prettyLine(a, b);
    }
    if (x2 - x1 == 1 || y2 - y1 == 1) {
        vector<ImVec2> tmp;
        for (int x = x1; x <= x2; x++)
            for (int y = y1; y <= y2; y++) {
                tmp.push_back({ 1.0f * x,1.0f * y });
            }
        return tmp;
    }

    long double A = (x1 - x2) / 2.0;
    long double B = (y1 - y2) / 2.0;
    set<pair<int, int>> out_;
    for (int x = x1; x <= x2; x++) {
        int ix = x - (x1 + x2) / 2;
        for (int y = -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5; y <= sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5; y++)
            out_.insert({ x,y });
    }
    swap(A, B);
    swap(x1, y1);
    swap(x2, y2);
    for (int x = x1; x <= x2; x++) {
        int ix = x - (x1 + x2) / 2;
        for (int y = -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5; y <= sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5; y++)
            out_.insert({ y,x });
        //if (!out_.count({ sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 - 1 ,x }) && !out_.count({ sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 + 1 ,x }))
        //    out_.insert({ sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 ,x });
        //if (!out_.count({ -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 - 1 ,x }) && !out_.count({ -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 + 1 ,x }))
        //    out_.insert({ -sqrtl(B * B - B * B * ix * ix / (A * A)) + (y1 + y2) / 2 + 0.5 ,x });
    }

    vector<ImVec2> out;
    for (auto& pos : out_) {
        out.push_back({ 1.0f * pos.first,1.0f * pos.second });
    }
    return out;
}

ScoreBoard::~ScoreBoard() {

}

ScoreBoard::ScoreBoard() {
    baseTime = clock();
    stepCount = 0;
    regretCount = 0;
    hintCount = 0;
    submitFailedCount = 0;
    locked = 0;
    _score = 0;
    _time = 0;
}

void ScoreBoard::Reset() {
    baseTime = clock();
    stepCount = 0;
    regretCount = 0;
    hintCount = 0;
    submitFailedCount = 0;
    locked = 0;
    _score = 0;
    _time = 0;
}

int ScoreBoard::GetTime() {
    if (locked)return _time;
    return (clock() - baseTime) / CLOCKS_PER_SEC;
}

int ScoreBoard::calcScore() {
    if (locked)return _score;
    return stepCount * 10 + regretCount * 20 + hintCount * 100 + GetTime() * 2 + submitFailedCount * 3;
}

void ScoreBoard::step() {
    stepCount++;
}

void ScoreBoard::regret() {
    regretCount++;
}

void ScoreBoard::hint() {
    hintCount++;
}

void ScoreBoard::submit() {
    submitFailedCount++;
}

void ScoreBoard::lock() {
    _score = calcScore();
    _time = GetTime();
    locked = 1;
}

void ScoreBoard::unlock() {
    locked = 0;
    _score = 0;
    _time = 0;
}

string sec2time(int sec) {
    string m = to_string(sec / 60);
    string s = to_string(sec % 60);
    if (m.length() < 2)m = "0" + m;
    if (s.length() < 2)s = "0" + s;
    return m + ":" + s;
}

//  文件读写与账户信息管理

bool checkFile(const wstring& filename) {
    WIN32_FIND_DATA findData;
    return (FindFirstFile(filename.c_str(), &findData) != INVALID_HANDLE_VALUE);
}

void writeFileFromString(const string& filename, const string& body)
{
    ofstream ofile(filename);
    ofile << body;
    ofile.close();
}

string readFileIntoString(const char* filename)
{
    ifstream ifile(filename);
    if (!ifile.is_open()) {
        cerr << "文件打开错误"; return "";
    }
    ostringstream buf;
    string filestr;
    char ch;
    while (buf && ifile.get(ch)) {
        buf.put(ch);
    }
    filestr = buf.str();
    return filestr;
}

Json::Value readJsonFromString(const string& mystr)
{
    Json::CharReaderBuilder ReaderBuilder;
    ReaderBuilder["emitUTF8"] = true;   //utf8支持,不加这句,utf8的中文字符会编程\uxxx

    unique_ptr<Json::CharReader> charread(ReaderBuilder.newCharReader());

    Json::Value root;

    string strerr;
    bool isok = charread->parse(mystr.c_str(), mystr.c_str() + mystr.size(), &root, &strerr);
    if (!isok || strerr.size() != 0) {
        cerr << "json解析出错";
        //Json::Value R;
        //R["JSONERROR"] = 1;
        //return R;
    }

    return root;
}

Json::Value readJsonFile(const string& filename)
{
    ifstream ifile;
    ifile.open(filename);

    Json::CharReaderBuilder ReaderBuilder;
    ReaderBuilder["emitUTF8"] = true;// utf8支持，不加这句，utf8的中文字符会编程\uxxx

    Json::Value root;

    string strerr;
    bool ok = Json::parseFromStream(ReaderBuilder, ifile, &root, &strerr);
    if (!ok) {
        cerr << "json解析错误";
    }
    return root;
}

void writeJsonFile(const string& filename, const Json::Value& root) {
    Json::StreamWriterBuilder writebuild;
    writebuild["emitUTF8"] = true;  //utf8支持,加这句,utf8的中文字符会编程\uxxx

    string document = Json::writeString(writebuild, root);

    writeFileFromString(filename, document);
}

string ENCODE(string &origin) {
    if (origin == "")return "";
    string tmp = base64::encode(origin);
    for (auto& c : tmp) {
        if (c != '+' && c != '/' && c != '=')c ^= 1;
        if (c == '`')c = 'z';
        if (c == '{')c = 'a';
        if (c == '@')c = 'Z';
        if (c == '[')c = 'A';
    }
    return tmp;
}

string DECODE(string encoded) {
    if (encoded == "")return "";
    for (auto& c : encoded) {
        if (c == 'z')c = '`';
        if (c == 'a')c = '{';
        if (c == 'Z')c = '@';
        if (c == 'A')c = '[';
        if (c != '+' && c != '/' && c != '=')c ^= 1;
    }
    return base64::decode(encoded);
}

string JSONToEJSON(Json::Value& R)
{
    Json::StreamWriterBuilder writebuild;
    writebuild["emitUTF8"] = true;  //utf8支持,加这句,utf8的中文字符会编程\uxxx
    string document = Json::writeString(writebuild, R);
    return ENCODE(document);
}

Json::Value EJSONToJSON(string& ejson)
{
    return readJsonFromString(DECODE(ejson));
}

UserManager::UserManager(){}
UserManager::~UserManager(){}

void UserManager::LoadInfo() {
    string rawInfo = readFileIntoString("userinfo.dat");
    R = readJsonFromString(DECODE(rawInfo));
    if (R == Json::nullValue)setDefaultUserData();
}

void UserManager::SaveInfo() {
    Json::StreamWriterBuilder writebuild;
    writebuild["emitUTF8"] = true;  //utf8支持,加这句,utf8的中文字符会编程\uxxx

    string document = Json::writeString(writebuild, R);
    //cout << R;
    //cout << "保存用户信息成功";
    writeFileFromString("userinfo.dat", ENCODE(document));
}

bool checkPassword(string s) {
    if (s.length() < 6||s.length() > 20)return false;
    return true;
}

bool checkName(string s) {
    
    bool onlyspace = 1;//用户名为纯空格
    for (auto c : s) {
        if (c != ' ') {
            onlyspace = 0;
            break;
        }
    }
    if (onlyspace || s.length() < 1 || s.length() > 15)return false;
    if (s == "未登录")return false;
    return true;
}

int UserManager::Register(string name,string password="123456") {
    if (!checkName(name))return -1;//名字不合法
    if (!checkPassword(password))return 0;//密码不合法
    for (string username : R["User"].getMemberNames()) {
        if (username == name) {
            return -2;//    已存在
        }
    }
    R["User"][name] = Json::objectValue;
    R["User"][name]["password"] = password;
    R["User"][name]["ID"] = time(0);
    R["User"][name]["mapState"] = Json::objectValue;
    R["CurrentUser"] = name;
    return 1;
}

bool UserManager::Login(string name, string password) {
    for (string username : R["User"].getMemberNames()) {
        if (username == name) {
            if (R["User"][name]["password"] != password) {
                return 0;
            }
            else {
                R["CurrentUser"] = name;
                return 1;
            }
        }
    }
    return 0;
}

void UserManager::Quit() {
    R["CurrentUser"] = nulluser;
}

void UserManager::ClearHistory() {
    if (R["CurrentUser"] != nulluser)R["User"][R["CurrentUser"].asString()]["mapState"] = Json::objectValue;
}

void UserManager::DestoryUser() {
    if (R["CurrentUser"] != nulluser)R["User"].removeMember(R["CurrentUser"].asString());
    R["CurrentUser"] = nulluser;
}

Json::Value& UserManager::getRawData()
{
    return R;
}

void UserManager::updateMapInfo(map<int,string> exist) {
    for (string name : R["User"].getMemberNames()) {
        for (string mapID : R["User"][name]["mapState"].getMemberNames()) {
            if (!exist.count(stoi(mapID))) {
                R["User"][name]["mapState"].removeMember(mapID);
            }
        }
    }
}

std::string UserManager::getName() {
    return R["CurrentUser"].asString();//这里出来的串是非UTF8编码的，要记得转换
}

string UserManager::getID()
{
    return R["User"][R["CurrentUser"].asString()]["ID"].asString();
}

float UserManager::getRating()
{
    return R["User"][R["CurrentUser"].asString()]["Rating"].asFloat();
}

ImVec4 UserManager::getRatingColor(float rating)
{
    if(rating == 0)return ImVec4(128 / 255.0, 128 / 255.0, 128 / 255.0, 255 / 255.0);//灰Unrated
    else if (rating < 10 * (1200.0 / 3000.0))return ImVec4(100 / 255.0, 200.0 / 255.0, 100.0 / 255.0, 255 / 255.0);//浅绿Newbie
    else if (rating < 10 * (1400.0 / 3000.0))return ImVec4(0 / 255.0, 128 / 255.0, 0 / 255.0, 255 / 255.0);//绿Pupil
    else if (rating < 10 * (1600.0 / 3000.0))return ImVec4(3.0 / 255.0, 168.0 / 255.0, 158.0 / 255.0, 255.0 / 255.0);//青Specialist
    else if (rating < 10 * (1900.0 / 3000.0))return ImVec4(0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0);//蓝Expert
    else if (rating < 10 * (2100.0 / 3000.0))return ImVec4(170.0 / 255.0, 0.0 / 255.0, 170.0 / 255.0, 255.0 / 255.0);//紫Candidate Master
    else if (rating < 10 * (2400.0 / 3000.0))return ImVec4(255.0 / 255.0, 140.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0);//橙Master
    else if (rating < 10 * (3000.0 / 3000.0))return ImVec4(255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0);//红Grand Master
    return ImVec4(0, 0, 0, 0);//   彩Legendary GrandMaster
}

string UserManager::getRatingName(float rating)
{
    if (rating == 0)return "Unrated";//灰Unrated
    else if (rating < 10 * (1200.0 / 3000.0))return "Newbie";//浅绿Newbie
    else if (rating < 10 * (1400.0 / 3000.0))return "Pupil";//绿Pupil
    else if (rating < 10 * (1600.0 / 3000.0))return "Specialist";//青Specialist
    else if (rating < 10 * (1900.0 / 3000.0))return "Expert";//蓝Expert
    else if (rating < 10 * (2100.0 / 3000.0))return "Candidate Master";//紫Candidate Master
    else if (rating < 10 * (2400.0 / 3000.0))return "Master";//橙Master
    else if (rating < 10 * (3000.0 / 3000.0))return "Grand Master";//红Grand Master
    return "Legendary GrandMaster";//   彩Legendary GrandMaster
}

void UserManager::saveHistory(int mapID,int time,int penelty) {
    //cout << R["User"][R["CurrentUser"].asString()][];
    if (R["User"][R["CurrentUser"].asString()]["mapState"][to_string(mapID)] == Json::nullValue) {//
        R["User"][R["CurrentUser"].asString()]["mapState"][to_string(mapID)] = Json::arrayValue;
    }
    Json::Value tmp;
    tmp["time"] = time;
    tmp["penelty"] = penelty;
    R["User"][R["CurrentUser"].asString()]["mapState"][to_string(mapID)].append(tmp);
}

void UserManager::setDefaultUserData() {
    R = Json::objectValue;
    R["User"] = Json::objectValue;
    R["CurrentUser"] = nulluser;
    R["FX Volume"] = 130;
    R["BGM Volume"] = 220;
    R["noColorCheck"] = 0;
}

string trans(string s) {// \uxxxx解码
    return Json::Value(s).asString();
}

std::mt19937 myrand(unsigned int(time(0)));

long long rnd(long long l, long long r) {
    return std::uniform_int_distribution<long long>(min(l, r), max(l, r))(myrand);
}
int rnd(int l, int r) {
    return std::uniform_int_distribution<int>(min(l, r), max(l, r))(myrand);
}

//  生成范围：（0 到 range）
//  多少sigma算小概率样本点：k
int norm_rnd(int range, double k) {
    int out = range;
    while (out >= range) {
        out = (int)abs(std::normal_distribution<double>(0.0, (double)(range / k))(myrand));
    }
    return out;
}

std::ostream& operator<<(std::ostream& COUT, ImVec2 vec) {
    COUT << "[" << vec.x << "," << vec.y << "]";
    return COUT;
}

std::ostream& operator<<(std::ostream& COUT, ImVec4 vec) {
    COUT << "[" << vec.x << "," << vec.y << "," << vec.z << "," << vec.w << "]";
    return COUT;
}

ImVec2 GetMousePos_WithRestrict(float Max_x, float Max_y, float Min_x, float Min_y, bool noRelated) {
    ImVec2 tmp = ImGui::GetMousePos();
    tmp -= ImGui::GetWindowPos() * (!noRelated);
    tmp.x = max(Min_x, tmp.x);
    tmp.y = max(Min_y, tmp.y);
    if (tmp.x > Max_x)tmp.x = Max_x;
    if (tmp.y > Max_y)tmp.y = Max_y;
    tmp.x = min(Max_x, tmp.x);
    tmp.y = min(Max_y, tmp.y);
    tmp += ImGui::GetWindowPos() * (!noRelated);
    return tmp;
}

ImVec2 GetMousePos_InWindow() {
    return ImGui::GetMousePos()-ImGui::GetWindowPos();
}

bool PaintBoard(const char* label, const ImVec2& size_arg)
{
    //ImGuiButtonFlags_ flags = ImGuiButtonFlags_None;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    //ImVec2 tmp = GetMousePos_WithRestrict(400.0f + 10, 380.0f + 10, 10, 10);
    //std::string ppos = std::to_string(tmp.x) + "," + std::to_string(tmp.y);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    //ImVec2 pos = { dx,dy };


    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

    // Render
    //const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    //const ImU32 col = ImGui::GetColorU32(ImGuiCol_Button);
    //ImGui::RenderNavHighlight(bb, id);
    //ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    //drawRect(pos.x, pos.y, size_arg.x, size_arg.y, ImVec4(1, 0, 1, 1), 1);
    //ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    return pressed;
}

ImageLoader::ImageLoader() {
    pd3dDevice = nullptr;
}

ImageLoader::~ImageLoader() {
    ID3D11ShaderResourceView* tmp;
    for (auto &[name, img] : IMG) {
        tmp = (ID3D11ShaderResourceView*)img.image;
        tmp->Release();
        stbi_image_free(img.data);
    }
}

void ImageLoader::Init(ID3D11Device* pd3dDevice_) {
    pd3dDevice = pd3dDevice_;
}

void* ImageLoader::LoadImageToTexture(std::string index, const char* path) {
    int channels, width, height;
    unsigned char* image_data = stbi_load(path, &width, &height, &channels, 4); // 强制加载为RGBA格式
    if (!image_data) {
        //  图像加载失败的处理
        //  不说话，生闷气，传个空指针回去让调用者自己猜
        return nullptr;
    }
    // 创建纹理
    D3D11_TEXTURE2D_DESC texDesc = {};
    ZeroMemory(&texDesc, sizeof(texDesc));
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    //texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    //texDesc.MiscFlags = 0;//D3D11_RESOURCE_MISC_TEXTURE2D;

    // 上传图像数据到纹理
    ID3D11Texture2D* texture = nullptr;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = image_data;
    initData.SysMemPitch = width * 4; // 每个像素4个字节
    initData.SysMemSlicePitch = 0;
    HRESULT hr = pd3dDevice->CreateTexture2D(&texDesc, &initData, &texture);
    if (FAILED(hr)) {
        //  创建纹理失败的处理
        //  不说话，生闷气，传个空指针回去让调用者自己猜
        stbi_image_free(image_data);
        return nullptr;
    }

    //pd3dDeviceContext->UpdateSubresource(texture, 0, nullptr, initData.pSysMem, initData.SysMemPitch, initData.SysMemSlicePitch);
    //if (FAILED(hr)) {
        // 更新纹理数据失败的处理
        // 
    //}

    // 创建Shader Resource View
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

    ID3D11ShaderResourceView* textureView = nullptr;
    hr = pd3dDevice->CreateShaderResourceView(texture, &srvDesc, &textureView);
    if (FAILED(hr)) {
        //  创建SRV失败的处理
        //  不说话，生闷气，传个空指针回去让调用者自己猜
        stbi_image_free(image_data);
        texture->Release();
        return nullptr;
    }

    if (IMG.count(index)) {
        ID3D11ShaderResourceView* tmp = (ID3D11ShaderResourceView*)IMG[index].image;
        tmp->Release();
    }
    IMG[index] = { (void*)textureView ,width,height, image_data};

    // 转换为ImGui的纹理ID
    //ImTextureID texID = (ImTextureID)textureView;
    //stbi_image_free(image_data);
    texture->Release();
    return (void*)textureView;
}

void ImageLoader::remove(std::string index) {
    if (IMG.count(index)) {
        ID3D11ShaderResourceView* tmp = (ID3D11ShaderResourceView*)IMG[index].image;
        tmp->Release();
        stbi_image_free(IMG[index].data);
        IMG.erase(index);
    }
}

void ImageLoader::ScaleImage(std::string baseImg, std::string saveAs, int w, int h) {
    unsigned char* data = new unsigned char[w * h * 4];
    percent = 0;
    int tot = 4 * h * w * IMG[baseImg].h / h * IMG[baseImg].w / w;
    int cnt = 0;
    for (int k = 0; k < 4; k++) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                long double tmp = 0;
                for (int iy = 0; iy < IMG[baseImg].h / h; iy++) {
                    for (int ix = 0; ix < IMG[baseImg].w / w; ix++) {
                        tmp += IMG[baseImg].data[
                            int(
                                (
                                    (y * IMG[baseImg].h / h + iy) * IMG[baseImg].w
                                    + x * IMG[baseImg].w / w + ix
                                )
                               ) * 4
                            + k
                        ];
                        cnt++;
                        percent = 100.0f * cnt / tot;
                    }
                }
                data[(y * w + x) * 4 + k] = tmp / ((IMG[baseImg].h / h) * (IMG[baseImg].w / w));
            }
        }
    }
    //cout << cnt << "]";

    if (!data) {
        return;
    }
    // 创建纹理
    D3D11_TEXTURE2D_DESC texDesc = {};
    ZeroMemory(&texDesc, sizeof(texDesc));
    texDesc.Width = w;
    texDesc.Height = h;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    //texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;

    ID3D11Texture2D* texture = nullptr;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = w * 4;
    initData.SysMemSlicePitch = 0;
    HRESULT hr = pd3dDevice->CreateTexture2D(&texDesc, &initData, &texture);
    if (FAILED(hr)) {
        stbi_image_free(data);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

    ID3D11ShaderResourceView* textureView = nullptr;
    hr = pd3dDevice->CreateShaderResourceView(texture, &srvDesc, &textureView);
    if (FAILED(hr)) {
        stbi_image_free(data);
        texture->Release();
    }

    if (IMG.count(saveAs)) {
        ID3D11ShaderResourceView* tmp = (ID3D11ShaderResourceView*)IMG[saveAs].image;
        tmp->Release();
    }
    IMG[saveAs] = { (void*)textureView ,w,h, data};

    texture->Release();
}

bool ImageButton(const char* label, const ImVec2& size_arg, ImTextureID tex, bool noDefault, ImVec2 size_tex, int ID) {
    if (!noDefault) {
        size_tex = size_arg;
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;
    ImVec2 lstCursorPos_Begin = ImGui::GetCursorPos();

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    
    ImGuiID id;
    if (ID != -1) {
        id = window->GetID(ID);
    }
    else {
        id = window->GetID(tex);
    }

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, size_tex.x + style.FramePadding.x * 2.0f, size_tex.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

    // Render
    const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    ImVec2 lstCursorPos_End = ImGui::GetCursorPos();

    if (ImGui::IsItemHovered() && sizeof(label) != 0)
    {
        ImGui::SetTooltip(label);
    }

    ImGui::SetCursorPos(lstCursorPos_Begin + size_arg / 2 - size_tex / 2);
    ImGui::Image(tex, size_tex);
    ImGui::SetCursorPos(lstCursorPos_End);

    return pressed;
}

// 辅助函数，将RGB值转换为CIELAB颜色空间
void RGBtoLAB(float R, float G, float B, double& L, double& a, double& b) {
    // 将RGB值归一化到[0, 100]区间
    double r_ = R;// / 255.0;
    double g_ = G;// / 255.0;
    double b_ = B;// / 255.0;

    // 将RGB转换为XYZ颜色空间
    double X = 0.4124564 * r_ + 0.3575761 * g_ + 0.1804375 * b_;
    double Y = 0.2126729 * r_ + 0.7151522 * g_ + 0.0721750 * b_;
    double Z = 0.0193339 * r_ + 0.1191920 * g_ + 0.9503041 * b_;

    // 将XYZ转换为CIELAB
    X = X / 0.95047; // 参考白点
    Y = Y / 1.0;    // 参考白点
    Z = Z / 1.08883; // 参考白点

    X = X > 0.008856 ? pow(X, 1.0 / 3.0) : (7.787 * X + 16.0 / 116.0);
    Y = Y > 0.008856 ? pow(Y, 1.0 / 3.0) : (7.787 * Y + 16.0 / 116.0);
    Z = Z > 0.008856 ? pow(Z, 1.0 / 3.0) : (7.787 * Z + 16.0 / 116.0);

    L = 116.0 * Y - 16.0;
    a = 500.0 * (X - Y);
    b = 200.0 * (Y - Z);
}

// CIEDE2000色差计算函数
double CIEDE2000(float R1, float G1, float B1, float R2, float G2, float B2) {
    double L1, a1, b1;
    double L2, a2, b2;
    RGBtoLAB(R1, G1, B1, L1, a1, b1);
    RGBtoLAB(R2, G2, B2, L2, a2, b2);
    // double deltaE00 = sqrt(pow(L1 - L2, 2) + pow(a1 - a2, 2) + pow(b1 - b2, 2));

    double C1 = sqrt(pow(a1, 2) + pow(b1, 2));
    double C2 = sqrt(pow(a2, 2) + pow(b2, 2));
    double Cmean = (C1 + C2) / 2.0;
    double G = (1 - sqrt(pow(Cmean, 7) / (pow(Cmean, 7) + pow(25.0, 7)))) / 2.0;
    double a1p = (1 + G) * a1;
    double a2p = (1 + G) * a2;

    double C1p = sqrt(pow(a1p, 2) + pow(b1, 2));
    double C2p = sqrt(pow(a2p, 2) + pow(b2, 2));
    double hdz = acos(-1) / 180.0;
    double zdh = 180.0 / acos(-1);
    auto getH = [&](double a, double b) {
        double h = 0;
        double hab = 0;

        if (a == 0) {
            return 90 * hdz;
        }

        h = atan2(b, a) * zdh;

        if (a > 0 && b > 0) {
            hab = h;
        }
        else if (a < 0 && b>0) {
            hab = 180 + h;
        }
        else if (a < 0 && b < 0) {
            hab = 180 + h;
        }
        else {
            hab = 360 + h;
        }
        return hab * hdz;
        };

    double h1 = getH(a1p, b1);//atan2(b1, a1p) * 180.0 / acos(-1);//asin(b1 / a1p);//
    double h2 = getH(a2p, b2);//atan2(b2, a2p) * 180.0 / acos(-1);//asin(b2 / a2p);//
    //std::cout << h1 << " " << h2 << "\n";
    //if (C1p * C2p == 0) {
    //    h1 = h2 = 0;
    //}
    //else {
    //    if (h2 < h1) h2 += 360.0;
    //    h2 = h2 - h1;
    //    if (h2 > 180.0) h2 -= 360.0;
    //}

    double deltaH = h2 - h1;//2 * sqrt(C1p * C2p) * sin((h2 - h1) / 2);//h2 - h1;
    double deltaL = L2 - L1;
    double deltaC = C2p - C1p;

    double deltaHp = 2.0 * sqrt(C1p * C2p) * sin(deltaH / 2);// * acos(-1) / 180.0);

    double Sl = 1.0 + ((0.015 * pow(L1 - 50.0, 2)) / sqrt(20.0 + pow(L1 - 50.0, 2)));
    double Sc = 1.0 + 0.045 * Cmean;
    double T = 1.0 - 0.17 * cos((h1 + h2) / 2 - 30 * hdz) + 0.24 * cos(2 * (h1 + h2) / 2) + 0.32 * cos(3 * (h1 + h2) / 2 + 6 * hdz) - 0.2 * cos(4 * (h1 + h2) / 2 - 63 * hdz);
    double Sh = 1.0 + 0.015 * Cmean * T;//* (1.0 - sqrt(3.0 / (Cmean + 0.03)));//

    double RC = 1 - 2 * G;
    double DeltaTheta = 30 * exp(-pow(((h1 + h2) / 2 - 275 * hdz) / 25, 2)) * hdz;
    double RT = -sin(2 * DeltaTheta) * RC;

    double deltaE00 = sqrt(pow(deltaL / Sl, 2) + pow(deltaC / Sc, 2) + pow(deltaHp / Sh, 2) + RT * deltaC / Sc * deltaHp / Sh);
    return deltaE00;
}

AudioLoader::AudioLoader()
{
}

AudioLoader::~AudioLoader()
{
}

void AudioLoader::addButton(wstring path, string as)
{
    mciSendString((wstring(L"open ") + path + wstring(L" alias ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
    mciSendString((wstring(L"setaudio ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as)) + wstring(L" volume to 130")).c_str(), 0, 0, 0);
    Buttons[as] = path;
}

void AudioLoader::addBGM(wstring path, string as)
{
    mciSendString((wstring(L"open ") + path + wstring(L" alias ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
    BGMs[as] = path;
}

void AudioLoader::playButton(string as)
{
    for (auto& [name, path] : Buttons) {
        mciSendString((wstring(L"stop ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(name))).c_str(), 0, 0, 0);
    }
    mciSendString((wstring(L"seek ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as)) + wstring(L" to 0")).c_str(), 0, 0, 0);
    mciSendString((wstring(L"play ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
    //mciSendString((wstring(L"close ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
    //mciSendString((wstring(L"play ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
}

void AudioLoader::playBGM(string as)
{
    if (playing == "") {
        mciSendString((wstring(L"open ") + BGMs[as] + wstring(L" alias ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
        mciSendString((wstring(L"play ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as)) + wstring(L" repeat")).c_str(), 0, 0, 0);
        playing = as;
    }
    else {
        mciSendString((wstring(L"stop ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(playing))).c_str(), 0, 0, 0);
        playing = as;
        //mciSendString((wstring(L"open ") + BGMs[as] + wstring(L" alias ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
        //mciSendString((wstring(L"open ") + BGMs[as] + wstring(L" alias ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as))).c_str(), 0, 0, 0);
        mciSendString((wstring(L"seek ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as)) + wstring(L" to 0")).c_str(), 0, 0, 0);
        mciSendString((wstring(L"play ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(as)) + wstring(L" repeat")).c_str(), 0, 0, 0);
    }
}

void AudioLoader::SetButtonVolume(int v)
{
    for (auto& [name, path] : Buttons) {
        mciSendString((wstring(L"setaudio ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(name)) + wstring(L" volume to ") + to_wstring(v)).c_str(), 0, 0, 0);
    }
}

void AudioLoader::SetBGMVolume(int v)
{
    for (auto& [name, path] : BGMs) {
        mciSendString((wstring(L"setaudio ") + encode::UTF8_To_Wstring(encode::string_To_UTF8(name)) + wstring(L" volume to ") + to_wstring(v)).c_str(), 0, 0, 0);
    }
}
