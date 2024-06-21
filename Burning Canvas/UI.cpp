#include "UI.h"
#include "CustomMap.h"
#include "AnimeManager.h"
#include "SocketManager.h"

/////////////////////////////////////GLOBAL VARIABLES////////////////////////////////////////
float Screen_Width;
float Screen_Height;
float width;
float height;
float buttonwidth;
int baseoffset;

vector<wstring> MapNames;
vector<Map*> Maps;
map<int, int> IDtoMaps;
map<int, string> IDtoPath;
MapDisplay MapDisplayer;

Map *SelectMap, *GameMap;
stack<Map*> LastMap;
string playerOperation;

ImFont* Font, * Font_Big;

ScoreBoard* scoreboard = new ScoreBoard();

HintState hintstate = NONE;

bool ButtonLock = 0;//  播放过场动画（粒子）时，用于锁定全局按钮的线程锁
//  subwindow的管理机制更新了，换成subWindow.size()会不会好一点？先留着吧，默认设置为0
//&&subWindow.size()==0

UserManager* userManager = new UserManager();

CustomManager* customManager = new CustomManager();
Drawer drawer;
ImageLoader* imageLoader = new ImageLoader();
SocketManager* socketManager = new SocketManager();

Map* defaultAvator = new Map();
Map* MyAvator;
/////////////////////////////////////MAIN FUNCTION////////////////////////////////////////
void refreshMapIndex() {
    IDtoMaps.clear();
    for (int i = 0; i < Maps.size(); i++) {
        IDtoMaps[Maps[i]->getID()] = i;
    }
}

void clearMap() {
    for (Map*& _map : Maps) {
        delete _map;
    }
    Maps.clear();
}

void EXIT() {
    clearMap();
    userManager->SaveInfo();
    delete scoreboard, userManager, customManager, imageLoader;
    delete defaultAvator;
    exit(0);
}

std::ostream& operator<<(std::ostream& os, std::wstring_view str)
{
    return os << encode::wideToOme(str);
}

vector <wstring> GetAllFiles(wstring path)
{
    vector<wstring> files;
    HANDLE hFind;
    WIN32_FIND_DATA findData;
    LARGE_INTEGER size;
    if (FindFirstFile(L"Map", &findData)== INVALID_HANDLE_VALUE) {
        CreateDirectoryA("Map", NULL);
    }
    hFind = FindFirstFile(path.c_str(), &findData); //搜索第一个文件，创建并返回搜索句柄，有则返回TRUE
    if (hFind == INVALID_HANDLE_VALUE)
    {
        //cout << "Failed to find first file!\n";DEBUG
        return files;
    }
    do
    {
        // 忽略"."和".."两个结果 
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)   //strcmp 比较字符，相同则返回0
            continue;
        files.push_back(findData.cFileName);
    } while (FindNextFile(hFind, &findData));
    return files;
}

void refresh() {
    MapNames = GetAllFiles(L"Map\\*.dat");
    clearMap();
    for (wstring level : MapNames) {
        Maps.push_back(new Map(encode::wideToOme(level)));
        if ((*Maps.rbegin())->getID() == -1) {
            Maps.erase(--Maps.end());
        }
        else {
            IDtoPath[(*Maps.rbegin())->getID()] = encode::wideToUtf8(level);//  这只有在删除，创建地图时候需要用到
        }
    }
    //cout << MapNames[0];

    userManager->LoadInfo();
    //if()
    //cout << userManager->getName();DEBUG
    sort(Maps.begin(), Maps.end(), [](Map* a, Map* b) {
        return a->getID() < b->getID();
    });
    refreshMapIndex();
}

void init(ID3D11Device* g_pd3dDevice) {
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;
    ImFontConfig Font_cfg;
    Font_cfg.FontDataOwnedByAtlas = false;

    //ImFont* Font = io.Fonts->AddFontFromFileTTF("..\\ImGui Tool\\Font.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    //Font = io.Fonts->AddFontFromMemoryTTF((void*)Font_data, Font_size, Screen_Width / 1024 * 18.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    //Font_Big = io.Fonts->AddFontFromMemoryTTF((void*)Font_data, Font_size, Screen_Width / 1024 * 24.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    if (checkFile(L"res\\Source Han Sans & Saira Hybrid-Regular.ttf")) {//  优先选择res里的这个字体
        Font = io.Fonts->AddFontFromFileTTF("res\\Source Han Sans & Saira Hybrid-Regular.ttf", Screen_Width / 1024 * 18.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    }
    else {
        //  防止用户误删字体文件，用户换别的字体文件的话，不一定保证字库包含中文，所以用自己的
        Font = io.Fonts->AddFontFromMemoryTTF((void*)Font_data, Font_size, Screen_Width / 1024 * 18.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    }
    
    //Font_Big = io.Fonts->AddFontFromFileTTF("Source Han Sans & Saira Hybrid-Regular.ttf", Screen_Width / 1024 * 24.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());

    setFont(Font);

    //  加载图片资源
    imageLoader->Init(g_pd3dDevice);
    imageLoader->LoadImageToTexture("drawpix", "res/drawpix.png");
    imageLoader->LoadImageToTexture("eraser", "res/eraser.png");
    imageLoader->LoadImageToTexture("line", "res/line.png");
    imageLoader->LoadImageToTexture("squra", "res/squra.png");
    imageLoader->LoadImageToTexture("squraf", "res/squraf.png");
    imageLoader->LoadImageToTexture("circ", "res/circ.png");
    imageLoader->LoadImageToTexture("circf", "res/circf.png");
    imageLoader->LoadImageToTexture("fill", "res/fill.png");
    //imageLoader->LoadImageToTexture("select", "res/select.png");
    imageLoader->LoadImageToTexture("colorpicker", "res/colorpicker.png");
    imageLoader->LoadImageToTexture("move", "res/move.png");
    imageLoader->LoadImageToTexture("center", "res/center.png");
    imageLoader->LoadImageToTexture("grid", "res/grid.png");

    imageLoader->LoadImageToTexture("back", "res/back.png");
    imageLoader->LoadImageToTexture("forward", "res/forward.png");
    imageLoader->LoadImageToTexture("penelty", "res/penelty.png");
    imageLoader->LoadImageToTexture("time", "res/time.png");
    imageLoader->LoadImageToTexture("eye", "res/eye.png");

    imageLoader->LoadImageToTexture("title0", "res/title0.png");
    imageLoader->LoadImageToTexture("title1", "res/title1.png");
    imageLoader->LoadImageToTexture("title2", "res/title2.png");
    //imageLoader->LoadImageToTexture("random", encode::wideToOme(wstring(MAKEINTRESOURCE(IDB_RANDOM))).c_str());
    imageLoader->LoadImageToTexture("random", "res/random.png");
    imageLoader->LoadImageToTexture("custom", "res/custom.png");

    imageLoader->LoadImageToTexture("rated", "res/rated.png");
    imageLoader->LoadImageToTexture("net", "res/net.png");
    imageLoader->LoadImageToTexture("server", "res/server.png");
    imageLoader->LoadImageToTexture("client", "res/client.png");
    imageLoader->LoadImageToTexture("ICON", "res/icon.ico");


    defaultAvator->loadDefault();

    //  文件管理器需要的一个初始化
    ifd::FileDialog::Instance().Init(g_pd3dDevice);
    ifd::FileDialog::Instance().CreateTexture = [](uint8_t* image_data, int width, int height, char fmt) -> void* {
        if (!image_data) {
            //  图像加载失败的处理
            //  不说话，生闷气，传个空指针回去让调用者自己猜
            return nullptr;
        }
        // 创建纹理
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = fmt == 0 ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        HRESULT hr = ifd::FileDialog::Instance().device->CreateTexture2D(&desc, &subResource, &pTexture);
        if (FAILED(hr)) {
            //  创建纹理失败的处理
            //  不说话，生闷气，传个空指针回去让调用者自己猜
            return nullptr;
        }

        // Create texture view
        ID3D11ShaderResourceView* out_srv = NULL;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        hr = ifd::FileDialog::Instance().device->CreateShaderResourceView(pTexture, &srvDesc, &out_srv);
        pTexture->Release();
        if (FAILED(hr)) {
            //  创建SRV失败的处理
            //  不说话，生闷气，传个空指针回去让调用者自己猜
            return nullptr;
        }

        return (void*)out_srv;
        };
    ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
        ID3D11ShaderResourceView* texV = (ID3D11ShaderResourceView*)tex;
        if (texV != nullptr)texV->Release();
        //GLuint texID = (GLuint)tex;
        //glDeleteTextures(1, &texID);
        };

    refresh();
    getScreenRect();
    //cout << "[" << Screen_Width<<","<< Screen_Height << "]";
    //system("pause");
    MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

    socketManager->setUserManager(userManager);
    socketManager->setMapDisplayer(&MapDisplayer);
    socketManager->setMaps(&Maps);
    socketManager->setIDtoMaps(&IDtoMaps);
    socketManager->setGAMEMAP(&GameMap);

    AudioLoader::get().addButton(L"res\\Tap1.mp3", "TAP");
    AudioLoader::get().addButton(L"res\\Tap5.mp3", "SELECT");
    AudioLoader::get().addBGM(L"res\\baba.mp3", "GAME");
    AudioLoader::get().addBGM(L"res\\map.mp3", "MAP");
    AudioLoader::get().addBGM(L"res\\ruin.mp3", "EDITOR");

    if (userManager->getRawData()["FX Volume"] == Json::nullValue) {
        userManager->getRawData()["FX Volume"] = 130;
    }
    if (userManager->getRawData()["BGM Volume"] == Json::nullValue) {
        userManager->getRawData()["BGM Volume"] = 220;
    }
    AudioLoader::get().SetButtonVolume(userManager->getRawData()["FX Volume"].asInt());
    AudioLoader::get().SetBGMVolume(userManager->getRawData()["BGM Volume"].asInt());
}

void getScreenRect() {
    Screen_Width = MIN(1561, MAX(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)));//  获取显示器的宽
    Screen_Height = MIN(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));//    获取显示器的高     //
    width = Screen_Width * 0.6f;
    height = Screen_Width * 0.4f;
    buttonwidth = width / 4;
    baseoffset = Screen_Width / 1024 * 24.0f + width / 25;
    //cout << 12345;
}

////////////////////////////////////GLOBAL VARIABLE FOR FRAME
static int Tab = 0;
enum TAB
{
    Main,
    GameSelect,
    Game,
    MoreMode,
    RatingMode,
    NetModeSelect,
    ChatRoom,
    PkMode,
    EditorSelect,
    Editor_Random,
    Editor_Custom,
    Editor_Generator,
    About,
    Settings,
};
static int Color_ = 0;
enum Color_
{
    Red,
    Green,
    Blue,
    Orange
};
//  有时间就把这些整理进单独的FRAME
ImVec4* Color;
float kbw = 6.0;

vector<tuple<string, int, int>> ranking;

//function<void()> subwindow = nullptr;//子窗口
//function<void()> SettingWindow;//子子窗口
function<void(bool)> SettingWindow;
struct SubWindow {
    function<void(bool)> window;//  布尔值：就算我绘制了控件，我该响应吗？
    bool isDraw;//  即便我不在栈顶，你也会绘制吗？
    bool isAct;//   即便我不在栈顶，你也会相应我的点击事件吗？
};

vector<SubWindow> subWindow;

vector<int> removeIndex;//色盘相关参数
float col[3] = { 0,0,0 };
int selectIndex = 0;//  这个变量命名似乎在GAMESELECT里也有一个

int h_ = 0, w_ = 0;//谁都可以用的临时变量
ImVec2 cursor;

string savedMapName = "";

ImVec2 last_mouse_hit_pos = ImVec2(0, 0);//鼠标信息
int H = 0, W = 0;

float freq = 0.1;

float swicthFrameL = 0;//切换场景用的左右下标
float swicthFrameR = 0;
int Framedelay = 300;

int passedCnt = 0;//Rate模式的通过数
bool ratingCalced = 0;
//drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
//int fixIndex = 0;// DEBUG
////////////////////////////FRAME
void MAIN() {
    //cout << width << "]";
    //system("pause");

    W = width * 3 / 5; H = W * 465 / 1213;
    ImGui::SetCursorPos({ width * 0.5f - W / 2,height * 0.25f - H / 2 });
    if (clock() % int(3 * CLOCKS_PER_SEC * freq) < 1000 * freq) {
        ImGui::Image(imageLoader->IMG["title0"].image, { W * 1.0f,H * 1.0f });
    }
    else if (clock() % int(3 * CLOCKS_PER_SEC * freq) > 2000 * freq) {
        ImGui::Image(imageLoader->IMG["title2"].image, { W * 1.0f,H * 1.0f });
    }
    else {
        ImGui::Image(imageLoader->IMG["title1"].image, { W * 1.0f,H * 1.0f });
    }

    ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * 0.6f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"开始游戏", { buttonwidth,buttonwidth / kbw }) && !ButtonLock && subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ranking.clear();
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::GameSelect;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * (0.6f + 0.07f) });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"关卡编辑器", { buttonwidth,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            AudioLoader::get().playBGM("EDITOR");
            Tab = TAB::EditorSelect;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * (0.6f + 0.14f) });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"关于", { buttonwidth,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::About;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
            });
        
        //cout << "还没写（关于）\n";
    }
    ImGui::PopStyleColor();


    ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * 0.6f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"其他模式", { buttonwidth,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        if (userManager->getRawData()["User"][userManager->getName()]["Rating"] == Json::nullValue) {
            userManager->getRawData()["User"][userManager->getName()]["Rating"] = 0.0;
        }

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::MoreMode;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
        
        //cout << "还没写（其他模式）\n";
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * (0.6f + 0.07f) });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"设置", { buttonwidth,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Settings;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * (0.6f + 0.14f) });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"退出", { buttonwidth,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        EXIT();
    }
    ImGui::PopStyleColor();

    drawText(width / 2, height - width / 120 - Screen_Width / 1024 * 12.0f, Screen_Width / 1024 * 12.0f, ImVec4(0.6, 0.6, 0.6, 1), u8"Version 1.0.0  By XIA");
    //cout << height << "]";
    //system("pause");
}

void GAMESELECT() {
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    static float scrollY = 0;//0 ~ BH * Maps.size() + baseoffset - height// + BH
    static float dy = 0;
    static int selectIndex = 0;
    int OFFSETX = width * 0.15;
    float BH = width * 0.1f;

    //fixIndex = selectIndex;

    if (ImGui::GetMousePos().x - ImGui::GetWindowPos().x > width * 2 / 3 && ImGui::GetMousePos().x - ImGui::GetWindowPos().x < width) {
        if (io.MouseWheel != 0) {
            animeManager.unBind(&scrollY);
        }
        scrollY -= height / 10 * io.MouseWheel;
        scrollY = MIN(scrollY, BH * Maps.size() - BH);
        scrollY = MAX(scrollY, 0);
    }

    //  滚动条绘制
    {
        if (Maps.size() > 1) {
            ImGui::SetCursorPos({
                width - 5.0f - width / 120,
                scrollY / (BH * Maps.size() - BH) * (height - height / Maps.size())
                });
            PaintBoard(u8"滚动条", ImVec2(width / 120, height / Maps.size()));
            if (ImGui::IsItemActivated()) {
                animeManager.unBind(&scrollY);
                dy = ImGui::GetMousePos().y - ImGui::GetWindowPos().y - (scrollY / (BH * Maps.size() - BH) * (height - height / Maps.size()));
            }
            if (ImGui::IsItemActive()) {
                animeManager.unBind(&scrollY);
                ImVec2 pos = GetMousePos_InWindow();
                pos.y = constrict(pos.y - dy, 0, height - height / Maps.size());
                scrollY = pos.y * (BH * Maps.size() - BH) / (height - height / Maps.size());// ImGui::GetMouseDragDelta().y * (BH * Maps.size() - BH) / (height - height / Maps.size());
            }
        }

    }
    //  绘制中间部分
    if(!Maps.empty()) {
        static bool editingName = 0;
        static char buf[15];
        //cout << "第" << selectIndex << "个地图\n";
        MapDisplayer.setMap(Maps[selectIndex]);
        MapDisplayer.setCenter(width / 3 + OFFSETX, height / 2);
        MapDisplayer.setSize(width / 3);
        MapDisplayer.displayFinish();
        if (editingName) {
            ImGui::SetKeyboardFocusHere();
            ImGui::SetCursorPos({ width / 3 + OFFSETX - width / 3 / 2, height / 2 + width / 3 / 2 + Screen_Width / 1024 * 24.0f + width / 120- buttonwidth / kbw/2 });
            //ImGui::SetNextItemWidth(width / 3);
            //ImGui::InputTextWithHint(u8"*",u8"1到15个字符", buf, sizeof buf);
            ImGui::InputTextEx(u8"*", u8"1到15个字符", buf, sizeof buf, ImVec2(width / 3, buttonwidth/kbw), 0);

            if (ImGui::IsItemDeactivated()) {
                editingName = 0;
                if(string(buf) != "" && string(buf) != Maps[IDtoMaps[Maps[selectIndex]->getID()]]->getName()){
                    bool canRename = checkName(buf);//  
                    for (int index = 0; index < Maps.size(); index++) {
                        if (index == selectIndex)continue;
                        if (Maps[index]->getName() == string(buf)) {
                            canRename = 0;
                            break;
                        }
                    }
                    if (canRename == 0) {
                        function<void(bool)> window = [&](bool alive)->void {
                            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                            static char levelname[16];
                            float w, h, x, y;//长宽，中心坐标
                            w = width / 2.6;
                            h = height / 3.8 * 0.5;//
                            x = width / 2;
                            y = height / 2;
                            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                            ImGui::BeginChild(
                                u8"error",
                                ImVec2(w, h),
                                1,
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings// |
                                //ImGuiWindowFlags_NoMove
                            );
                            {
                                drawText(
                                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                                    Screen_Width / 1024 * 24.0f / 2,
                                    ImVec4(255, 255, 255, 255),
                                    u8"重命名失败，存在相同名字的地图"
                                );
                                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                                {
                                    AudioLoader::get().playButton("TAP");
                                    subWindow.pop_back();
                                }
                            }

                            ImGui::EndChild();
                            };
                        subWindow.push_back({ window,0,0 });
                    }
                    else {
                        Maps[IDtoMaps[Maps[selectIndex]->getID()]]->getName() = string(buf);
                        customManager->SaveMap("Map\\" + IDtoPath[Maps[selectIndex]->getID()], (CMap*)Maps[IDtoMaps[Maps[selectIndex]->getID()]]);
                    }
                }
            }
        }
        else {
            drawText(
                width / 3 + OFFSETX, height / 2 + width / 3 / 2 + Screen_Width / 1024 * 24.0f + width / 120,
                Screen_Width / 1024 * 24.0f,
                ImVec4(255, 255, 255, 255),
                Maps[selectIndex]->getName()
            );        
        }

        if (userManager->getRawData()["User"][userManager->getName()]["mapState"][to_string(Maps[selectIndex]->getID())].size() == 0) {
            userManager->getRawData()["User"][userManager->getName()]["mapState"].removeMember(to_string(Maps[selectIndex]->getID()));
        }
        else {
            ImGui::SetCursorPos({
                width / 3 + OFFSETX - width / 3 / 2 / 2, height - buttonwidth / kbw - width / 120
            });
            if (ImGui::Button(u8"设置为头像", ImVec2(width / 3 / 2, buttonwidth / kbw))) {
                AudioLoader::get().playButton("TAP");
                userManager->getRawData()["User"][userManager->getName()]["avator"] = Maps[selectIndex]->getID();
            }
        }

        ImGui::SetCursorPos({
            width / 3 + OFFSETX - width / 120 - width / 3 / 2, height - buttonwidth / kbw * 2  - width / 120
        });
        if (ImGui::Button(u8"重命名", ImVec2(width / 3 / 2, buttonwidth / kbw))) {
            AudioLoader::get().playButton("TAP");
            editingName = 1;
            strcpy_s(buf, (Maps[IDtoMaps[Maps[selectIndex]->getID()]]->getName()).c_str());
            //userManager->getRawData()["User"][userManager->getName()]["avator"] = Maps[selectIndex]->getID();
        }
        ImGui::SetCursorPos({
            width / 3 + OFFSETX + width / 120, height - buttonwidth / kbw * 2 - width / 120
        });
        if (ImGui::Button(u8"删除", ImVec2(width / 3 / 2, buttonwidth / kbw))) {
            AudioLoader::get().playButton("TAP");
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"你确定要删除该地图吗？"
                    );
                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - width / 120,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        subWindow.pop_back();
                    }
                    ImGui::SetCursorPos({ w / 2 + width / 120,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        int ID = Maps[selectIndex]->getID();
                        delete Maps[selectIndex];
                        Maps.erase(Maps.begin() + selectIndex);
                        IDtoMaps.erase(ID);
                        filesystem::remove("Map\\" + IDtoPath[ID]);
                        IDtoPath.erase(ID);
                        selectIndex--;

                        userManager->updateMapInfo(IDtoPath);
                        subWindow.pop_back();

                        //animeManager.addAnime(&scrollY, scrollY, scrollY + (BH * selectIndex - scrollY), 800, OutExpo);
                    }
                }

                ImGui::EndChild();
                };
            subWindow.push_back({ window,0,0 });
            //userManager->getRawData()["User"][userManager->getName()]["avator"] = Maps[selectIndex]->getID();
        }
        //  重命名按钮和删除按钮
        //if (userManager->getName() != "未登录") {
        //    for (const string& ID : userManager->getRawData()["User"][userManager->getName()]["mapState"].getMemberNames()) {
        //        if (ID == to_string(Maps[selectIndex]->getID())) {
        //            ImGui::TextColored(ImVec4(0, 170, 0, 255), "Accepted");
        //            break;
        //        }
        //    }
        //}
    }
    else {

    }
    ////////////////////////////////顶部
    ImVec2 rect = getTextSize(Screen_Width / 1024 * 18.0f, encode::string_To_UTF8(userManager->getName()).c_str());
    w_ = width / 3 + width * 0.06f - width / 3 / 2 - width / 120;
    drawRect(
        width / 2 - rect.x / 2 - width / 120,
        width / 120 + 1.0f * w_ / 3 - rect.y - width / 120,
        rect.x + width / 120 * 2,
        rect.y + width / 120,
        ImVec4(0.9, 0.9, 0.9, 1.0)
    );

    if (userManager->getName() != "未登录" && userManager->getRawData()["User"][userManager->getName()]["Rating"] == Json::nullValue) {
        userManager->getRawData()["User"][userManager->getName()]["Rating"] = 0.0;
    }

    if (userManager->getName() != "未登录" && userManager->getRatingColor(userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat()).w > 0) {
        drawText(
            width / 2, width / 120 + 1.0f * w_ / 3 - width / 120 - rect.y / 2,
            Screen_Width / 1024 * 18.0f,
            userManager->getRatingColor(userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat()),
            encode::string_To_UTF8(userManager->getName()).c_str()
        );
    }
    else {
        if (userManager->getName() != "未登录") {
            drawText_RAINBOW(
                width / 2, width / 120 + 1.0f * w_ / 3 - width / 120 - rect.y / 2,
                Screen_Width / 1024 * 18.0f,
                encode::string_To_UTF8(userManager->getName()).c_str()
            );
        }
        else {
            drawText(
                width / 2, width / 120 + 1.0f * w_ / 3 - width / 120 - rect.y / 2,
                Screen_Width / 1024 * 18.0f,
                userManager->getRatingColor(0),
                encode::string_To_UTF8(userManager->getName()).c_str()
            );
        }
    }

    if (userManager->getName() != "未登录" &&
        (userManager->getRawData()["User"][userManager->getName()]["avator"] == Json::nullValue ||
            !IDtoMaps.count(userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()))
        ) {
        userManager->getRawData()["User"][userManager->getName()]["avator"] = -1;
    }
    MapDisplayer.setMap((userManager->getName() == "未登录" || userManager->getRawData()["User"][userManager->getName()]["avator"] == -1) ?
        defaultAvator :
        Maps[IDtoMaps[userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()]]
    );//refreshMapIndex()

    MapDisplayer.setCenter(width / 2 + rect.x / 2 + width / 120 + 1.0f * w_ / 3 / 2, width / 120 + 1.0f * w_ / 3 / 2);
    MapDisplayer.setSize(1.0f * w_ / 3);
    MapDisplayer.displayFinish();

    stringstream ss;
    ss << fixed << setprecision(2) << showpoint << userManager->getRating();
    ImVec2 Rrect = getTextSize(Screen_Width / 1024 * 12.0f, ss.str());
    drawRect(
        width / 2 + rect.x / 2 + width / 120 + 1.0f * w_ / 3 - (Rrect.x + width / 120 * 2) / 2,
        width / 120 + 1.0f * w_ / 3 - Rrect.y / 2, 
        Rrect.x + width / 120 * 2,
        Rrect.y,
        ImVec4(1, 1, 1, 1.0)
    );

    drawText(
        width / 2 + rect.x / 2 + width / 120 + 1.0f * w_ / 3,
        width / 120 + 1.0f * w_ / 3,
        Screen_Width / 1024 * 12.0f,
        ImVec4(0, 0, 0, 1.0),
        ss.str()
    );

    //  绘制左侧排行榜
    static vector<float> RankingPaddingLeft;
    {
        static int mode = 1;//1为按罚时排序，2为按用时排序

        w_ = width / 3 + width * 0.08f - width / 3 / 2 - width / 120;//width/120常用的小量
        h_ = height / 2;
        drawText(
            width / 120, h_ - Screen_Width / 1024 * 12.0f,
            Screen_Width / 1024 * 12.0f,
            ImVec4(255, 255, 255, 255),
            u8"排行榜",
            0
        );
        //ImGui::SetCursorPos({ w_ - Screen_Width / 1024 * 12.0f - width / 120, width / 120 });
        ImGui::SameLine();
        if (ImageButton(
            (mode == 1) ? u8"按罚时排序" : u8"按用时排序",
            ImVec2(Screen_Width / 1024 * 12.0f, Screen_Width / 1024 * 12.0f),
            imageLoader->IMG[(mode == 1) ? "penelty" : "time"].image,
            true,
            ImVec2(Screen_Width / 1024 * 12.0f * 0.9, Screen_Width / 1024 * 12.0f * 0.9)
        )) {
            AudioLoader::get().playButton("TAP");
            mode = 3 - mode;
            ranking.clear();
        }

        if (Maps.size() > 0 && ranking.empty()) {
            //cout << "初始化";
            RankingPaddingLeft.clear();
            RankingPaddingLeft.push_back(0);
            ranking.push_back({ "",-1,-1 });//    保证ranking非空
            for (string name : userManager->getRawData()["User"].getMemberNames()) {
                int penelty = 1e9;
                int time = 1e9;
                if (userManager->getRawData()["User"][name]["mapState"][to_string(Maps[selectIndex]->getID())].size() == 0) {
                    userManager->getRawData()["User"][name]["mapState"].removeMember(to_string(Maps[selectIndex]->getID()));
                }
                else {
                    for (Json::Value& state : userManager->getRawData()["User"][name]["mapState"][to_string(Maps[selectIndex]->getID())]) {
                        penelty = MIN(penelty, state["penelty"].asInt());
                        time = MIN(time, state["time"].asInt());
                    }
                }
                if (time != 1e9) {
                    ranking.push_back({ name,penelty,time });
                    RankingPaddingLeft.push_back(0);
                    //不能在这里绑定动画，因为vector容器会扩容
                    //animeManager.addAnime(&RankingPaddingLeft[RankingPaddingLeft.size() - 1], 0, 1, 500, OutBack, (RankingPaddingLeft.size() - 2) * 70);
                }
            }
            sort(ranking.begin(), ranking.end(), [&](tuple<string, int, int> a, tuple<string, int, int> b) {
                if ((mode == 1) ? (get<1>(a) == get<1>(b)) : (get<2>(a) == get<2>(b))) {
                    if ((mode == 1) ? (get<2>(a) == get<2>(b)) : (get<1>(a) == get<1>(b))) {
                        return get<0>(a) < get<0>(b);// 比字符串，总不可能名字还相等了
                    }
                    return (mode == 1) ? (get<2>(a) < get<2>(b)) : (get<1>(a) < get<1>(b));
                }
                return (mode == 1) ? (get<1>(a) < get<1>(b)) : (get<2>(a) < get<2>(b));
            });
            for (int i = 1; i < ranking.size(); i++) {
                animeManager.addAnime(&RankingPaddingLeft[i], 0, 1, 500, OutBack, (i - 1) * 70);
            }
        }

        if (Maps.empty()) {
            if (ranking.size() != 1) {
                ranking.clear();
                ranking.push_back({ "",-1,-1 });
            }
        }

        ImGui::SetCursorPos({ 0.0f, 1.0f * h_ });
        ImGui::BeginChild(
            u8"排行榜 ",
            { w_ * 1.0f,h_ * 1.0f },
            1,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings
        );

        if (ranking.size() == 1) {
            drawText(
                w_ / 2, h_ / 2,
                Screen_Width / 1024 * 12.0f,
                ImVec4(155, 155, 155, 255),
                u8"暂无任何记录"
            );
        }
        else {
            for (int i = 1; i < ranking.size(); i++) {
                float offsetx = RankingPaddingLeft[i] * w_ - w_;
                //cout << offsetx << ":" << i << "\n";
                if (userManager->getRawData()["User"][get<0>(ranking[i])]["avator"] == Json::nullValue) {
                    userManager->getRawData()["User"][get<0>(ranking[i])]["avator"] = -1;
                }
                MapDisplayer.setMap((userManager->getRawData()["User"][get<0>(ranking[i])]["avator"] == -1) ?
                    defaultAvator :
                    Maps[IDtoMaps[userManager->getRawData()["User"][get<0>(ranking[i])]["avator"].asInt()]]
                );//refreshMapIndex()

                MapDisplayer.setCenter(width / 120 + 1.0f * w_ / 3 / 2 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + 1.0f * w_ / 3 / 2 + width / 120);
                MapDisplayer.setSize(w_ / 3);
                MapDisplayer.displayFinish();

                drawRect(
                    width / 120 + w_ / 3 - w_ / 3 / 3 / 2 + offsetx,
                    (w_ / 3 + width / 120) * (i - 1) + width / 120 - w_ / 3 / 3 / 2 + width / 120,
                    w_ / 3 / 3,
                    w_ / 3 / 3,
                    (i == 1) ? ImVec4(1.0f, 0.58823f, 0.03921f, 1.0f) : (i == 2) ? ImVec4(0.54117f, 0.54117f, 0.54117f, 1.0f) : (i == 3) ? ImVec4(0.83137f, 0.32549f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                );
                
                drawText(
                    width / 120 + w_ / 3 + offsetx, (w_ / 3 + width / 120)* (i - 1) + width / 120 + width / 120,
                    Screen_Width / 1024 * 12.0f,
                    ImVec4(0, 0, 0, 255),
                    to_string(i)
                );

                if (userManager->getRawData()["User"][get<0>(ranking[i])]["Rating"] == Json::nullValue) {
                    userManager->getRawData()["User"][get<0>(ranking[i])]["Rating"] = 0.0;
                }

                if (userManager->getRatingColor(userManager->getRawData()["User"][get<0>(ranking[i])]["Rating"].asFloat()).w > 0) {
                    drawText(
                        width / 120 + w_ / 3 + width / 120 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120,
                        Screen_Width / 1024 * 12.0f,
                        userManager->getRatingColor(userManager->getRawData()["User"][get<0>(ranking[i])]["Rating"].asFloat()),
                        encode::string_To_UTF8(get<0>(ranking[i])).c_str(),
                        0
                    );
                    if(ImGui::IsItemHovered()){
                        ImGui::SetTooltip(userManager->getRatingName(userManager->getRawData()["User"][get<0>(ranking[i])]["Rating"].asFloat()).c_str());
                    }
                }
                else {
                    drawText_RAINBOW(
                        width / 120 + w_ / 3 + width / 120 + offsetx, (w_ / 3 + width / 120)* (i - 1) + width / 120 + width / 120,
                        Screen_Width / 1024 * 12.0f,
                        encode::string_To_UTF8(get<0>(ranking[i])).c_str(),
                        0,
                        userManager->getRatingName(userManager->getRawData()["User"][get<0>(ranking[i])]["Rating"].asFloat())
                    );
                }

                drawText(
                    width / 120 + w_ / 3 + width / 120 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + Screen_Width / 1024 * 12.0f + width / 120 + width / 120,
                    Screen_Width / 1024 * 16.0f,
                    ImVec4(155/255.0, 155 / 255.0, 155 / 255.0, 255),
                    (mode == 1) ? to_string(get<1>(ranking[i])) : sec2time(get<2>(ranking[i])),
                    0
                );
            }
            drawText(
                -Screen_Width / 1024 * 12.0f, (w_ / 3 + width / 120) * (ranking.size() - 1) + width / 120 + width / 120,
                Screen_Width / 1024 * 12.0f,
                ImVec4(0, 0, 0, 255),
                u8"."
            );
        }
        if (subWindow.size() || ButtonLock)drawRect(0, 0, w_, h_, ImVec4(0, 0, 0, 50));

        //界面切换用的FRAMEMASK
        drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
        ImGui::EndChild();
    }

    //scrollY = ImGui::GetScrollY();

    //drawText(
    //    width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
    //    Screen_Width / 1024 * 24.0f,
    //    ImVec4(255, 255, 255, 255),
    //    to_string(scrollY)+" "+ to_string(BH * Maps.size() + baseoffset)//u8"选择关卡"
    //);

    //drawText(
    //    width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50+ Screen_Width / 1024 * 24.0f,
    //    Screen_Width / 1024 * 24.0f,
    //    ImVec4(255, 255, 255, 255),
    //    u8"选择关卡"//to_string(width * 2 / 3)+" "+ to_string(width)//
    //);
    //drawRect(0.0f, 0.0f, w_, BH * Maps.size(), ImVec4(0, 0, 0, 0));

    //ImGui::Text(u8"选择关卡");
    //ImGui::PopFont();

    //右侧的关卡选择
    if (1) {//    无意义if，折叠代码块用
        MapDisplayer.setSize(BH * 0.8f);
        string space = " ";
        int d = 0;
        float k = 0.4;
        for (auto level : Maps) {
            float offsetX;
            if (BH * d < scrollY)offsetX = (scrollY - BH * d) * k;
            else offsetX = (BH * d - scrollY) * k;
            ImGui::SetCursorPos({ width * (0.9f - 0.35f) + offsetX + OFFSETX,baseoffset + BH * d - scrollY + BH });/////////////////50.0f->width/20
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            bool leave = 0;
            if (ImGui::Button(space.c_str(), { width * 0.7f, BH }) && !ButtonLock && subWindow.size() == 0)
            {
                if (selectIndex != d) {
                    selectIndex = d;
                    animeManager.addAnime(&scrollY, scrollY, scrollY + (BH * d - scrollY), 800, OutExpo);
                    AudioLoader::get().playButton("SELECT");
                    ranking.clear();
                }
                else {
                    AudioLoader::get().playButton("TAP");
                    //initFont(height * 0.7f / max(GameMap->h, GameMap->w));
                    SelectMap = level;
                    //GameMap = new Map(level);

                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;

                        MapDisplayer.setMap(new Map(SelectMap));
                        leave = 1;
                        playerOperation = "";
                        AudioLoader::get().playBGM("GAME");
                        Tab = TAB::Game;
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                            scoreboard->Reset();
                            });
                        });
                    //Tab = TAB::Panel;
                    //cout << level->getName() << d << ImGui::GetWindowPos().x << " " << ImGui::GetWindowPos().y;
                }
            }
            space += " ";
            ImGui::PopStyleColor();
            if (leave) break;
            ImGui::SetCursorPos({
                width * (0.9f - 0.35f) + BH + offsetX + OFFSETX,
                baseoffset + BH / 12 + BH * d - scrollY + BH/////////////////
                });

            ImGui::PushFont(Font);
            ImGui::SetWindowFontScale(0.75);
            ImGui::Text(level->getName().c_str());
            if (userManager->getName() != "未登录") {
                if (userManager->getRawData()["User"][userManager->getName()]["mapState"][to_string(level->getID())].size() == 0) {
                    userManager->getRawData()["User"][userManager->getName()]["mapState"].removeMember(to_string(level->getID()));
                }
                else {
                    ImGui::SetCursorPos({
                        width * (0.9f - 0.35f) + BH + offsetX + OFFSETX,
                        ImGui::GetCursorPosY()/////////////////
                        });
                    ImGui::TextColored(ImVec4(0, 170, 0, 255), "Accepted");
                }
            }
            ImGui::SetWindowFontScale(1.0);
            ImGui::PopFont();

            MapDisplayer.setCenter(width * (0.9f - 0.35f) + BH / 2 + offsetX + OFFSETX, baseoffset + BH * d + BH / 2 - scrollY + BH);
            MapDisplayer.setMap(level);
            MapDisplayer.displayFinish();
            d++;
        }
        drawRect(
            width - 5.0f - width / 120,
            scrollY / (BH * Maps.size() - BH) * (height - height / Maps.size()),
            width / 120,
            height / Maps.size(),
            ImVec4(0.9, 0.9, 0.9, 1)
        );
        if (Maps.empty()) {
            drawText(
                width * (0.9f - 0.35f) + BH + OFFSETX, height / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                u8"无可用地图"
            );
        }
    }

    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Main;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();
}

void GAME() {
    MapDisplayer.setSize(height * 0.7f);
    MapDisplayer.setCenter(width * 0.5f, height * 0.5f);
    MapDisplayer.display();

    MapDisplayer.setSize(height * 0.3f);
    MapDisplayer.setCenter(width * 0.133f, height * 0.5f);
    MapDisplayer.displayFinish();
    //======================================================//
    //void drawText(float x, float y, float size, ImVec4 color, string text)
    drawText(
        width * 0.86f,
        height * 0.2f,
        buttonwidth / kbw * 0.6,
        ImVec4(255, 255, 255, 255),
        "Penelty"
    );
    drawText(
        width * 0.86f,
        height * 0.2f + buttonwidth / kbw * 0.8,
        buttonwidth / kbw,
        ImVec4(255, 255, 255, 255),
        to_string(scoreboard->calcScore())
    );
    drawText(
        width * 0.5f,
        buttonwidth / kbw * 0.7,
        buttonwidth / kbw,
        ImVec4(255, 255, 255, 255),
        sec2time(scoreboard->GetTime())
    );
    //======================================================//
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"逆时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        LastMap.push(new Map(MapDisplayer.myMap));
        MapDisplayer.myMap->anticlockwise();
        scoreboard->step();
    }
    ImGui::PopStyleColor();
    if (hintstate == ANTICLOCK)drawRect(
        width * 0.86f - buttonwidth / 2,
        height * 0.4f,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f , height * 0.4f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"顺时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        LastMap.push(new Map(MapDisplayer.myMap));
        MapDisplayer.myMap->clockwise();
        scoreboard->step();
    }
    ImGui::PopStyleColor();
    if (hintstate == CLOCK)drawRect(
        width * 0.86f,
        height * 0.4f,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 4, height * 0.4f + 1.5f * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"燃烧", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        Map* tmp = new Map(MapDisplayer.myMap);
        if (MapDisplayer.myMap->Burn())LastMap.push(tmp);
        scoreboard->step();
    }
    ImGui::PopStyleColor();
    if (hintstate == BURN)drawRect(
        width * 0.86f - buttonwidth / 4,
        height * 0.4f + 1.5f * buttonwidth / kbw,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 3 * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"撤回", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        if (!LastMap.empty()) {
            Map* tmp = MapDisplayer.myMap;
            MapDisplayer.myMap = LastMap.top();
            LastMap.pop();
            delete tmp;
            scoreboard->regret();
        }
    }
    ImGui::PopStyleColor();
    if (hintstate == RETREAT)drawRect(
        width * 0.86f - buttonwidth / 2,
        height * 0.4f + 3 * buttonwidth / kbw,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f , height * 0.4f + 3 * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"提示", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = MapDisplayer.myMap->Hint();
        scoreboard->hint();
    }
    ImGui::PopStyleColor();
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"提交答案", { buttonwidth, buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        //LastMap.push(new Map(MapDisplayer.myMap));
        if (!MapDisplayer.myMap->Submit()) {
            scoreboard->submit();
        }
        else {
            ButtonLock = 0;
            scoreboard->lock();
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 4;
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"确认窗口",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //cout << "ok";
                //ImGui::SetCursorPos({ width / 2, height / 2 });

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(0, 170, 0, 255),
                    u8"Accepted!"
                );
                ImGui::SetCursorPos({ 0, Screen_Width / 1024 * 24.0f });
                ImGui::Indent(width / 12);
                ImGui::Text(encode::string_To_UTF8((string)"罚时：" + to_string(scoreboard->calcScore())).c_str());
                ImGui::Text(encode::string_To_UTF8((string)"用时：" + sec2time(scoreboard->GetTime())).c_str());
                string tip = u8"";
                if (userManager->getName() == userManager->nulluser)tip = u8"你当前未登录账号，无法保存通关记录";

                drawText(
                    w / 2, h - buttonwidth / kbw * 1.4f - Screen_Width / 1024 * 24.0f / 2 / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 0, 0, 255),
                    tip
                );
                //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 4, h - buttonwidth / kbw * 1.3f });
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive &&!ButtonLock)
                {
                    AudioLoader::get().playButton("TAP");
                    AudioLoader::get().playBGM("MAP");
                    if (userManager->getName()!=userManager->nulluser) {
                        userManager->getRawData()["User"][userManager->getName()]["COUNT"] = userManager->getRawData()["User"][userManager->getName()]["COUNT"].asInt() + 1;
                    }
                    //ButtonLock = 0;
                    hintstate = NONE;
                    MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;
                        subWindow.pop_back();
                        Tab = TAB::GameSelect;
                        delete MapDisplayer.myMap;//    清理缓存
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine,0, [&]() {
                            scoreboard->unlock();
                        });
                    });
                    
                    ////  SAVE YOUR MAP!  一个致命的错误，导致我的几个地图文件坏掉了
                    //int w, h; LastMap.top()->getSize(w, h);
                    //for (int i = 1; i <= h; i++)
                    //    for (int j = 1; j <= w; j++) {
                    //        Maps[fixIndex]->goalColors[i][j] = MapDisplayer.myMap->colors[i][j];
                    //    }
                    //customManager->SaveMap("Map\\"+IDtoPath[Maps[fixIndex]->ID], (CMap*)Maps[fixIndex]);
                    ////  

                    while (LastMap.size()) {
                        Map* tmp = LastMap.top();
                        LastMap.pop();
                        delete tmp;
                    }
                    //    传进来的时候new了一个

                    if (userManager->getName() != userManager->nulluser) {
                        userManager->saveHistory(
                            MapDisplayer.myMap->getID(),
                            scoreboard->GetTime(),
                            scoreboard->calcScore()
                        );
                    }

                    ranking.clear();//  排行榜重置
                    //cout << "ok";
                }

                ImGui::PopStyleColor();
                ImGui::EndChild();

                };
            subWindow.push_back({ window,0,0 });
        }
    }
    ImGui::PopStyleColor();
    if (hintstate == SUBMIT)drawRect(
        width * 0.86f - buttonwidth / 2,
        height * 0.4f + 4.5f * buttonwidth / kbw,
        buttonwidth,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width - buttonwidth * 2 / kbw - 5.0f,5.0f + ImGui::GetScrollY() });
    //ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"重试", { buttonwidth * 2 / kbw, buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        //cout << SelectMap->ans<<"]";
        delete MapDisplayer.myMap;
        MapDisplayer.setMap(new Map(SelectMap));
        while (LastMap.size()) {
            Map* tmp = LastMap.top();
            LastMap.pop();
            delete tmp;
        }
        scoreboard->Reset();
    }
    ImGui::PopStyleColor();
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::GameSelect;
            AudioLoader::get().playBGM("MAP");
            delete MapDisplayer.myMap;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });

        
        while (LastMap.size()) {
            Map* tmp = LastMap.top();
            LastMap.pop();
            delete tmp;
        }

        ranking.clear();
        //cout << "喵7";
    }
    ImGui::PopStyleColor();
}

void MOREMODE() {
    drawText(
        width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
        Screen_Width / 1024 * 24.0f,
        ImVec4(255, 255, 255, 255),
        u8"选择模式"
    );

    ImGui::SetCursorPos({ width / 2 - height / 3 - width / 120, height / 2 - height / 3 / 2 });
    if (ImageButton(u8"Rating模式", {1.0f* height /3,1.0f * height /3},imageLoader->IMG["rated"].image)) {
    //if (ImGui::Button(u8"Rating模式")) {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            customManager->RateRandom(0);//自带reset
            MapDisplayer.setMap(new Map(customManager->getBegin()));
            passedCnt = 0;

            scoreboard->Reset();
            scoreboard->lock();
            AudioLoader::get().playBGM("GAME");
            Tab = TAB::RatingMode;
            ratingCalced = 0;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                scoreboard->unlock();
            });
        });
    }
    ImGui::SetCursorPos({ width / 2 + width / 120, height / 2 - height / 3 / 2 });
    if (ImageButton(u8"联机模式", { 1.0f * height / 3,1.0f * height / 3 }, imageLoader->IMG["net"].image)) {
    //if (ImGui::Button(u8"联机模式")) {
        AudioLoader::get().playButton("TAP");
        if (userManager->getName() == "未登录") {
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"请登录"
                    );
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"当前未登录账号，无法进行联机"
                    );
                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                    {
                        AudioLoader::get().playButton("TAP");
                        subWindow.pop_back();
                    }
                }
                ImGui::EndChild();
                };
            subWindow.push_back({ window, 0, 0 });
        }
        else {
            ButtonLock = 1;
            swicthFrameR = swicthFrameL = 0;
            animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                ButtonLock = 0;

                Tab = TAB::NetModeSelect;
                animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                });
        }
    }

    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Main;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
        
    }
    ImGui::PopStyleColor();
}

void RATINGMODE() {
    //drawText(
    //    width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
    //    Screen_Width / 1024 * 24.0f,
    //    ImVec4(255, 255, 255, 255),
    //    u8"选择模式"
    //);
    static float RatingChange = 0;//  RatingChange

    MapDisplayer.setSize(height * 0.7f);
    MapDisplayer.setCenter(width * 0.5f, height * 0.5f);
    MapDisplayer.display();

    MapDisplayer.setSize(height * 0.3f);
    MapDisplayer.setCenter(width * 0.133f, height * 0.5f);
    MapDisplayer.displayFinish();
    //======================================================//
    //void drawText(float x, float y, float size, ImVec4 color, string text)
    drawText(
        width * 0.86f,
        height * 0.2f,
        buttonwidth / kbw * 0.6,
        ImVec4(255, 255, 255, 255),
        "Score"
    );
    drawText(
        width * 0.86f,
        height * 0.2f + buttonwidth / kbw * 0.8,
        buttonwidth / kbw,
        ImVec4(255, 255, 255, 255),
        to_string(passedCnt)
    );
    drawText(
        width * 0.5f,
        buttonwidth / kbw * 0.7,
        buttonwidth / kbw,
        ImVec4(255, 255, 255, 255),
        sec2time(300 - scoreboard->GetTime())
    );
    //======================================================//
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"逆时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        LastMap.push(new Map(MapDisplayer.myMap));
        MapDisplayer.myMap->anticlockwise();
        //scoreboard->step();
    }
    ImGui::PopStyleColor();
    if (hintstate == ANTICLOCK)drawRect(
        width * 0.86f - buttonwidth / 2,
        height * 0.4f,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f , height * 0.4f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"顺时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        LastMap.push(new Map(MapDisplayer.myMap));
        MapDisplayer.myMap->clockwise();
        // scoreboard->step();
    }
    ImGui::PopStyleColor();
    if (hintstate == CLOCK)drawRect(
        width * 0.86f,
        height * 0.4f,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 4, height * 0.4f + 1.5f * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"燃烧", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        Map* tmp = new Map(MapDisplayer.myMap);
        if (MapDisplayer.myMap->Burn())LastMap.push(tmp);
        //scoreboard->step();
    }
    ImGui::PopStyleColor();
    if (hintstate == BURN)drawRect(
        width * 0.86f - buttonwidth / 4,
        height * 0.4f + 1.5f * buttonwidth / kbw,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2 / 2, height * 0.4f + 3 * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"撤回", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        if (!LastMap.empty()) {
            Map* tmp = MapDisplayer.myMap;
            MapDisplayer.myMap = LastMap.top();
            LastMap.pop();
            delete tmp;
            //scoreboard->regret();
        }
    }
    ImGui::PopStyleColor();
    if (hintstate == RETREAT)drawRect(//    提示被禁用了，但是还是得为调试模式做适配
        width * 0.86f - buttonwidth, // / 2,
        height * 0.4f + 3 * buttonwidth / kbw,
        buttonwidth / 2,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );
    ///////////////////////////////////不准提示，记得注释掉//////////////////////////////////////
    //ImGui::SetCursorPos({ width * 0.86f + buttonwidth / 2 /2 , height * 0.4f + 3 * buttonwidth / kbw });
    //ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    //if (ImGui::Button(u8"提示", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    //{
    //    hintstate = MapDisplayer.myMap->Hint();
    //    scoreboard->hint();
    //}
    //ImGui::PopStyleColor();
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"提交答案", { buttonwidth, buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        //LastMap.push(new Map(MapDisplayer.myMap));
        if (!MapDisplayer.myMap->Submit()) {
            //scoreboard->submit();
            //无罚时
        }
        else {
            passedCnt++;
            if (userManager->getName() != userManager->nulluser) {
                userManager->getRawData()["User"][userManager->getName()]["COUNT"] = userManager->getRawData()["User"][userManager->getName()]["COUNT"].asInt() + 1;
            }
            customManager->RateRandom((passedCnt - 1) / 5);//自带reset
            MapDisplayer.setMap(new Map(customManager->getBegin()));
            while (LastMap.size()) {
                Map* tmp = LastMap.top();
                LastMap.pop();
                delete tmp;
            }
        }
    }
    ImGui::PopStyleColor();
    if (hintstate == SUBMIT)drawRect(
        width * 0.86f - buttonwidth / 2,
        height * 0.4f + 4.5f * buttonwidth / kbw,
        buttonwidth,
        buttonwidth / kbw + 1,
        ImVec4(255, 255, 0, 100)
    );

    if (300 - scoreboard->GetTime() <= 0 && !ratingCalced) {
        subWindow.clear();//    万一玩家按了退出按钮又不确认
        scoreboard->lock();
        ratingCalced = 1;
        ButtonLock = 0;
        if (userManager->getName() != userManager->nulluser) {
            if (userManager->getRawData()["User"][userManager->getName()]["Rating"] == Json::nullValue) {
                userManager->getRawData()["User"][userManager->getName()]["Rating"] = 0.0f;
            }
            RatingChange = (passedCnt - userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat()) / 4.0;
            userManager->getRawData()["User"][userManager->getName()]["Rating"] = userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat() + RatingChange;
        }
        function<void(bool)> window = [&](bool alive)->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.8;
            h = height / 4;
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"确认窗口",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings
            );
            drawText(
                w / 2, Screen_Width / 1024 * 24.0f / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(0, 170, 0, 255),
                u8"Finished!"
            );
            ImGui::SetCursorPos({ 0, Screen_Width / 1024 * 24.0f });
            ImGui::Indent(width / 12);
            ImGui::Text(encode::string_To_UTF8((string)"分数：" + to_string(passedCnt)).c_str());


            string tip = u8"";
            if (userManager->getName() == userManager->nulluser)tip = u8"你当前未登录账号，无法更新Rating信息";
            else {
                ImGui::Text(u8"Rating变化："); ImGui::SameLine();
                stringstream ss;
                ss << setprecision(2) << fixed << showpos << RatingChange;
                ImGui::TextColored(RatingChange < 0 ? ImVec4(1, 0, 0, 1) : RatingChange == 0 ? ImVec4(0.5, 0.5, 0.5, 1) : ImVec4(0, 1, 0, 1), ss.str().c_str());
            }

            drawText(
                w / 2, h - buttonwidth / kbw * 1.4f - Screen_Width / 1024 * 24.0f / 2 / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 0, 0, 255),
                tip
            );
            //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());
            ImGui::SetCursorPos({ w / 2 - buttonwidth / 4, h - buttonwidth / kbw * 1.3f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
            {
                AudioLoader::get().playButton("TAP");
                AudioLoader::get().playBGM("MAP");
                //ButtonLock = 0;
                hintstate = NONE;
                MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

                //  播放切屏动画
                ButtonLock = 1;
                swicthFrameR = swicthFrameL = 0;
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    ButtonLock = 0;
                    subWindow.pop_back();
                    delete MapDisplayer.myMap;
                    Tab = TAB::MoreMode;
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                        scoreboard->unlock();
                    });
                });
                //  清空撤回栈
                while (LastMap.size()) {
                    Map* tmp = LastMap.top();
                    LastMap.pop();
                    delete tmp;
                }
            }

            ImGui::PopStyleColor();
            ImGui::EndChild();
        };
        subWindow.push_back({ window,0,0 });
    }
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ width - buttonwidth * 2 / kbw - 5.0f,5.0f + ImGui::GetScrollY() });
    //ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"重试", { buttonwidth * 2 / kbw, buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        hintstate = NONE;
        //cout << SelectMap->ans<<"]";
        delete MapDisplayer.myMap;
        MapDisplayer.setMap(new Map(customManager->getBegin()));
        while (LastMap.size()) {
            Map* tmp = LastMap.top();
            LastMap.pop();
            delete tmp;
        }
        //scoreboard->Reset();
    }
    ImGui::PopStyleColor();
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        function<void(bool)> window = [&](bool alive)->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.8;
            h = height / 4;
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"确认窗口",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings
            );
            drawText(
                w / 2, Screen_Width / 1024 * 24.0f / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                u8"你确定要退出吗"
            );
            ImGui::SetCursorPos({ 0, Screen_Width / 1024 * 24.0f });

            drawText(
                w / 2, h - buttonwidth / kbw * 1.4f - Screen_Width / 1024 * 24.0f / 2 / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                u8"你的成绩将不被保存"
            );
            //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());
            
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2, h - buttonwidth / kbw * 1.3f });
            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
            {
                AudioLoader::get().playButton("TAP");
                subWindow.pop_back();
            }
            ImGui::SetCursorPos({ w / 2, h - buttonwidth / kbw * 1.3f });
            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
            {
                AudioLoader::get().playButton("TAP");
                //ButtonLock = 0;
                hintstate = NONE;
                MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

                //  播放切屏动画
                ButtonLock = 1;
                swicthFrameR = swicthFrameL = 0;
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    ButtonLock = 0;
                    subWindow.pop_back();
                    delete MapDisplayer.myMap;
                    AudioLoader::get().playBGM("MAP");
                    Tab = TAB::MoreMode;
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                });

                //  清空撤回栈
                while (LastMap.size()) {
                    Map* tmp = LastMap.top();
                    LastMap.pop();
                    delete tmp;
                }

                scoreboard->unlock();
            }

            ImGui::PopStyleColor();
            //界面切换用的FRAMEMASK
            drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255),1);
            ImGui::EndChild();
        };
        subWindow.push_back({ window,0,0 });
    }

    ////////////////////////////////////////////////////////////////////////
    //ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    //ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    //if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    //{
    //    ButtonLock = 1;
    //    swicthFrameR = swicthFrameL = 0;
    //    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
    //        ButtonLock = 0;
    //        Tab = TAB::MoreMode;
    //        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
    //    });
    //}
    //ImGui::PopStyleColor();
}

void NETMODESELECT() {
    static char IP[16] = { '.' };
    static int sign = 0;

    drawText(
        width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
        Screen_Width / 1024 * 24.0f,
        ImVec4(255, 255, 255, 255),
        u8"联机模式"
    );

    ImGui::SetCursorPos({ width / 2 - height / 3 - width / 120, height / 2 - height / 3 / 2 });
    if (ImageButton(u8"创建房间", { 1.0f * height / 3,1.0f * height / 3 }, imageLoader->IMG["server"].image)) {
    //if (ImGui::Button(u8"创建房间")) {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 0;
        function<void(bool)> window = [&](bool alive)->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.8;
            h = height / 4;
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"确认窗口",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings
            );
            drawText(
                w / 2, Screen_Width / 1024 * 24.0f / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                u8"创建房间"
            );
            ImGui::Indent(width / 30);
            ImGui::SetCursorPos({ width / 30, Screen_Width / 1024 * 24.0f });
            ImGui::Text(encode::string_To_UTF8("IP：" + socketManager->GetLocalHost()).c_str());
            ImGui::Text(u8"端口："); ImGui::SameLine(); ImGui::InputInt(u8" ", &socketManager->getPort());

            //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());

            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2, h - buttonwidth / kbw * 1.3f });
            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
            {
                AudioLoader::get().playButton("TAP");
                subWindow.pop_back();
            }
            ImGui::SetCursorPos({ w / 2, h - buttonwidth / kbw * 1.3f });
            if (ImGui::Button(u8"创建", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
            {
                AudioLoader::get().playButton("TAP");
                //  将头像信息传入socketManager
                if (userManager->getRawData()["User"][userManager->getName()]["avator"] == Json::nullValue) {
                    userManager->getRawData()["User"][userManager->getName()]["avator"] = -1;
                }
                if (userManager->getRawData()["User"][userManager->getName()]["avator"] == -1) {
                    socketManager->setTmp(SaveMapAsJson((CMap*)defaultAvator));
                }
                else {
                    socketManager->setTmp(SaveMapAsJson((CMap*)Maps[IDtoMaps[userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()]]));
                }
                socketManager->AvatorInfo[userManager->getID()] = new CMap((CMap*)Maps[IDtoMaps[userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()]]);
                socketManager->Server_Start();
                //socketManager->UserInfo[socketManager->]
                ButtonLock = 1;
                swicthFrameR = swicthFrameL = 0;
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    subWindow.pop_back();
                    Tab = TAB::ChatRoom;
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                        ButtonLock = 0;
                    });
                });
            }
            ImGui::PopStyleColor();
            //界面切换用的FRAMEMASK
            drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
            ImGui::EndChild();
        };
        subWindow.push_back({ window,0,0 });
    }
    
    ImGui::SetCursorPos({ width / 2 + width / 120, height / 2 - height / 3 / 2 });
    if (ImageButton(u8"加入房间", { 1.0f * height / 3,1.0f * height / 3 }, imageLoader->IMG["client"].image)) {
    //if (ImGui::Button(u8"加入房间")) {
        AudioLoader::get().playButton("TAP");
        sign = 0;
        ButtonLock = 0;
        function<void(bool)> window = [&](bool alive)->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.8;
            h = height / 4;
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"确认窗口",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings
            );
            drawText(
                w / 2, Screen_Width / 1024 * 24.0f / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                u8"加入房间"
            );
            ImGui::Indent(width / 30);
            ImGui::SetCursorPos({ width / 30, Screen_Width / 1024 * 24.0f });
            ImGui::Text(u8"IP："); ImGui::SameLine(); ImGui::InputText(u8"  ", IP, IM_ARRAYSIZE(IP));
            ImGui::Text(u8"端口："); ImGui::SameLine(); ImGui::InputInt(u8" ", &socketManager->getPort());

            //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());

            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2, h - buttonwidth / kbw * 1.3f });
            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
            {
                AudioLoader::get().playButton("TAP");
                sign = 2;
                subWindow.pop_back();
            }

            ImGui::SetCursorPos({ w / 2, h - buttonwidth / kbw * 1.3f });

            if (socketManager->getStatu() == UNACTIVE) {
                if (ImGui::Button(u8"加入", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                {
                    AudioLoader::get().playButton("TAP");
                    //  将头像信息传入socketManager
                    if (userManager->getRawData()["User"][userManager->getName()]["avator"] == Json::nullValue) {
                        userManager->getRawData()["User"][userManager->getName()]["avator"] = -1;
                    }
                    if (userManager->getRawData()["User"][userManager->getName()]["avator"] == -1) {
                        socketManager->setTmp(SaveMapAsJson((CMap*)defaultAvator));
                    }
                    else {
                        socketManager->setTmp(SaveMapAsJson((CMap*)Maps[IDtoMaps[userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()]]));
                    }
                    socketManager->AvatorInfo[userManager->getID()] = new CMap((CMap*)Maps[IDtoMaps[userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()]]);

                    sign = 0;
                    socketManager->Client_Start(std::string(IP), socketManager->getPort());
                    std::thread t([&]() {
                        socketManager->Client_ConnetToServer(&sign);
                        });
                    t.detach();
                }
            }
            else if (socketManager->getStatu() == CLIENT_CONNECTING) {
                if (ImGui::Button(u8"停止连接", { buttonwidth / 2,buttonwidth / kbw })) {
                    AudioLoader::get().playButton("TAP");
                    sign = 2;
                }
            }

            if (sign == 1 && !ButtonLock) {
                ButtonLock = 1;
                swicthFrameR = swicthFrameL = 0;
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    subWindow.pop_back();
                    Tab = TAB::ChatRoom;
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                        ButtonLock = 0;
                    });
                });
            }

            ImGui::PopStyleColor();
            //界面切换用的FRAMEMASK
            drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
            ImGui::EndChild();
            };
        subWindow.push_back({ window,0,0 });
    }

    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::MoreMode;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });

    }
    ImGui::PopStyleColor();
}

void CHATROOM() {
    static char message[500];
    static float OFFSETXw = 0;

    ////////////////////////////////////////滚动条信息控制//////////////////////////////////////////
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    static float scrollY = 0;//0 ~ BH * Maps.size() + baseoffset - height// + BH
    static float dy = 0;
    static int selectIndex = 0;
    int OFFSETX = width * 0.15;
    float BH = width * 0.1f;
    //fixIndex = selectIndex;

    if (socketManager->getStatu()==SERVER && !Maps.empty() && socketManager->selectMapID != to_string(Maps[selectIndex]->getID())) {
        socketManager->selectMapID = to_string(Maps[selectIndex]->getID());
    }

    if (ImGui::GetMousePos().x - ImGui::GetWindowPos().x > width * 2 / 3 && ImGui::GetMousePos().x - ImGui::GetWindowPos().x < width) {
        if (io.MouseWheel != 0) {
            animeManager.unBind(&scrollY);
        }
        scrollY -= height / 10 * io.MouseWheel;
        scrollY = MIN(scrollY, BH * Maps.size() - BH);
        scrollY = MAX(scrollY, 0);
    }

    //  滚动条绘制
    {
        if (Maps.size() > 1) {
            ImGui::SetCursorPos({
                width - 5.0f - width / 120,
                scrollY / (BH * Maps.size() - BH) * (height - height / Maps.size())
                });
            PaintBoard(u8"滚动条", ImVec2(width / 120, height / Maps.size()));
            if (ImGui::IsItemActivated()) {
                animeManager.unBind(&scrollY);
                dy = ImGui::GetMousePos().y - ImGui::GetWindowPos().y - (scrollY / (BH * Maps.size() - BH) * (height - height / Maps.size()));
            }
            if (ImGui::IsItemActive()) {
                animeManager.unBind(&scrollY);
                ImVec2 pos = GetMousePos_InWindow();
                pos.y = constrict(pos.y - dy, 0, height - height / Maps.size());
                scrollY = pos.y * (BH * Maps.size() - BH) / (height - height / Maps.size());// ImGui::GetMouseDragDelta().y * (BH * Maps.size() - BH) / (height - height / Maps.size());
            }
        }

    }
    //////////////////////////////////测试//////////////////////////////////////
    drawText(
        width / 2, width / 120 + Screen_Width / 1024 * 12.0f / 2,
        Screen_Width / 1024 * 12.0f,
        ImVec4(0.7, 0.7, 0.7, 1),
        encode::string_To_UTF8("IP：" + socketManager->GetLocalHost()).c_str()
    );
    drawText(
        width / 2, width / 120 + Screen_Width / 1024 * 12.0f / 2 + Screen_Width / 1024 * 12.0f + width / 120,
        Screen_Width / 1024 * 12.0f,
        ImVec4(0.7, 0.7, 0.7, 1),
        encode::string_To_UTF8("端口：" + to_string(socketManager->getPort())).c_str()
    );
    //if (ImGui::Button(u8"ff")) {
    //    socketManager->sendMessage("qwq");
    //}
    //////////////////////////////////聊天窗口/////////////////////////////////
    if (subWindow.empty()) {//   
        w_ = width / 3;//width/120常用的小量
        h_ = height / 2;
        ImGui::SetCursorPos({ 1.0f * width / 120,1.0f * height / 2 - width / 120 });
        ImGui::BeginChild(
            u8"聊天室 ",
            { w_ * 1.0f,h_ * 1.0f },
            1,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings
        );
        {
            ImGui::BeginChild(
                u8"消息列表 ",
                { w_ * 1.0f - width / 120 * 2,h_ * 1.0f - buttonwidth / kbw - width / 120 * 2 },
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings
            );
            {
                //int i = 0;
                ImGui::Indent(width / 120);
                ImGui::SetCursorPos({ width / 120, width / 120 });

                auto ShowMsg = [&](Json::Value &msg) {
                    if (msg["type"] == "MESSAGE") {
                        if (msg["ID"].asString() != userManager->getID()) {
                            ImVec2 rect_ = ImGui::GetCursorPos();
                            MapDisplayer.setMap(socketManager->AvatorInfo[msg["ID"].asString()]);
                            MapDisplayer.setCenter(width / 120 + w_ / 3 / 6, width / 120 + rect_.y + w_ / 3 / 6);
                            MapDisplayer.setSize(1.0f * w_ / 3 / 3);
                            MapDisplayer.displayFinish();

                            //ImGui::SameLine();
                            ImGui::SetCursorPos({ width / 120 * 2 + 1.0f * w_ / 3 / 3 ,rect_.y });
                            rect_ = ImGui::GetCursorPos();
                            drawText(
                                width / 120 + 1.0f * w_ / 3 / 3, rect_.y,
                                Screen_Width / 1024 * 12.0f,
                                (msg["title"] == "server") ? ImVec4(1.0, 0.4, 0.4, 1) : ImVec4(0.7, 0.7, 0.7, 1),
                                encode::string_To_UTF8(msg["from"].asString()),
                                0
                            );
                            ImGui::SetCursorPos({ rect_.x, ImGui::GetCursorPosY() });

                            //ImVec2 rect = getTextSize(Screen_Width / 1024 * 12.0f, encode::string_To_UTF8(msg["from"].asString()) + " :");
                            drawText_autoNewLine(
                                ImGui::GetCursorPosX(), ImGui::GetCursorPosY(),
                                Screen_Width / 1024 * 12.0f,
                                ImVec4(0.7, 0.7, 0.7, 1),
                                encode::string_To_UTF8(msg["body"].asString()),
                                w_ * 1.0f - width / 120 * 4 - rect_.x - OFFSETXw,
                                msg["time"].asString(),
                                0
                            );
                        }
                        else {
                            ImVec2 rect_ = ImGui::GetCursorPos();
                            MapDisplayer.setMap(socketManager->AvatorInfo[msg["ID"].asString()]);
                            MapDisplayer.setCenter(w_ - w_ / 3 / 6 - width / 120 * 3 - OFFSETXw, width / 120 + rect_.y + w_ / 3 / 6);
                            MapDisplayer.setSize(1.0f * w_ / 3 / 3);
                            MapDisplayer.displayFinish();

                            ImVec2 rect = getTextSize(Screen_Width / 1024 * 12.0f, encode::string_To_UTF8(msg["from"].asString()));
                            //ImGui::SameLine();
                            rect_ = ImGui::GetCursorPos();
                            drawText(
                                w_ - 1.0f * w_ / 3 / 3 - 2 * width / 120 * 2 - rect.x - OFFSETXw, rect_.y,// + width / 120,
                                Screen_Width / 1024 * 12.0f,
                                (msg["title"] == "server") ? ImVec4(1.0, 0.4, 0.4, 1) : ImVec4(0.7, 0.7, 0.7, 1),
                                encode::string_To_UTF8(msg["from"].asString()),
                                0
                            );
                            //ImGui::SetCursorPos({ rect_.x, ImGui::GetCursorPosY() });
                            rect_ = getTextSize(Screen_Width / 1024 * 12.0f, encode::string_To_UTF8("中"));

                            rect = getTextSize_autoNewLine(Screen_Width / 1024 * 12.0f, encode::string_To_UTF8(msg["body"].asString()), 1.0f * w_ - 1.0f * w_ / 3 / 3 - 3 * width / 120);
                            bool over = 0;
                            if (rect.x > 1.0f * w_ - 1.0f * w_ / 3 / 3 - 3 * width / 120 - OFFSETXw - (rect_.x) * (OFFSETXw > 0)) {
                                rect.x = 1.0f * w_ - 1.0f * w_ / 3 / 3 - 3 * width / 120 - OFFSETXw - (rect_.x) * (OFFSETXw > 0);
                                over = -1;
                            }
                            drawText_autoNewLine(
                                1.0f * w_ - 1.0f * w_ / 3 / 3 - 3 * width / 120 - rect.x - (rect_.x) * (OFFSETXw > 0) + over * (rect_.x), ImGui::GetCursorPosY(),
                                Screen_Width / 1024 * 12.0f,
                                ImVec4(0.7, 0.7, 0.7, 1),
                                encode::string_To_UTF8(msg["body"].asString()),
                                1.0f * w_ - 1.0f * w_ / 3 / 3 - 3 * width / 120 - over * rect_.x + (!(OFFSETXw > 0) && (over)) * rect_.x,
                                msg["time"].asString(),
                                0
                            );
                        }
                    }
                    else if (msg["type"] == "FINALSTANDING") {
                        {
                            //static int mode = 1;//1为按罚时排序，2为按用时排序
                            float w_ = width / 3 + width * 0.08f - width / 3 / 2 - width / 120;//width/120常用的小量
                            float h_ = height / 2;
                            w_ *= 0.75f;
                            h_ *= 0.75f;
                            drawText(
                                width / 120, ImGui::GetCursorPosY(),//h_ - Screen_Width / 1024 * 12.0f,
                                Screen_Width / 1024 * 12.0f,
                                ImVec4(255, 255, 255, 255),
                                u8"上一局排行榜",
                                0
                            );
                            //ImGui::SetCursorPos({ w_ - Screen_Width / 1024 * 12.0f - width / 120, width / 120 });
                            ImGui::SameLine();
                            if (ImageButton(
                                (msg["mode"].asInt() == 1) ? u8"罚时" : u8"用时",
                                ImVec2(Screen_Width / 1024 * 12.0f, Screen_Width / 1024 * 12.0f),
                                imageLoader->IMG[(msg["mode"].asInt() == 1) ? "penelty" : "time"].image,
                                true,
                                ImVec2(Screen_Width / 1024 * 12.0f * 0.9, Screen_Width / 1024 * 12.0f * 0.9),
                                stoi(msg["TIME"].asString())
                            )) {
                                msg["mode"] = 3 - msg["mode"].asInt();
                            }

                            ranking.clear();
                            ranking.push_back({ "",-1,-1 });//    保证ranking非空
                            for (Json::Value J : msg["Ranking"]) {
                                ranking.push_back({ J["ID"].asString(),J["penelty"].asInt(),J["time"].asInt() });
                            }

                            ImGui::SetCursorPos({ width / 120, ImGui::GetCursorPosY() });
                            ImGui::BeginChild(
                                msg["TIME"].asString().c_str(),//   排行榜
                                { w_ * 1.0f,h_ * 1.0f },
                                1,
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings
                            );
                            {
                                if (ranking.size() == 1) {//    按理来说，这句在这里不会执行
                                    drawText(
                                        w_ / 2, h_ / 2,
                                        Screen_Width / 1024 * 12.0f,
                                        ImVec4(155, 155, 155, 255),
                                        u8"暂无任何记录"
                                    );
                                }
                                else {
                                    //cout << "==\n";
                                    for (int i = 1; i < ranking.size(); i++) {
                                        string ID = get<0>(ranking[i]);
                                        //cout << socketManager->UserRanking[ID]["name"].asString() << ":" << socketManager->UserRanking[ID]["isFinish"].asInt() << "\n";

                                        float offsetx = 0;// - w_;//RankingPaddingLeft[i] * w_ - w_;
                                        MapDisplayer.setMap(socketManager->AvatorInfo[ID]);

                                        MapDisplayer.setCenter(width / 120 + 1.0f * w_ / 3 / 2 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + 1.0f * w_ / 3 / 2 + width / 120);
                                        MapDisplayer.setSize(w_ / 3);
                                        MapDisplayer.displayFinish();

                                        drawRect(
                                            width / 120 + w_ / 3 - w_ / 3 / 3 / 2 + offsetx,
                                            (w_ / 3 + width / 120) * (i - 1) + width / 120 - w_ / 3 / 3 / 2 + width / 120,
                                            w_ / 3 / 3,
                                            w_ / 3 / 3,
                                            (i == 1) ? ImVec4(1.0f, 0.58823f, 0.03921f, 1.0f) : (i == 2) ? ImVec4(0.54117f, 0.54117f, 0.54117f, 1.0f) : (i == 3) ? ImVec4(0.83137f, 0.32549f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                                        );

                                        drawText(
                                            width / 120 + w_ / 3 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120,
                                            Screen_Width / 1024 * 12.0f,
                                            ImVec4(0, 0, 0, 255),
                                            to_string(i)
                                        );

                                        if (userManager->getRatingColor(socketManager->UserInfo[ID]["Rating"].asFloat()).w > 0) {
                                            drawText(
                                                width / 120 + w_ / 3 + width / 120 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120,
                                                Screen_Width / 1024 * 12.0f,
                                                userManager->getRatingColor(socketManager->UserInfo[ID]["Rating"].asFloat()),
                                                encode::string_To_UTF8(socketManager->UserInfo[ID]["name"].asString()).c_str(),
                                                0
                                            );
                                            if (ImGui::IsItemHovered()) {
                                                ImGui::SetTooltip(userManager->getRatingName(socketManager->UserInfo[ID]["Rating"].asFloat()).c_str());
                                            }
                                        }
                                        else {
                                            drawText_RAINBOW(
                                                width / 120 + w_ / 3 + width / 120 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120,
                                                Screen_Width / 1024 * 12.0f,
                                                encode::string_To_UTF8(socketManager->UserInfo[ID]["name"].asString()).c_str(),
                                                0,
                                                userManager->getRatingName(socketManager->UserInfo[ID]["Rating"].asFloat())
                                            );
                                        }

                                        drawText(
                                            width / 120 + w_ / 3 + width / 120 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + Screen_Width / 1024 * 12.0f + width / 120 + width / 120,
                                            Screen_Width / 1024 * 16.0f,
                                            ImVec4(155 / 255.0, 155 / 255.0, 155 / 255.0, 255),
                                            (msg["mode"].asInt() == 1) ? to_string(get<1>(ranking[i])) : sec2time(get<2>(ranking[i])),
                                            0
                                        );
                                    }
                                    drawText(
                                        -Screen_Width / 1024 * 12.0f, (w_ / 3 + width / 120) * (ranking.size() - 1) + width / 120 + width / 120,
                                        Screen_Width / 1024 * 12.0f,
                                        ImVec4(0, 0, 0, 255),
                                        u8"."
                                    );
                                }
                                if (subWindow.size() || ButtonLock)drawRect(0, 0, w_, h_, ImVec4(0, 0, 0, 50));

                                //界面切换用的FRAMEMASK
                                drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                            }
                            ImGui::EndChild();
                        }
                    }
                    else if (msg["type"] == "SHARE") {
                        ImVec2 end;
                        if (msg["userID"].asString() != userManager->getID()) {//   不是我发的
                            ImVec2 rect_ = ImGui::GetCursorPos();
                            MapDisplayer.setMap(socketManager->AvatorInfo[msg["userID"].asString()]);
                            MapDisplayer.setCenter(width / 120 + w_ / 3 / 6, width / 120 + rect_.y + w_ / 3 / 6);
                            MapDisplayer.setSize(1.0f * w_ / 3 / 3);
                            MapDisplayer.displayFinish();

                            //ImGui::SameLine();
                            ImGui::SetCursorPos({ width / 120 * 2 + 1.0f * w_ / 3 / 3 ,rect_.y });
                            rect_ = ImGui::GetCursorPos();
                            drawText(
                                width / 120 + 1.0f * w_ / 3 / 3, rect_.y,
                                Screen_Width / 1024 * 12.0f,
                                (msg["title"] == "server") ? ImVec4(1.0, 0.4, 0.4, 1) : ImVec4(0.7, 0.7, 0.7, 1),
                                encode::string_To_UTF8(msg["from"].asString()),
                                0
                            );
                            ImGui::SetCursorPos({ rect_.x, ImGui::GetCursorPosY() });
                            rect_ = ImGui::GetCursorPos();

                            //R["type"] = "SHARE";
                            //R["isDownloaded"] = 0;
                            //R["userID"] = userManager->getID();

                            CMap* tmp = new CMap();
                            tmp->LoadMapFromJson(msg);
                            ImGui::PushStyleColor(ImGuiCol_Button, msg["isDownloaded"].asInt() == 1 ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, msg["isDownloaded"].asInt() == 1 ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, msg["isDownloaded"].asInt() == 1 ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
                            if (ImageButton(u8"保存到本地", { 1.0f*w_ - 5 * width / 120 - 1.0f * w_ / 3 / 3 - OFFSETXw, 1.0f * h_ / 3 }, imageLoader->IMG["eye"].image, 1, { 1.0f,1.0f }, msg["Time"].asInt()) && msg["isDownloaded"].asInt() == 0) {
                                AudioLoader::get().playButton("TAP");
                                msg["isDownloaded"] = 1;
                                if (!IDtoMaps.count(stoi(msg["ID"].asString()))) {//    本地有的就别再下了
                                    customManager->SaveMap("Map\\S" + to_string(msg["ID"].asInt()) + ".dat", tmp);

                                    Maps.push_back(new Map("S" + to_string(msg["ID"].asInt()) + ".dat"));
                                    IDtoPath[(*Maps.rbegin())->getID()] = "S" + to_string(msg["ID"].asInt()) + ".dat";
                                    refreshMapIndex();
                                }
                            }
                            ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

                            MapDisplayer.setMap(tmp);
                            MapDisplayer.setCenter(rect_.x + 1.0f * h_ / 3 / 2, rect_.y + 1.0f * h_ / 3 / 2);
                            MapDisplayer.setSize(1.0f * h_ / 3 * 0.8);
                            MapDisplayer.displayFinish();
                            delete tmp;

                            //ImGui::SetCursorPos({ rect_.x + 1.0f * h_ / 3, rect_.y });
                            drawText(
                                rect_.x + 1.0f * h_ / 3,
                                rect_.y + width / 120,
                                Screen_Width / 1024 * 12.0f,
                                ImVec4(1, 1, 1, 1),
                                msg["name"].asString(),
                                0
                            );

                            end = rect_;
                            end.y = end.y + 1.0f * h_ / 3 + width / 120;
                        }
                        else {//    是我发的
                            ImVec2 rect_ = ImGui::GetCursorPos();
                            MapDisplayer.setMap(socketManager->AvatorInfo[msg["userID"].asString()]);
                            MapDisplayer.setCenter(w_ - w_ / 3 / 6 - width / 120 * 3 - OFFSETXw, width / 120 + rect_.y + w_ / 3 / 6);
                            MapDisplayer.setSize(1.0f * w_ / 3 / 3);
                            MapDisplayer.displayFinish();

                            ImVec2 rect = getTextSize(Screen_Width / 1024 * 12.0f, encode::string_To_UTF8(msg["from"].asString()));
                            //ImGui::SameLine();
                            rect_ = ImGui::GetCursorPos();
                            drawText(
                                w_ - 1.0f * w_ / 3 / 3 - 2 * width / 120 * 2 - rect.x - OFFSETXw, rect_.y,// + width / 120,
                                Screen_Width / 1024 * 12.0f,
                                (msg["title"] == "server") ? ImVec4(1.0, 0.4, 0.4, 1) : ImVec4(0.7, 0.7, 0.7, 1),
                                encode::string_To_UTF8(msg["from"].asString()),
                                0
                            );
                            ImGui::SetCursorPos({ width/120, ImGui::GetCursorPosY()});
                            rect_ = ImGui::GetCursorPos();

                            ImageButton(u8"[地图文件]", {1.0f * w_ - 5 * width / 120 - 1.0f * w_ / 3 / 3 - OFFSETXw, 1.0f * h_ / 3}, imageLoader->IMG["eye"].image, 1, {1.0f,1.0f}, msg["Time"].asInt());

                            MapDisplayer.setMap(Maps[IDtoMaps[msg["ID"].asInt()]]);
                            MapDisplayer.setCenter(rect_.x + 1.0f * h_ / 3 / 2, rect_.y + 1.0f * h_ / 3 / 2);
                            MapDisplayer.setSize(1.0f * h_ / 3 * 0.8);
                            MapDisplayer.displayFinish();

                            drawText(
                                rect_.x + 1.0f * h_ / 3,
                                rect_.y + width / 120,
                                Screen_Width / 1024 * 12.0f,
                                ImVec4(1, 1, 1, 1),
                                msg["name"].asString(),
                                0
                            );
                            end = rect_;
                            end.y = end.y + 1.0f * h_ / 3 + width / 120;
                        }

                        drawText(
                            -Screen_Width / 1024 * 12.0f, end.y,
                            Screen_Width / 1024 * 12.0f,
                            ImVec4(0, 0, 0, 255),
                            u8"."
                        );
                    }
                    //i++;
                    ImGui::Separator();
                };

                for (auto& msg : socketManager->getMessageList()) {
                    ShowMsg(msg);
                }
                if (ImGui::GetCursorPosY() > h_ - buttonwidth / kbw - width / 120 * 3) {
                    OFFSETXw = width / 120 * 2;
                }
                else {
                    OFFSETXw = 0;
                }
                if (socketManager->pagedown) {
                    if (socketManager->pagedown == 2) {
                        socketManager->pagedown = 0;
                        ImGui::SetScrollY(ImGui::GetScrollMaxY());
                    }
                    else {
                        socketManager->pagedown++;
                    }
                }
            }
            //界面切换用的FRAMEMASK
            drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
            ////界面切换用的FRAMEMASK
            //drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
            if (subWindow.size()) drawRect(0, 0, width, height, ImColor(0, 0, 0, 255));
            ImGui::EndChild();
        }

        ImGui::SetCursorPos({ width / 120,h_ * 1.0f - buttonwidth / kbw });
        ImGui::SetNextItemWidth(w_ - buttonwidth * 2 / kbw - width / 120 * 2);
        if (subWindow.empty()) {
            ImGui::InputTextMultiline(u8"  ", message, IM_ARRAYSIZE(message), ImVec2(w_ - buttonwidth * 2 / kbw - width / 120, buttonwidth / kbw - width / 120));
        }
        ImGui::SetCursorPos({ w_ - buttonwidth * 2 / kbw, h_ * 1.0f - buttonwidth / kbw });
        if (ImGui::Button(u8"发送", { buttonwidth * 2 / kbw - width / 120,buttonwidth / kbw - width / 120 }) && !ButtonLock && subWindow.size() == 0 && string(message) != "")
        {
            AudioLoader::get().playButton("TAP");
            time_t timer;
            tm t;
            time(&timer);
            localtime_s(&t, &timer);
            stringstream ss;
            ss << setw(2) << setfill('0') << right << t.tm_hour << ":" << setw(2) << setfill('0') << right << t.tm_min << ":" << setw(2) << setfill('0') << right << t.tm_sec;
            Json::Value R;
            R["type"] = "MESSAGE";
            R["time"] = ss.str();
            R["from"] = userManager->getName();
            R["ID"] = userManager->getID();
            R["body"] = encode::UTF8_To_String(message);
            R["title"] = (socketManager->getStatu() == SERVER) ? "server" : "client";

            socketManager->SplitSender(JSONToEJSON(R));
            if (socketManager->getStatu() == SERVER) {
                socketManager->MessageList.push_back(R);
                socketManager->pagedown = 1;
            }

            memset(message, 0, sizeof(message));
            message[0] = '\0';
        }
        //socketManager->SplitSender(encode::UTF8_To_String(message));

        //界面切换用的FRAMEMASK
        //drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
        if (subWindow.size()) drawRect(0, 0, width, height, ImColor(0, 0, 0, 255));
        //界面切换用的FRAMEMASK
        drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
        ImGui::EndChild();
    }
    //////////////////////////////中间部分//////////////////////////////////////
    //     //  绘制中间部分
    if (socketManager->getStatu() == SERVER) {
        if (!Maps.empty()) {
            static bool editingName = 0;
            static char buf[15];
            //cout << "第" << selectIndex << "个地图\n";
            float dx = width / 2 + width / 120 * 3;

            MapDisplayer.setMap(Maps[selectIndex]);
            MapDisplayer.setCenter(dx, height / 2);
            MapDisplayer.setSize(width / 3);
            MapDisplayer.displayFinish();

            drawText(
                dx, height / 2 + width / 3 / 2 + Screen_Width / 1024 * 24.0f + width / 120,
                Screen_Width / 1024 * 24.0f,
                ImVec4(255, 255, 255, 255),
                Maps[selectIndex]->getName()
            );

            ImGui::SetCursorPos({
                dx - width / 120 - width / 3 / 2, height - buttonwidth / kbw * 2 - width / 120
                });
            if (ImGui::Button(u8"以指定地图开始", ImVec2(width / 3 / 2, buttonwidth / kbw))&&subWindow.empty()) {
                AudioLoader::get().playButton("TAP");
                if (!socketManager->IsAnybodyOnline()) {//  人数不足以开始游戏
                    function<void(bool)> window = [&](bool alive)->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.6;
                        h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
                        x = width / 2;
                        y = height / 2;
                        ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                        ImGui::BeginChild(
                            u8"error",
                            ImVec2(w, h),
                            1,
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoSavedSettings// |
                            //ImGuiWindowFlags_NoMove
                        );
                        {
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"无法开始"
                            );
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"人数至少要达到 2 人才能开始游戏"
                            );
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                            {
                                AudioLoader::get().playButton("TAP");
                                subWindow.pop_back();
                            }
                        }
                        ImGui::EndChild();
                        };
                    subWindow.push_back({ window, 0, 0 });
                }
                else {
                    Json::Value R;
                    R["type"] = "START";
                    socketManager->SplitSender(JSONToEJSON(R));
                    SelectMap = Maps[selectIndex];
                    GameMap = new Map(SelectMap);// 记得delete

                    socketManager->ClearWatchList();
                    socketManager->InitRankingList();// 服务端自身初始化玩家状态
                    socketManager->setFinishStatu(0);
                    socketManager->isWatching = 0;
                    socketManager->UserRanking.clear();

                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;
                        AudioLoader::get().playBGM("GAME");
                        Tab = TAB::PkMode;
                        ranking.clear();
                        scoreboard->unlock();
                        scoreboard->Reset();
                        MapDisplayer.setMap(GameMap);
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                        });
                }
            }
            ImGui::SetCursorPos({
                dx + width / 120, height - buttonwidth / kbw * 2 - width / 120
                });
            if (ImGui::Button(u8"以随机地图开始", ImVec2(width / 3 / 2, buttonwidth / kbw)) && subWindow.empty()) {
                AudioLoader::get().playButton("TAP");
                if (!socketManager->IsAnybodyOnline()) {//  人数不足以开始游戏
                    function<void(bool)> window = [&](bool alive)->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.6;
                        h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
                        x = width / 2;
                        y = height / 2;
                        ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                        ImGui::BeginChild(
                            u8"error",
                            ImVec2(w, h),
                            1,
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoSavedSettings// |
                            //ImGuiWindowFlags_NoMove
                        );
                        {
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"无法开始"
                            );
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"人数至少要达到 2 人才能开始游戏"
                            );
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                            {
                                AudioLoader::get().playButton("TAP");
                                subWindow.pop_back();
                            }
                        }
                        ImGui::EndChild();
                        };
                    subWindow.push_back({ window, 0, 0 });
                }
                else {
                    //static int X = 10, Y = 10;
                    //static int MINStep = 1, MAXStep = 10;
                    static int PaletteSize = 6;//   2~14
                    //palette = randomPalette(rnd(10, 12));

                    function<void(bool)> window = [&](bool alive)->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.6;
                        h = height / 3.8 * 0.5 + (buttonwidth / kbw + width / 120) * 4;//
                        x = width / 2;
                        y = height / 2;
                        ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                        ImGui::BeginChild(
                            u8"error",
                            ImVec2(w, h),
                            1,
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoSavedSettings// |
                            //ImGuiWindowFlags_NoMove
                        );
                        {
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"随机地图参数设定"
                            );

                            ImGui::SliderInt(u8"宽度", &customManager->w, 3, MAX_WIDTH);
                            ImGui::SliderInt(u8"高度", &customManager->h, 3, MAX_HEIGHT);
                            ImGui::SliderInt(u8"最小数字", &customManager->maxstep, 1, 400);
                            ImGui::SliderInt(u8"最大数字", &customManager->minstep, 2, 400);
                            ImGui::SliderInt(u8"色盘大小", &PaletteSize, 2, 14);

                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2-width/120,h - 10.0f - buttonwidth / kbw });
                            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                            {
                                subWindow.pop_back();
                            }
                            ImGui::SetCursorPos({ w / 2 + width/120,h - 10.0f - buttonwidth / kbw });
                            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                            {
                                customManager->palette = customManager->randomPalette(PaletteSize);
                                customManager->Random();
                                customManager->EndCurrentToBeginGoal();

                                Json::Value R = SaveMapAsJson(customManager->getBegin());
                                R["type"] = "RANDOMSTART";
                                R["Rated"] = 0;
                                socketManager->SplitSender(JSONToEJSON(R));

                                if (socketManager->RandomMapID != "") {
                                    delete socketManager->tmpMapInfo[socketManager->RandomMapID];
                                    socketManager->tmpMapInfo.erase(socketManager->RandomMapID);
                                }
                                socketManager->RandomMapID = to_string(R["ID"].asInt());
                                socketManager->tmpMapInfo[socketManager->RandomMapID] = new CMap(customManager->getBegin());

                                socketManager->RandomStart = 1;

                                socketManager->ClearWatchList();
                                socketManager->InitRankingList();// 服务端自身初始化玩家状态
                                socketManager->setFinishStatu(0);
                                socketManager->isWatching = 0;
                                socketManager->UserRanking.clear();

                                subWindow.pop_back();
                            }
                        }
                        ImGui::EndChild();
                        };
                    subWindow.push_back({ window, 0, 0 });
                }
            }
        }
        else {

        }
    }
    else {
        if (socketManager->selectMapID != "" && socketManager->tmpMapInfo.count(socketManager->selectMapID)) {
            float dx = width / 2 + width / 120 * 3;

            MapDisplayer.setMap(socketManager->tmpMapInfo[socketManager->selectMapID]);
            MapDisplayer.setCenter(dx, height / 2);
            MapDisplayer.setSize(width / 3);
            MapDisplayer.displayFinish();

            drawText(
                dx, height / 2 + width / 3 / 2 + Screen_Width / 1024 * 24.0f + width / 120,
                Screen_Width / 1024 * 24.0f,
                ImVec4(255, 255, 255, 255),
                MapDisplayer.myMap->getName()
            );
        }
    }
    //////////////////////////用户端收到开始信息的行为///////////////////////////
    if (socketManager->getStatu() == CLIENT && socketManager->GetStart) {
        ButtonLock = 1;
        SelectMap = socketManager->tmpMapInfo[socketManager->selectMapID];//
        socketManager->setFinishStatu(0);
        GameMap = new Map(SelectMap);
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            AudioLoader::get().playBGM("GAME");
            Tab = TAB::PkMode;
            ranking.clear();
            scoreboard->unlock();
            scoreboard->Reset();
            MapDisplayer.setMap(GameMap);

            socketManager->GetStart = 0;
            socketManager->isWatching = 0;
            socketManager->UserRanking.clear();

            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }

    if (socketManager->RandomStart) {
        socketManager->RandomStart = 0;
        ButtonLock = 1;
        SelectMap = socketManager->tmpMapInfo[socketManager->RandomMapID];//
        socketManager->setFinishStatu(0);
        GameMap = new Map(SelectMap);
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            AudioLoader::get().playBGM("GAME");
            Tab = TAB::PkMode;
            ranking.clear();
            scoreboard->unlock();
            scoreboard->Reset();
            MapDisplayer.setMap(GameMap);

            socketManager->GetStart = 0;
            socketManager->isWatching = 0;
            socketManager->UserRanking.clear();

            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
            });
    }
    /////////////////////////////右侧关卡选择//////////////////////////////////
    static ImVec2 MousePos;
    static function<void()> Fun=nullptr;
    static int subMenuAlive = 0;
    static Map* ShareSelect = nullptr;

    if (1) {//    无意义if，折叠代码块用
        MapDisplayer.setSize(BH * 0.8f);
        string space = " ";
        int d = 0;
        float k = 0.4;
        for (auto level : Maps) {
            float offsetX;
            if (BH * d < scrollY)offsetX = (scrollY - BH * d) * k;
            else offsetX = (BH * d - scrollY) * k;
            ImGui::SetCursorPos({ width * (0.9f - 0.35f) + offsetX + OFFSETX,baseoffset + BH * d - scrollY + BH });/////////////////50.0f->width/20
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            bool leave = 0;
            if (ImGui::Button(space.c_str(), { width * 0.7f, BH }) && !ButtonLock && subWindow.size() == 0)
            {
                if (selectIndex != d) {
                    AudioLoader::get().playButton("SELECT");
                    selectIndex = d;
                    animeManager.addAnime(&scrollY, scrollY, scrollY + (BH * d - scrollY), 800, OutExpo);

                    if (socketManager->getStatu() == SERVER) {
                        socketManager->selectMapID=to_string(Maps[selectIndex]->getID());
                        Json::Value R;
                        R["type"] = "DISPLAY";
                        R["ID"] = to_string(Maps[selectIndex]->getID());
                        socketManager->SplitSender(JSONToEJSON(R));
                    }

                    ranking.clear();
                }
                else {
                    AudioLoader::get().playButton("TAP");
                    //initFont(height * 0.7f / max(GameMap->h, GameMap->w));
                    //SelectMap = level;
                    ////GameMap = new Map(level);

                    //ButtonLock = 1;
                    //swicthFrameR = swicthFrameL = 0;
                    //animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    //    ButtonLock = 0;

                    //    MapDisplayer.setMap(new Map(SelectMap));
                    //    leave = 1;
                    //    playerOperation = "";
                    //    Tab = TAB::Game;
                    //    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                    //        scoreboard->Reset();
                    //        });
                    //    });
                    //Tab = TAB::Panel;
                    //cout << level->getName() << d << ImGui::GetWindowPos().x << " " << ImGui::GetWindowPos().y;
                }
            }
            
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                MousePos = ImGui::GetMousePos() - ImGui::GetWindowPos();
                Fun = [&]() {
                    Json::Value R=SaveMapAsJson((CMap*)ShareSelect);
                    R["type"] = "SHARE";
                    R["isDownloaded"] = 0;
                    R["userID"] = userManager->getID();
                    R["Time"] = time(0);
                    R["from"] = userManager->getName();
                    R["title"] = (socketManager->getStatu() == SERVER) ? "server" : "client";
                    socketManager->SplitSender(JSONToEJSON(R));
                    socketManager->MessageList.push_back(R);
                };
                subMenuAlive = 1;
                ShareSelect = level;
            }

            space += " ";
            ImGui::PopStyleColor();
            if (leave) break;
            ImGui::SetCursorPos({
                width * (0.9f - 0.35f) + BH + offsetX + OFFSETX,
                baseoffset + BH / 12 + BH * d - scrollY + BH/////////////////
                });

            ImGui::PushFont(Font);
            ImGui::SetWindowFontScale(0.75);
            ImGui::Text(level->getName().c_str());
            if (userManager->getName() != "未登录") {
                if (userManager->getRawData()["User"][userManager->getName()]["mapState"][to_string(level->getID())].size() == 0) {
                    userManager->getRawData()["User"][userManager->getName()]["mapState"].removeMember(to_string(level->getID()));
                }
                else {
                    ImGui::SetCursorPos({
                        width * (0.9f - 0.35f) + BH + offsetX + OFFSETX,
                        ImGui::GetCursorPosY()/////////////////
                        });
                    ImGui::TextColored(ImVec4(0, 170, 0, 255), "Accepted");
                }
            }
            ImGui::SetWindowFontScale(1.0);
            ImGui::PopFont();

            MapDisplayer.setCenter(width * (0.9f - 0.35f) + BH / 2 + offsetX + OFFSETX, baseoffset + BH * d + BH / 2 - scrollY + BH);
            MapDisplayer.setMap(level);
            MapDisplayer.displayFinish();
            d++;
        }
        drawRect(
            width - 5.0f - width / 120,
            scrollY / (BH * Maps.size() - BH) * (height - height / Maps.size()),
            width / 120,
            height / Maps.size(),
            ImVec4(0.9, 0.9, 0.9, 1)
        );
        if (Maps.empty()) {
            drawText(
                width * (0.9f - 0.35f) + BH + OFFSETX, height / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                u8"无可用地图"
            );
        }

        if (Fun != nullptr) {
            w_ = buttonwidth / kbw * 4 + width / 120 * 2;//width/120常用的小量
            h_ = buttonwidth / kbw + width / 120 * 2;
            ImGui::SetCursorPos(MousePos);
            ImGui::BeginChild(
                u8"子菜单 ",
                { w_ * 1.0f,h_ * 1.0f },
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings
            );
            {
                drawRect(0, 0, w_, h_, ImVec4(0, 0, 0, 1));
                ImGui::SetCursorPos({ width / 120,width / 120 });
                if (ImGui::Button(u8"分享", { buttonwidth / kbw * 4 ,buttonwidth / kbw })) {
                    AudioLoader::get().playButton("TAP");
                    Fun();
                    Fun = nullptr;
                }
                if (subMenuAlive == 2 && !ImGui::IsItemHovered() && (io.MouseWheel != 0 || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Left))) {
                    Fun = nullptr;
                    subMenuAlive = 0;
                }
                if (subMenuAlive == 1)subMenuAlive++;
            }
            ImGui::EndChild();
        }
    }

    //////////////////////////////////返回////////////////////////////////////
    if (socketManager->kickInfo != Json::nullValue && socketManager->kickInfo["comfirm"].asInt()==0) {
        //ButtonLock = 0;
        socketManager->kickInfo["comfirm"] = 1;

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            socketManager->End();
            Tab = TAB::NetModeSelect;

            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"你被踢出了该房间"
                    );
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        encode::string_To_UTF8(socketManager->kickInfo["reason"].asString()).c_str()
                    );
                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                    {
                        subWindow.pop_back();
                        AudioLoader::get().playButton("TAP");
                    }
                }
                ImGui::EndChild();
                };
            subWindow.push_back({ window, 0, 0 });
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }

    if (socketManager->getStatu() == UNACTIVE && subWindow.empty()) {
        ButtonLock = 0;
        function<void(bool)> window = [&](bool alive)->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.6;
            h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"error",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );
            {
                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    u8"连接断开"
                );
                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    u8"房主似乎关闭了房间"
                );
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                {
                    AudioLoader::get().playButton("TAP");
                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;
                        subWindow.pop_back();
                        //socketManager->End();
                        Tab = TAB::NetModeSelect;
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                    });
                }
            }
            ImGui::EndChild();
        };
        subWindow.push_back({ window, 0, 0 });
    }

    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            socketManager->End();
            Tab = TAB::NetModeSelect;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();

    socketManager->Update();
}

void PKMODE() {
    {// 掉线处理
        if (socketManager->getStatu() == UNACTIVE) {
            if (subWindow.empty()) {
                ButtonLock = 0;
                function<void(bool)> window = [&](bool alive)->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.6;
                    h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
                    x = width / 2;
                    y = height / 2;
                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                    ImGui::BeginChild(
                        u8"error",
                        ImVec2(w, h),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings// |
                        //ImGuiWindowFlags_NoMove
                    );
                    {
                        drawText(
                            w / 2, Screen_Width / 1024 * 24.0f / 2,
                            Screen_Width / 1024 * 24.0f / 2,
                            ImVec4(255, 255, 255, 255),
                            u8"连接断开"
                        );
                        drawText(
                            w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                            Screen_Width / 1024 * 24.0f / 2,
                            ImVec4(255, 255, 255, 255),
                            u8"你与该房间断开了连接"
                        );
                        ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                        {
                            AudioLoader::get().playButton("TAP");
                            ButtonLock = 1;
                            swicthFrameR = swicthFrameL = 0;
                            animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                                ButtonLock = 0;
                                subWindow.pop_back();
                                //socketManager->End();
                                AudioLoader::get().playBGM("MAP");
                                Tab = TAB::NetModeSelect;
                                delete GameMap;
                                while (LastMap.size()) {
                                    Map* tmp = LastMap.top();
                                    LastMap.pop();
                                    delete tmp;
                                }
                                animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                                });
                        }
                    }
                    ImGui::EndChild();
                    };
                subWindow.push_back({ window, 0, 0 });
            }
            return;
        }
    }
    {// 实时排行榜
        static int mode = 1;//1为按罚时排序，2为按用时排序

        w_ = width / 3 + width * 0.08f - width / 3 / 2 - width / 120;//width/120常用的小量
        h_ = height / 2;
        drawText(
            width / 120, h_ - Screen_Width / 1024 * 12.0f,
            Screen_Width / 1024 * 12.0f,
            ImVec4(255, 255, 255, 255),
            u8"排行榜",
            0
        );
        //ImGui::SetCursorPos({ w_ - Screen_Width / 1024 * 12.0f - width / 120, width / 120 });
        ImGui::SameLine();
        if (ImageButton(
            (mode == 1) ? u8"按罚时排序" : u8"按用时排序",
            ImVec2(Screen_Width / 1024 * 12.0f, Screen_Width / 1024 * 12.0f),
            imageLoader->IMG[(mode == 1) ? "penelty" : "time"].image,
            true,
            ImVec2(Screen_Width / 1024 * 12.0f * 0.9, Screen_Width / 1024 * 12.0f * 0.9)
        )) {
            AudioLoader::get().playButton("TAP");
            mode = 3 - mode;
        }

        static bool osciilator = 1;
        //static vector<tuple<string, int, int>> last_ranking;
        static map<string, int> last_ranking;
        static vector<float> RankingPaddingUp;
        if ((int(clock() / 500) & 1) ^ osciilator) {//   D触发器，每500ms更新一次
            osciilator ^= 1;
            //cout << osciilator;
            last_ranking.clear();// 记录该玩家上一轮的排名
            for (int i = 1; i < ranking.size(); i++) {
                last_ranking[get<0>(ranking[i])] = i;
            }
            ranking.clear();//          可以有优化，比如让排行榜在更新信息时才发生变化
        }

        if (ranking.empty()) {
            //cout << "初始化";
            RankingPaddingUp.clear();
            RankingPaddingUp.push_back(0);
            ranking.push_back({ "",-1,-1 });//    保证ranking非空

            //  这里开始往ranking里面写东西

            for (auto& [ID, R] : socketManager->UserRanking) {
                ranking.push_back({ R["ID"].asString(),R["penelty"].asInt(),R["time"].asInt() });
                RankingPaddingUp.push_back(0);
            }

            //for (string name : userManager->getRawData()["User"].getMemberNames()) {
            //    int penelty = 1e9;
            //    int time = 1e9;
            //    if (userManager->getRawData()["User"][name]["mapState"][to_string(Maps[selectIndex]->getID())].size() == 0) {
            //        userManager->getRawData()["User"][name]["mapState"].removeMember(to_string(Maps[selectIndex]->getID()));
            //    }
            //    else {
            //        for (Json::Value& state : userManager->getRawData()["User"][name]["mapState"][to_string(Maps[selectIndex]->getID())]) {
            //            penelty = MIN(penelty, state["penelty"].asInt());
            //            time = MIN(time, state["time"].asInt());
            //        }
            //    }
            //    if (time != 1e9) {
            //        ranking.push_back({ name,penelty,time });
            //        //RankingPaddingLeft.push_back(0);
            //        //不能在这里绑定动画，因为vector容器会扩容
            //        //animeManager.addAnime(&RankingPaddingLeft[RankingPaddingLeft.size() - 1], 0, 1, 500, OutBack, (RankingPaddingLeft.size() - 2) * 70);
            //    }
            //}

            sort(ranking.begin() + 1, ranking.end(), [&](tuple<string, int, int> a, tuple<string, int, int> b) {//      对玩家的成绩进行排序
                if (socketManager->UserRanking[get<0>(a)]["isFinish"].asInt()==0 && socketManager->UserRanking[get<0>(b)]["isFinish"].asInt()==0) {
                    if ((mode == 1) ? (get<1>(a) == get<1>(b)) : (get<2>(a) == get<2>(b))) {
                        if ((mode == 1) ? (get<2>(a) == get<2>(b)) : (get<1>(a) == get<1>(b))) {
                            return get<0>(a) < get<0>(b);// 比字符串，总不可能名字还相等了
                        }
                        return (mode == 1) ? (get<2>(a) < get<2>(b)) : (get<1>(a) < get<1>(b));
                    }
                    return (mode == 1) ? (get<1>(a) < get<1>(b)) : (get<2>(a) < get<2>(b));
                }
                else if(socketManager->UserRanking[get<0>(a)]["isFinish"].asInt() ^ socketManager->UserRanking[get<0>(b)]["isFinish"].asInt()){//   赛时先完成的人排在前面
                    return socketManager->UserRanking[get<0>(a)]["isFinish"].asInt() > socketManager->UserRanking[get<0>(b)]["isFinish"].asInt();
                }
                else {
                    return get<2>(a) < get<2>(b);
                }
            });


            for (int i = 1; i < ranking.size(); i++) {//    根据玩家的排名变动，绑定动画
                animeManager.addAnime(&RankingPaddingUp[i], (last_ranking[get<0>(ranking[i])] - i), 0, 500, InSine);
            }
        }

        ImGui::SetCursorPos({ 0.0f, 1.0f * h_ });
        ImGui::BeginChild(
            u8"排行榜 ",
            { w_ * 1.0f,h_ * 1.0f },
            1,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings
        );

        if (ranking.size() == 1) {
            drawText(
                w_ / 2, h_ / 2,
                Screen_Width / 1024 * 12.0f,
                ImVec4(155, 155, 155, 255),
                u8"暂无任何记录"
            );
        }
        else {
            //cout << "==\n";
            for (int i = 1; i < ranking.size(); i++) {
                string ID = get<0>(ranking[i]);
                //cout << socketManager->UserRanking[ID]["name"].asString() << ":" << socketManager->UserRanking[ID]["isFinish"].asInt() << "\n";

                float offsetx = 0;// - w_;//RankingPaddingLeft[i] * w_ - w_;
                float offsety = RankingPaddingUp[i] * (w_ / 3 + width / 120);
                MapDisplayer.setMap(socketManager->AvatorInfo[ID]);

                MapDisplayer.setCenter(
                    width / 120 + 1.0f * w_ / 3 / 2 + offsetx, 
                    (w_ / 3 + width / 120) * (i - 1) + width / 120 + 1.0f * w_ / 3 / 2 + width / 120+ offsety//////////////////
                );
                MapDisplayer.setSize(w_ / 3);
                MapDisplayer.displayFinish();
                
                ImGui::SetCursorPos({
                    width / 120 + 1.0f * w_ / 3 / 2 + offsetx - w_ / 3 / 2 - w_ / 3 / 8,
                    (w_ / 3 + width / 120) * (i - 1) + width / 120 + 1.0f * w_ / 3 / 2 + width / 120 + w_ / 3 / 2 - w_ / 3 / 8 + offsety//////////////////
                });
                if (socketManager->getFinishStatu() && ImageButton(u8"观战", {1.0f * w_ / 3 / 4 ,1.0f * w_ / 3 / 4}, imageLoader->IMG["eye"].image, 0, {0,0}, stoi(ID))) {
                    AudioLoader::get().playButton("TAP");
                    socketManager->isWatching = (ID != userManager->getID());

                    if (socketManager->getStatu() == SERVER) {//    本来打算写自己发给自己的，但还是算了
                        socketManager->watchBinder(userManager->getID(), ID);
                    }
                    else {
                        Json::Value R;
                        R["type"] = "WATCH";
                        R["from"] = userManager->getID();
                        R["to"] = ID;
                        socketManager->SplitSender(JSONToEJSON(R));//   向服务端发送申请
                    }
                }

                if (socketManager->getStatu() == SERVER) {
                    ImGui::SetCursorPos({
                        1.0f * w_ + offsetx - width/120 - 1.0f * w_ / 3 / 4,
                        (w_ / 3 + width / 120) * (i - 1) + width / 120 + 1.0f * w_ / 3 / 2 + width / 120 - w_ / 3 / 8 + offsety//////////////////
                        });
                    if (userManager->getID()!=ID && ImageButton(u8"踢出房间", {1.0f * w_ / 3 / 4 ,1.0f * w_ / 3 / 4}, imageLoader->IMG["move"].image, 0, {0,0}, stoi(ID)) && subWindow.empty()) {
                        AudioLoader::get().playButton("TAP");
                        static string kickID = ID;
                        function<void(bool)> window = [&](bool alive) ->void {
                            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                            float w, h, x, y;//长宽，中心坐标
                            w = width / 2.8;
                            h = height / 6;
                            x = width / 2;
                            y = height / 2;
                            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                            ImGui::BeginChild(
                                u8"确认窗口",
                                ImVec2(w, h),
                                1,
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings// |
                                //ImGuiWindowFlags_NoMove
                            );

                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"你确定要踢出该玩家吗？"
                            );
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
                            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                            {
                                AudioLoader::get().playButton("TAP");
                                Json::Value R;
                                R["type"] = "KICK";
                                R["reason"] = encode::UTF8_To_String(u8"房主将你踢出了房间");
                                R["comfirm"] = 0;
                                socketManager->SplitSender(JSONToEJSON(R), { socketManager->IDToSocket[kickID] });

                                socketManager->RemoveUserFromUnfinished(kickID);
                            }
                            ImGui::SetCursorPos({ w / 2 + buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
                            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                            {
                                AudioLoader::get().playButton("TAP");
                                subWindow.pop_back();
                            }

                            ImGui::PopStyleColor();
                            ImGui::EndChild();

                            };
                        subWindow.push_back({ window,0,0 });
                    }
                }

                drawRect(
                    width / 120 + w_ / 3 - w_ / 3 / 3 / 2 + offsetx,
                    (w_ / 3 + width / 120) * (i - 1) + width / 120 - w_ / 3 / 3 / 2 + width / 120 + offsety,//////////////////
                    w_ / 3 / 3,
                    w_ / 3 / 3,
                    (i == 1) ? ImVec4(1.0f, 0.58823f, 0.03921f, 1.0f) : (i == 2) ? ImVec4(0.54117f, 0.54117f, 0.54117f, 1.0f) : (i == 3) ? ImVec4(0.83137f, 0.32549f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                );

                drawText(
                    width / 120 + w_ / 3 + offsetx, (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120 + offsety,//////////////////
                    Screen_Width / 1024 * 12.0f,
                    ImVec4(0, 0, 0, 255),
                    to_string(i)
                );

                if (userManager->getRatingColor(socketManager->UserInfo[ID]["Rating"].asFloat()).w > 0) {
                    drawText(
                        width / 120 + w_ / 3 + width / 120 + offsetx,
                        (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120 + offsety,//////////////////
                        Screen_Width / 1024 * 12.0f,
                        userManager->getRatingColor(socketManager->UserInfo[ID]["Rating"].asFloat()),
                        encode::string_To_UTF8(socketManager->UserInfo[ID]["name"].asString()).c_str(),
                        0
                    );
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(userManager->getRatingName(socketManager->UserInfo[ID]["Rating"].asFloat()).c_str());
                    }
                }
                else {
                    drawText_RAINBOW(
                        width / 120 + w_ / 3 + width / 120 + offsetx, 
                        (w_ / 3 + width / 120) * (i - 1) + width / 120 + width / 120 + offsety,//////////////////
                        Screen_Width / 1024 * 12.0f,
                        encode::string_To_UTF8(socketManager->UserInfo[ID]["name"].asString()).c_str(),
                        0,
                        userManager->getRatingName(socketManager->UserInfo[ID]["Rating"].asFloat())
                    );
                }

                drawText(
                    width / 120 + w_ / 3 + width / 120 + offsetx,
                    (w_ / 3 + width / 120) * (i - 1) + width / 120 + Screen_Width / 1024 * 12.0f + width / 120 + width / 120 + offsety,//////////////////
                    Screen_Width / 1024 * 16.0f,
                    ImVec4(155 / 255.0, 155 / 255.0, 155 / 255.0, 255),
                    (mode == 1) ? to_string(get<1>(ranking[i])) : sec2time(get<2>(ranking[i])),
                    0
                );


            }
            drawText(
                -Screen_Width / 1024 * 12.0f, (w_ / 3 + width / 120) * (ranking.size() - 1) + width / 120 + width / 120,
                Screen_Width / 1024 * 12.0f,
                ImVec4(0, 0, 0, 255),
                u8"."
            );
        }
        if (subWindow.size() || ButtonLock)drawRect(0, 0, w_, h_, ImVec4(0, 0, 0, 50));

        //界面切换用的FRAMEMASK
        drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
        ImGui::EndChild();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////在下面，不允许再改变MapDisplayer指向的地图了////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (socketManager->isWatching) {
        MapDisplayer.setMap(socketManager->watchMap);
    }
    else {
        MapDisplayer.setMap(GameMap);
    }
    MapDisplayer.setSize(height * 0.7f);
    MapDisplayer.setCenter(width * 0.5f, height * 0.5f);
    MapDisplayer.display();

    MapDisplayer.setSize(height * 0.3f);
    MapDisplayer.setCenter(width * 0.133f, height * 0.25f);
    MapDisplayer.displayFinish();
    //======================================================//
    //void drawText(float x, float y, float size, ImVec4 color, string text)
    drawText(
        width * 0.86f,
        height * 0.2f,
        buttonwidth / kbw * 0.6,
        ImVec4(255, 255, 255, 255),
        "Penelty"
    );
    drawText(
        width * 0.86f,
        height * 0.2f + buttonwidth / kbw * 0.8,
        buttonwidth / kbw,
        ImVec4(255, 255, 255, 255),
        to_string(scoreboard->calcScore())
    );
    drawText(
        width * 0.5f,
        buttonwidth / kbw * 0.7,
        buttonwidth / kbw,
        ImVec4(255, 255, 255, 255),
        sec2time(scoreboard->GetTime())
    );
    //======================================================//
    function<void()> UpdateMyRanking = [&]() {
        Json::Value R;
        R["type"] = "RANKING";
        R["ID"] = userManager->getID();
        R["name"] = userManager->getName();
        R["penelty"] = scoreboard->calcScore();
        R["time"] = scoreboard->GetTime();
        R["Rating"] = userManager->getRating();
        R["isFinish"] = socketManager->getFinishStatu();//    0代表未通关，1代表通关，负数代表离开（未完成）

        socketManager->UserRanking[userManager->getID()] = R;
        socketManager->SplitSender(JSONToEJSON(R));//   广播/向服务器转发 自己的score，time

        //  这里还要判一下有没有人观战自己（服务端情况下）
        if (socketManager->getStatu() == SERVER) {
            if (socketManager->getWatchingList().count(userManager->getID())) {

                std::thread t([&]() {
                    Json::Value reply;

                    while (GameMap == nullptr || GameMap->isEditing);//cout << "被占用的地图\n";
                    GameMap->isEditing = 1;
                    reply = SaveMapAsJson((CMap*)GameMap);
                    GameMap->isEditing = 0;

                    reply["type"] = "MAP_FOR_WATCH";
                    for (string from : socketManager->getWatchingList()[userManager->getID()]) {
                        socketManager->SplitSender(JSONToEJSON(reply), { socketManager->IDToSocket[from] });//  向客户端发送
                    }
                });

                t.detach();
            }
        }
    };

    static int scoretime = -1;
    if (scoretime != scoreboard->GetTime()) {
        scoretime = scoreboard->GetTime();
        UpdateMyRanking();
    }
    /////////////////////////////////////////////////////////////////////////
    if (!socketManager->getFinishStatu()) {
        ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"逆时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            hintstate = NONE;
            LastMap.push(new Map(MapDisplayer.myMap));
            MapDisplayer.myMap->anticlockwise();
            scoreboard->step();

            UpdateMyRanking();
        }
        ImGui::PopStyleColor();
        if (hintstate == ANTICLOCK)drawRect(
            width * 0.86f - buttonwidth / 2,
            height * 0.4f,
            buttonwidth / 2,
            buttonwidth / kbw + 1,
            ImVec4(255, 255, 0, 100)
        );
        /////////////////////////////////////////////////////////////////////////
        ImGui::SetCursorPos({ width * 0.86f , height * 0.4f });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"顺时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            hintstate = NONE;
            LastMap.push(new Map(MapDisplayer.myMap));
            MapDisplayer.myMap->clockwise();
            scoreboard->step();
            UpdateMyRanking();
        }
        ImGui::PopStyleColor();
        if (hintstate == CLOCK)drawRect(
            width * 0.86f,
            height * 0.4f,
            buttonwidth / 2,
            buttonwidth / kbw + 1,
            ImVec4(255, 255, 0, 100)
        );
        /////////////////////////////////////////////////////////////////////////
        ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 4, height * 0.4f + 1.5f * buttonwidth / kbw });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"燃烧", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            hintstate = NONE;
            Map* tmp = new Map(MapDisplayer.myMap);
            if (MapDisplayer.myMap->Burn())LastMap.push(tmp);
            scoreboard->step();

            UpdateMyRanking();
        }
        ImGui::PopStyleColor();
        if (hintstate == BURN)drawRect(
            width * 0.86f - buttonwidth / 4,
            height * 0.4f + 1.5f * buttonwidth / kbw,
            buttonwidth / 2,
            buttonwidth / kbw + 1,
            ImVec4(255, 255, 0, 100)
        );
        /////////////////////////////////////////////////////////////////////////
        ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2/2, height * 0.4f + 3 * buttonwidth / kbw });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"撤回", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            hintstate = NONE;
            if (!LastMap.empty()) {
                Map* tmp = GameMap;//MapDisplayer.myMap;
                //MapDisplayer.myMap = LastMap.top();
                GameMap = LastMap.top();
                LastMap.pop();
                delete tmp;
                scoreboard->regret();

                UpdateMyRanking();
            }
        }
        ImGui::PopStyleColor();
        if (hintstate == RETREAT)drawRect(
            width * 0.86f - buttonwidth / 2,
            height * 0.4f + 3 * buttonwidth / kbw,
            buttonwidth / 2,
            buttonwidth / kbw + 1,
            ImVec4(255, 255, 0, 100)
        );
        /////////////////////////////////////////////////////////////////////////
        //ImGui::SetCursorPos({ width * 0.86f , height * 0.4f + 3 * buttonwidth / kbw });
        //ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        //if (ImGui::Button(u8"提示", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        //{
        //    hintstate = MapDisplayer.myMap->Hint();
        //    scoreboard->hint();
        //}
        //ImGui::PopStyleColor();
        /////////////////////////////////////////////////////////////////////////
        ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"提交答案", { buttonwidth, buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            hintstate = NONE;
            //LastMap.push(new Map(MapDisplayer.myMap));
            if (!MapDisplayer.myMap->Submit()) {
                scoreboard->submit();

                UpdateMyRanking();
            }
            else {
                ButtonLock = 0;
                socketManager->setFinishStatu(1);
                UpdateMyRanking();
                socketManager->RemoveUserFromUnfinished(userManager->getID());
                scoreboard->lock();

                while (LastMap.size()) {
                    Map* tmp = LastMap.top();
                    LastMap.pop();
                    delete tmp;
                }

                function<void(bool)> window = [&](bool alive)->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.8;
                    h = height / 4;
                    x = width / 2;
                    y = height / 2;
                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                    ImGui::BeginChild(
                        u8"确认窗口",
                        ImVec2(w, h),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings// |
                        //ImGuiWindowFlags_NoMove
                    );
                    //cout << "ok";
                    //ImGui::SetCursorPos({ width / 2, height / 2 });

                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(0, 170, 0, 255),
                        u8"Accepted!"
                    );
                    ImGui::SetCursorPos({ 0, Screen_Width / 1024 * 24.0f });
                    ImGui::Indent(width / 12);
                    ImGui::Text(encode::string_To_UTF8((string)"罚时：" + to_string(scoreboard->calcScore())).c_str());
                    ImGui::Text(encode::string_To_UTF8((string)"用时：" + sec2time(scoreboard->GetTime())).c_str());
                    //string tip = u8"";
                    ////if (userManager->getName() == userManager->nulluser)tip = u8"你当前未登录账号，无法保存通关记录";

                    //drawText(
                    //    w / 2, h - buttonwidth / kbw * 1.4f - Screen_Width / 1024 * 24.0f / 2 / 2,
                    //    Screen_Width / 1024 * 24.0f / 2,
                    //    ImVec4(255, 0, 0, 255),
                    //    tip
                    //);
                    //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());
                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 4, h - buttonwidth / kbw * 1.3f });
                    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                    {
                        AudioLoader::get().playButton("TAP");
                        if (userManager->getName() != userManager->nulluser) {
                            userManager->getRawData()["User"][userManager->getName()]["COUNT"] = userManager->getRawData()["User"][userManager->getName()]["COUNT"].asInt() + 1;
                        }
                        ////ButtonLock = 0;
                        //hintstate = NONE;
                        //MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

                        //ButtonLock = 1;
                        //swicthFrameR = swicthFrameL = 0;
                        //animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        //    ButtonLock = 0;
                        //    subWindow.pop_back();
                        //    Tab = TAB::GameSelect;
                        //    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                        //        scoreboard->unlock();
                        //        });
                        //    });

                        //////  SAVE YOUR MAP!  一个致命的错误，导致我的几个地图文件坏掉了
                        ////int w, h; LastMap.top()->getSize(w, h);
                        ////for (int i = 1; i <= h; i++)
                        ////    for (int j = 1; j <= w; j++) {
                        ////        Maps[fixIndex]->goalColors[i][j] = MapDisplayer.myMap->colors[i][j];
                        ////    }
                        ////customManager->SaveMap("Map\\"+IDtoPath[Maps[fixIndex]->ID], (CMap*)Maps[fixIndex]);
                        //////  

                        
                        //if (userManager->getName() != userManager->nulluser) {
                        //    userManager->saveHistory(
                        //        MapDisplayer.myMap->getID(),
                        //        scoreboard->GetTime(),
                        //        scoreboard->calcScore()
                        //    );
                        //}
                        subWindow.pop_back();
                    }

                    ImGui::PopStyleColor();
                    ImGui::EndChild();

                    };
                subWindow.push_back({ window,0,0 });
            }
        }
        ImGui::PopStyleColor();
        if (hintstate == SUBMIT)drawRect(
            width * 0.86f - buttonwidth / 2,
            height * 0.4f + 4.5f * buttonwidth / kbw,
            buttonwidth,
            buttonwidth / kbw + 1,
            ImVec4(255, 255, 0, 100)
        );
        /////////////////////////////////////////////////////////////////////////
        ImGui::SetCursorPos({ width - buttonwidth * 2 / kbw - 5.0f,5.0f + ImGui::GetScrollY() });
        //ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"重试", { buttonwidth * 2 / kbw, buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            hintstate = NONE;
            //cout << SelectMap->ans<<"]";
            while (LastMap.size()) {
                Map* tmp = LastMap.top();
                LastMap.pop();
                delete tmp;
            }
            delete GameMap;

            GameMap = new Map(SelectMap);
            MapDisplayer.setMap(GameMap);
            scoreboard->step();//   重试在此时会消耗分数
            //scoreboard->Reset();
        }
        ImGui::PopStyleColor();
    }
    else {

    }
    /////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        function<void(bool)> window = [&](bool alive) ->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.8;
            h = height / 6;
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"确认窗口",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );
            //cout << "ok";
            //ImGui::SetCursorPos({ width / 2, height / 2 });

            drawText(
                w / 2, Screen_Width / 1024 * 24.0f / 2,
                Screen_Width / 1024 * 24.0f / 2,
                ImVec4(255, 255, 255, 255),
                (socketManager->getStatu() == CLIENT)?u8"你确定要退出该房间吗？": u8"你确定要结束本轮游戏吗？"
            );
            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive&& !ButtonLock)
            {
                AudioLoader::get().playButton("TAP");
                if (socketManager->getStatu() == CLIENT) {//    客户端下线，直接退出房间
                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;
                        socketManager->End();
                        AudioLoader::get().playBGM("MAP");
                        Tab = TAB::NetModeSelect;
                        delete GameMap;
                        while (LastMap.size()) {
                            Map* tmp = LastMap.top();
                            LastMap.pop();
                            delete tmp;
                        }
                        socketManager->UserRanking.clear();

                        subWindow.pop_back();
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                    });
                }
                else {//    服务端退出，提前结束本轮游戏
                    Json::Value R;
                    R["type"] = "FINALSTANDING";
                    R["discardThis"] = 1;

                    //if(Rated)
                    socketManager->SplitSender(JSONToEJSON(R));//   广播信息
                    //socketManager->MessageList.push_back(R);//      自己也要有这个信息

                    socketManager->GetOut = 1;
                    //cout << "加入了退出窗口\n";

                    function<void(bool)> window = [&](bool alive)->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.6;
                        h = height / 3.8 * 0.5;//
                        x = width / 2;
                        y = height / 2;
                        ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                        ImGui::BeginChild(
                            u8"finish",
                            ImVec2(w, h),
                            1,
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoSavedSettings// |
                            //ImGuiWindowFlags_NoMove
                        );
                        {
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"本轮比赛已经结束"
                            );
                        }
                        ImGui::EndChild();
                        };
                    ButtonLock = 1;
                    subWindow.clear();//    防止用户故意挂着一些窗口不关
                    subWindow.push_back({ window,0,0 });

                    swicthFrameL = swicthFrameR = 0;
                    animeManager.addAnime(&swicthFrameL, 0, 0, 3000, Linear, 0, [&]() {//     无意义动画，用于延迟执行
                        //cout << "执行1" << swicthFrameR << "\n";
                        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                            //cout << "执行2" << swicthFrameR << "\n";
                            ButtonLock = 0;
                            AudioLoader::get().playBGM("MAP");
                            Tab = TAB::ChatRoom;
                            //delete MapDisplayer.myMap;
                            delete GameMap;
                            while (LastMap.size()) {
                                Map* tmp = LastMap.top();
                                LastMap.pop();
                                delete tmp;
                            }

                            socketManager->GetOut = 0;
                            socketManager->UserRanking.clear();
                            socketManager->ClearWatchList();
                            
                            subWindow.pop_back();
                            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                            });
                        });
                }

                //subWindow.pop_back();
            }
            ImGui::SetCursorPos({ w / 2 + buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
            {
                AudioLoader::get().playButton("TAP");
                subWindow.pop_back();
            }

            ImGui::PopStyleColor();
            ImGui::EndChild();

            };
        subWindow.push_back({ window,0,0 });



        //hintstate = NONE;
        //MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);

        //ButtonLock = 1;
        //swicthFrameR = swicthFrameL = 0;
        //animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
        //    ButtonLock = 0;
        //    Tab = TAB::GameSelect;
        //    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        //    });

        //while (LastMap.size()) {
        //    Map* tmp = LastMap.top();
        //    LastMap.pop();
        //    delete tmp;
        //}
    }
    ImGui::PopStyleColor();

    if (socketManager->kickInfo != Json::nullValue && socketManager->kickInfo["comfirm"].asInt() == 0) {
        //ButtonLock = 0;
        socketManager->kickInfo["comfirm"] = 1;
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            socketManager->End();
            Tab = TAB::NetModeSelect;

            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.8 * 0.5 + buttonwidth / kbw + width / 120;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"你被踢出了该房间"
                    );
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2 * 2 + width / 120,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        encode::string_To_UTF8(socketManager->kickInfo["reason"].asString()).c_str()
                    );
                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive && !ButtonLock)
                    {
                        AudioLoader::get().playButton("TAP");
                        subWindow.pop_back();
                    }
                }
                ImGui::EndChild();
                };
            subWindow.push_back({ window, 0, 0 });

            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    //ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    //ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    //if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    //{
    //    ButtonLock = 1;
    //    swicthFrameR = swicthFrameL = 0;
    //    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
    //        ButtonLock = 0;
    //        Tab = TAB::ChatRoom;
    //        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
    //        });
    //}
    //ImGui::PopStyleColor();

    socketManager->Update();

    if (socketManager->getStatu() == SERVER) {
        if (socketManager->IsAllUserFinished() && !socketManager->GetOut) {
            Json::Value R;
            R["type"] = "FINALSTANDING";
            R["Ranking"] = Json::arrayValue;
            vector<Json::Value> sortRanking;
            for (auto& [ID, S] : socketManager->UserRanking) {
                if (socketManager->IsOnline(socketManager->IDToSocket[ID])) {
                    sortRanking.push_back(S);
                }
            }
            sortRanking.push_back(socketManager->UserRanking[userManager->getID()]);//  服务端不在S_pool里

            sort(sortRanking.begin(), sortRanking.end(), [&](Json::Value a, Json::Value b) {
                if (a["penelty"].asInt() == b["penelty"].asInt()) {
                    return a["time"].asInt() < b["time"].asInt();
                }
                else {
                    return a["penelty"].asInt() < b["penelty"].asInt();
                }
                });

            for (auto& J : sortRanking) {
                R["Ranking"].append(J);
            }

            R["Rated"] = 0;
            R["TIME"] = to_string(time(0));
            R["mode"] = 1;

            //if(Rated)
            socketManager->SplitSender(JSONToEJSON(R));//   广播排行榜信息
            socketManager->MessageList.push_back(R);//      自己也要有这个信息
            socketManager->pagedown = 1;

            socketManager->GetOut = 1;
            socketManager->ClearWatchList();
            scoreboard->lock();
            //cout << "加入了退出窗口\n";

            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"finish",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"本轮比赛已经结束"
                    );
                }
                ImGui::EndChild();
                };
            ButtonLock = 1;
            subWindow.clear();//    防止用户故意挂着一些窗口不关
            subWindow.push_back({ window,0,0 });

            swicthFrameL = swicthFrameR = 0;
            animeManager.addAnime(&swicthFrameL, 0, 0, 3000, Linear, 0, [&]() {//     无意义动画，用于延迟执行
                //cout << "执行1" << swicthFrameR << "\n";
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    //cout << "执行2" << swicthFrameR << "\n";
                    ButtonLock = 0;
                    AudioLoader::get().playBGM("MAP");
                    Tab = TAB::ChatRoom;
                    //delete MapDisplayer.myMap;
                    delete GameMap;
                    while (LastMap.size()) {
                        Map* tmp = LastMap.top();
                        LastMap.pop();
                        delete tmp;
                    }
                    socketManager->GetOut = 0;
                    socketManager->UserRanking.clear();
                    subWindow.pop_back();
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                    });
                });
        }
    }
    else {//    客户端游戏结束的退出行为
        if (socketManager->GetOut == 1) {
            scoreboard->lock();
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"finish",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"本轮比赛已经结束"
                    );
                    //ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    //if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    //{
                    //    subWindow.pop_back();
                    //}
                }
                ImGui::EndChild();
                };
            ButtonLock = 1;
            subWindow.clear();//    防止用户故意挂着一些窗口不关
            subWindow.push_back({ window,0,0 });

            socketManager->GetOut = 0;

            swicthFrameL = swicthFrameR = 0;
            animeManager.addAnime(&swicthFrameL, 0, 0, 3000, Linear, 0, [&]() {//     无意义动画，用于延迟执行
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    ButtonLock = 0;
                    AudioLoader::get().playBGM("MAP");
                    Tab = TAB::ChatRoom;
                    socketManager->UserRanking.clear();
                    //delete MapDisplayer.myMap;
                    delete GameMap;
                    while (LastMap.size()) {
                        Map* tmp = LastMap.top();
                        LastMap.pop();
                        delete tmp;
                    }
                    subWindow.pop_back();
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                    });
                });
        }
    }
}

void EDITORSELECT() {
    drawText(
        width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
        Screen_Width / 1024 * 24.0f,
        ImVec4(255, 255, 255, 255),
        u8"选择生成模式"
    );

    ImGui::SetCursorPos({ width / 2.0f - width * 2 / 5.0f - 5.0f ,height / 2.0f - height * 0.75f / 2 });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    //if (ImGui::Button(u8"  ", { width * 2 / 5.0f, height * 0.75f }) && !ButtonLock&&subWindow.size()==0)
    if (ImageButton(
        u8"随机生成",
        { width * 2 / 5.0f, height * 0.75f },
        imageLoader->IMG["random"].image,
        true,
        { width * 2 / 5.0f ,width * 2 / 5.0f }
    )&&!ButtonLock) {
        AudioLoader::get().playButton("TAP");
        customManager->reset();
        customManager->Random();

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Editor_Random;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ width / 2.0f + 5.0f, height / 2.0f - height * 0.75f / 2 });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    //if (ImGui::Button(u8"    ", { width * 2/ 5.0f, height * 0.75f }) && !ButtonLock&&subWindow.size()==0)
    if (ImageButton(
        u8"自定义生成",
        { width * 2 / 5.0f, height * 0.75f },
        imageLoader->IMG["custom"].image,
        true,
        { width * 2 / 5.0f ,width * 2 / 5.0f }
    )&&!ButtonLock) {
        AudioLoader::get().playButton("TAP");
        drawer.map->setSize(10, 10);
        drawer.cx = width / 3.0f;
        drawer.cy = height / 3.0f;
        drawer.k = 10;
        drawer.size = min(width, height) * 2.0f / 3 * 0.8;

        MapDisplayer.setShowGrid(0);

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Editor_Custom;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ 5.0f,5.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Main;
            AudioLoader::get().playBGM("MAP");
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
            });

        //cout << "喵7";
    }
    ImGui::PopStyleColor();
}

void EDITORRANDOM() {
    //drawText(
            //    width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
            //    Screen_Width / 1024 * 24.0f,
            //    ImVec4(255, 255, 255, 255),
            //    u8"随机生成关卡"
            //);
    ///////////////////////左边的设置///////////////////////
    ImGui::SetCursorPos({ 5.0f,10.0f + buttonwidth / kbw });
    ImGui::BeginChild(
        u8"设定",
        ImVec2(width / 3.8f, height - 20.0f - buttonwidth / kbw),
        1,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings// |
        //ImGuiWindowFlags_NoMove
    );
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        ImGui::SliderInt(u8"宽度", &customManager->w, 3, MAX_WIDTH);
        ImGui::SliderInt(u8"高度", &customManager->h, 3, MAX_HEIGHT);
        ImGui::SliderInt(u8"最小数字", &customManager->maxstep, 1, 400);
        ImGui::SliderInt(u8"最大数字", &customManager->minstep, 2, 400);

        ImGui::PushFont(Font);
        ImGui::BulletText(u8"色盘");
        ImGui::PopFont();
        ImGui::Separator();
        for (int index = 0; index < customManager->palette.size(); index++) {
            ImGui::BeginChild(
                (encode::wideToUtf8(L"色盘" + to_wstring(index + 1))).c_str(),
                ImVec2(width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );
            //ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]);

            ImGui::SameLine();
            if (ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]) && !ButtonLock && subWindow.size() == 0)
            {
                AudioLoader::get().playButton("TAP");
                selectIndex = index;
                col[0] = customManager->palette[index].x;
                col[1] = customManager->palette[index].y;
                col[2] = customManager->palette[index].z;
                //ButtonLock = 1;
                function<void(bool)> window = [&](bool alive)->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.8;
                    h = width / 2.8 + buttonwidth / kbw;
                    x = width / 2;
                    y = height / 2;
                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                    ImGui::BeginChild(
                        u8"选择颜色",
                        ImVec2(w, h),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings// |
                        //ImGuiWindowFlags_NoMove
                    );
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"选择颜色"
                    );
                    ImGui::ColorPicker3(u8"测试", col);
                    ImVec2 Pos = ImGui::GetCursorPos();
                    ImGui::SetCursorPosX(Pos.x + w / 2 - buttonwidth / kbw);
                    if (ImGui::Button(u8"确定", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        customManager->palette[selectIndex] = ImVec4(col[0], col[1], col[2], 1);
                        subWindow.pop_back();
                        //ButtonLock = 0;
                    }
                    ImGui::SetCursorPosX(Pos.x);

                    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                    ImGui::EndChild();
                    };
                subWindow.push_back({ window,0,0 });
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"X") && !ButtonLock && subWindow.size() == 0)
            {
                AudioLoader::get().playButton("TAP");
                removeIndex.push_back(index);
            }
            if (ButtonLock || (subWindow.size() > 0))drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
            ImGui::EndChild();
        }

        if (ImGui::Button(u8"+", { width / 3.8f - 10.0f,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            customManager->palette.push_back(ImVec4(rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, 1));
        }
        if (ButtonLock || (subWindow.size() > 0))drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
        for (auto it = removeIndex.rbegin(); it != removeIndex.rend(); it++) {
            if (customManager->palette.size() <= 2) {
                break;
            }
            customManager->palette.erase(customManager->palette.begin() + *it);
        }
        removeIndex.clear();
        //ImGui::ColorPicker3(u8"测试", col);
        //ImGui::Text(to_string(col[0]).c_str());
        //ImGui::Text(to_string(col[1]).c_str());
        //ImGui::Text(to_string(col[2]).c_str());

        ImGui::PopStyleColor();
        if (ButtonLock || (subWindow.size() > 0))drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
    }

    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
    ///////////////////////中间的展示///////////////////////
    ImGui::SetCursorPos({ 5.0f + width / 3.8f,10.0f + buttonwidth / kbw });
    ImGui::BeginChild(
        u8"展示",
        ImVec2(width - width / 3.8f - 10.0f, height - 20.0f - buttonwidth / kbw),
        1,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings// |
        //ImGuiWindowFlags_NoMove
    );
    {
        w_ = width - width / 3.8f - 10.0f;
        h_ = height - 20.0f - buttonwidth / kbw;
        MapDisplayer.setSize(w_ * 0.5 * 0.9);
        MapDisplayer.setMap(customManager->getBegin());
        MapDisplayer.setCenter(w_ * 0.25, h_ * 0.5);
        MapDisplayer.display();
        MapDisplayer.setMap(customManager->getEnd());
        MapDisplayer.setCenter(w_ * 0.75, h_ * 0.5);
        MapDisplayer.display(true);

        ImGui::SetCursorPos({ w_ * 0.5f - buttonwidth - 5.0f, h_ * 0.5f + w_ * 0.25f });
        if (ImGui::Button(u8"重新生成", { buttonwidth,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            if (userManager->getRawData()["noColorCheck"] == Json::nullValue) {
                userManager->getRawData()["noColorCheck"] = 0;
            }
            if (customManager->checkPalette() || userManager->getRawData()["noColorCheck"].asInt() == 1) {
                customManager->Random();
            }
            else {
                function<void(bool)> window = [&](bool alive)->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    static char levelname[16];
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.6 + width / 120 * 6;
                    h = height / 3.8 * 0.5;//
                    x = width / 2;
                    y = height / 2;
                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                    ImGui::BeginChild(
                        u8"error",
                        ImVec2(w, h),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings// |
                        //ImGuiWindowFlags_NoMove
                    );
                    {
                        drawText(
                            w / 2, Screen_Width / 1024 * 24.0f / 2,
                            Screen_Width / 1024 * 24.0f / 2,
                            ImVec4(255, 255, 255, 255),
                            u8"生成失败，请检查色盘中是否有过于接近的颜色"
                        );
                        ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                        {
                            AudioLoader::get().playButton("TAP");
                            subWindow.pop_back();
                        }
                    }

                    ImGui::EndChild();
                };
                subWindow.push_back({ window,0,0 });
            }
        }
        ImGui::SetCursorPos({ w_ * 0.5f + 5.0f, h_ * 0.5f + w_ * 0.25f });
        if (ImGui::Button(u8"保存关卡", { buttonwidth,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            savedMapName = to_string(time(0));
            customManager->SaveMap("Map/R" + savedMapName + ".dat", "R" + savedMapName, stoi(savedMapName));
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"保存成功 ",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    encode::string_To_UTF8("关卡已保存为：R" + savedMapName).c_str()
                );

                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    Maps.push_back(new Map("R" + savedMapName + ".dat"));
                    IDtoPath[(*Maps.rbegin())->getID()] = "R" + savedMapName + ".dat";
                    refreshMapIndex();
                    
                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;
                        Tab = TAB::EditorSelect;
                        subWindow.pop_back();
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0 ,[&]() {
                            customManager->reset();
                        });
                    });
                    //subwindow = nullptr;
                    
                    //ButtonLock = 0;
                }

                ImGui::EndChild();
                };
            subWindow.push_back({ window,0,0 });
            //ButtonLock = 1;
        }
        if (ButtonLock || (subWindow.size() > 0))drawRect(0, 0, width - width / 3.8f - 10.0f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
    }

    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
    /////////////////////经典左上角返回//////////////////////
    ImGui::SetCursorPos({ 5.0f,5.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        

        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::EditorSelect;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                customManager->reset();
            });
        });
    }
    ImGui::PopStyleColor();
}

void EDITORCUSTOM() {
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    static bool isScale = 0;//导入图片后用户是否量化了图片
    //////////////////////中央绘图区 与 主绘图逻辑/////////////////////////
    w_ = width * 2 / 3.0f;
    h_ = height * 2 / 3.0f;
    ImGui::SetCursorPos({ width / 2.0f - w_ / 2.0f,height / 2.0f - h_ / 2.0f });
    ImGui::BeginChild(
        u8"绘图区",
        { w_ * 1.0f,h_ * 1.0f },
        1,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );
    {
        MapDisplayer.setMap(drawer.map);
        MapDisplayer.setSize(drawer.size * drawer.k / 10.0);
        MapDisplayer.setCenter(drawer.cx, drawer.cy);
        MapDisplayer.display(true);

        drawer.map->getSize(W, H);
        ImGui::SetCursorPos({ drawer.cx - drawer.size / max(W,H) * W * drawer.k / 10.0f / 2,drawer.cy - drawer.size / max(W,H) * H * drawer.k / 10.0f / 2 });
        //  处理拖动
        if (PaintBoard(u8"按钮", { drawer.size / max(W,H) * W * drawer.k / 10.0f,drawer.size / max(W,H) * H * drawer.k / 10.0f }) && drawer.getState() != MOVE && !ButtonLock&&subWindow.size()==0) {
            //std::cout << 33;
            //cout << ImVec2(
            //    int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))),
            //    int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H)))
            //) << "\n";
        }
        if (drawer.getState() == MOVE && ImGui::IsItemActivated() && !ButtonLock&&subWindow.size()==0) {
            last_mouse_hit_pos = GetMousePos_WithRestrict(w_, h_, 0, 0);
        }
        if (drawer.getState() == MOVE && ImGui::IsItemActive() && !ButtonLock&&subWindow.size()==0) {
            //cout << GetMousePos_InWindow();
            drawer.cx += (GetMousePos_WithRestrict(w_, h_, 0, 0) - last_mouse_hit_pos).x;
            drawer.cy += (GetMousePos_WithRestrict(w_, h_, 0, 0) - last_mouse_hit_pos).y;
            last_mouse_hit_pos = GetMousePos_WithRestrict(w_, h_, 0, 0);
        }
        if (ImGui::IsItemHovered() && io.MouseWheel != 0 && !ButtonLock&&subWindow.size()==0) {
            if (drawer.k + io.MouseWheel >= 1 && drawer.k + io.MouseWheel <= 30) {
                float dw__ = GetMousePos_WithRestrict(w_, h_, 0, 0).x - (ImGui::GetWindowPos().x) - drawer.cx, dh__ = GetMousePos_WithRestrict(w_, h_, 0, 0).y - (ImGui::GetWindowPos().y) - drawer.cy;
                float dw_ = dw__ / drawer.k * (drawer.k + io.MouseWheel), dh_ = dh__ / drawer.k * (drawer.k + io.MouseWheel);
                drawer.k += io.MouseWheel;
                drawer.cx -= (dw_ - dw__);
                drawer.cy -= (dh_ - dh__);
            }
            //std::cout << io.MouseWheel << "\n";
        }

        //  处理不同状态下执行的功能
        if (drawer.getState() == DRAWDOT) {
            if (drawer.getIndex() != -1) {
                if (ImGui::IsItemActivated()) {
                    drawer.saveCurrentStep();
                }

                if (ImGui::IsItemActive()) {
                    drawer.draw(
                        int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                        int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                        drawer.getIndex()
                    );
                }

                if (ImGui::IsItemDeactivated()) {
                    if (drawer.getVaild()) {
                        drawer.save();
                        drawer.clearNextStep();
                        drawer.resetVaild();
                    }
                }
            }
        }

        if (drawer.getState() == ERASE) {
            if (ImGui::IsItemActivated()) {
                drawer.saveCurrentStep();
            }
            if (ImGui::IsItemActive()) {
                drawer.draw(
                    int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                    int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                    -1
                );
            }
            if (ImGui::IsItemDeactivated()) {
                if (drawer.getVaild()) {
                    drawer.save();
                    drawer.clearNextStep();
                    drawer.resetVaild();
                }
            }
        }

        if (drawer.getState() == LINE) {
            if (drawer.getIndex() != -1) {
                if (ImGui::IsItemActivated()) {
                    drawer.setBeginPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                }
                if (ImGui::IsItemActive()) {
                    drawer.setEndPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });

                    for (ImVec2 pos : prettyLine(drawer.getBeginPos(), drawer.getEndPos())) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                }
                if (ImGui::IsItemDeactivated()) {
                    for (ImVec2 pos : prettyLine(drawer.getBeginPos(), drawer.getEndPos())) {// 不加这段，在画线时会闪烁
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    drawer.saveCurrentStep();
                    drawer.line(drawer.getIndex());
                    if (drawer.getVaild()) {
                        drawer.save();
                        drawer.clearNextStep();
                        drawer.resetVaild();
                    }
                }
            }
        }

        if (drawer.getState() == SQURA) {
            if (drawer.getIndex() != -1) {
                if (ImGui::IsItemActivated()) {
                    drawer.setBeginPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                }
                if (ImGui::IsItemActive()) {
                    drawer.setEndPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                    float l, r, u, d;
                    l = MIN(drawer.getBeginPos().x, drawer.getEndPos().x);
                    r = MAX(drawer.getBeginPos().x, drawer.getEndPos().x);
                    u = MIN(drawer.getBeginPos().y, drawer.getEndPos().y);
                    d = MAX(drawer.getBeginPos().y, drawer.getEndPos().y);
                    for (ImVec2 pos : prettyLine({ l,u }, { r,u })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    for (ImVec2 pos : prettyLine({ l,d }, { r,d })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    for (ImVec2 pos : prettyLine({ l,u }, { l,d })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    for (ImVec2 pos : prettyLine({ r,u }, { r,d })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                }
                if (ImGui::IsItemDeactivated()) {
                    float l, r, u, d;
                    l = MIN(drawer.getBeginPos().x, drawer.getEndPos().x);
                    r = MAX(drawer.getBeginPos().x, drawer.getEndPos().x);
                    u = MIN(drawer.getBeginPos().y, drawer.getEndPos().y);
                    d = MAX(drawer.getBeginPos().y, drawer.getEndPos().y);
                    for (ImVec2 pos : prettyLine({ l,u }, { r,u })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    for (ImVec2 pos : prettyLine({ l,d }, { r,d })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    for (ImVec2 pos : prettyLine({ l,u }, { l,d })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    for (ImVec2 pos : prettyLine({ r,u }, { r,d })) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    drawer.saveCurrentStep();
                    drawer.squra(drawer.getIndex());
                    if (drawer.getVaild()) {
                        drawer.save();
                        drawer.clearNextStep();
                        drawer.resetVaild();
                    }
                }
            }
        }

        if (drawer.getState() == SQURAF) {
            if (drawer.getIndex() != -1) {
                if (ImGui::IsItemActivated()) {
                    drawer.setBeginPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                }
                if (ImGui::IsItemActive()) {
                    drawer.setEndPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                    int l, r, u, d;
                    l = MIN(drawer.getBeginPos().x, drawer.getEndPos().x);
                    r = MAX(drawer.getBeginPos().x, drawer.getEndPos().x);
                    u = MIN(drawer.getBeginPos().y, drawer.getEndPos().y);
                    d = MAX(drawer.getBeginPos().y, drawer.getEndPos().y);
                    for (int x = l; x <= r; x++)
                        for (int y = u; y <= d; y++) {
                            MapDisplayer.showUnitColor(x, y, drawer.palette[drawer.getIndex()]);
                        }

                }
                if (ImGui::IsItemDeactivated()) {
                    int l, r, u, d;
                    l = MIN(drawer.getBeginPos().x, drawer.getEndPos().x);
                    r = MAX(drawer.getBeginPos().x, drawer.getEndPos().x);
                    u = MIN(drawer.getBeginPos().y, drawer.getEndPos().y);
                    d = MAX(drawer.getBeginPos().y, drawer.getEndPos().y);
                    for (int x = l; x <= r; x++)
                        for (int y = u; y <= d; y++) {
                            MapDisplayer.showUnitColor(x, y, drawer.palette[drawer.getIndex()]);
                        }
                    drawer.saveCurrentStep();
                    drawer.squraf(drawer.getIndex());
                    if (drawer.getVaild()) {
                        drawer.save();
                        drawer.clearNextStep();
                        drawer.resetVaild();
                    }
                }
            }
        }

        if (drawer.getState() == CIRC) {
            if (drawer.getIndex() != -1) {
                if (ImGui::IsItemActivated()) {
                    drawer.setBeginPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                }
                if (ImGui::IsItemActive()) {
                    drawer.setEndPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });

                    for (ImVec2 pos : prettyCirc(drawer.getBeginPos(), drawer.getEndPos())) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                }
                if (ImGui::IsItemDeactivated()) {
                    for (ImVec2 pos : prettyCirc(drawer.getBeginPos(), drawer.getEndPos())) {// 不加这段，在画线时会闪烁
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    drawer.saveCurrentStep();
                    drawer.circ(drawer.getIndex());
                    if (drawer.getVaild()) {
                        drawer.save();
                        drawer.clearNextStep();
                        drawer.resetVaild();
                    }
                }
            }
        }

        if (drawer.getState() == CIRCF) {
            if (drawer.getIndex() != -1) {
                if (ImGui::IsItemActivated()) {
                    drawer.setBeginPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });
                }
                if (ImGui::IsItemActive()) {
                    drawer.setEndPos({
                            constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                            constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                        });

                    for (ImVec2 pos : prettyCircF(drawer.getBeginPos(), drawer.getEndPos())) {
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                }
                if (ImGui::IsItemDeactivated()) {
                    for (ImVec2 pos : prettyCircF(drawer.getBeginPos(), drawer.getEndPos())) {// 不加这段，在画线时会闪烁
                        MapDisplayer.showUnitColor(pos.x, pos.y, drawer.palette[drawer.getIndex()]);
                    }
                    drawer.saveCurrentStep();
                    drawer.circf(drawer.getIndex());
                    if (drawer.getVaild()) {
                        drawer.save();
                        drawer.clearNextStep();
                        drawer.resetVaild();
                    }
                }
            }
        }

        if (drawer.getState() == FILL) {
            if (ImGui::IsItemActivated()) {
                drawer.saveCurrentStep();

                drawer.fill(
                    int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                    int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                    drawer.getIndex()
                );
            }
            if (ImGui::IsItemDeactivated()) {
                if (drawer.getVaild()) {
                    drawer.save();
                    drawer.clearNextStep();
                    drawer.resetVaild();
                }
            }
        }

        if (drawer.getState() == SELECTOR) {
            if (ImGui::IsItemActive()) {
                drawer.setIndex(
                    drawer.map->getColors_index(
                        (int)constrict(1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, W),
                        (int)constrict(1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1, 1, H)
                    )
                );
            }
        }

        //  绘制选择框
        if (ImGui::IsItemHovered() && drawer.getState() != MOVE && !ButtonLock&&subWindow.size()==0) {
            MapDisplayer.setGridColor(ImVec4(0, 0, 0, 1));
            MapDisplayer.showUnitBoard(
                int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1
            );
            MapDisplayer.setGridColor(ImVec4(0.5, 0.5, 0.5, 1));
        }

        if (ButtonLock||(subWindow.size()>0))drawRect(0, 0, w_, h_, ImVec4(0, 0, 0, 50));
    }

    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
    ///////////////////色盘/////////////////////////
    w_ = width / 6.0f;
    h_ = height * 2 / 3.0f;
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    ImGui::SetCursorPos({ width / 2.0f + width / 3.0f,height / 2.0f - h_ / 2.0f });
    ImGui::BeginChild(
        u8"色盘区",
        ImVec2(w_ * 1.0, h_ * 1.0),
        1,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings// |
        //ImGuiWindowFlags_NoMove
    );
    {
        ImGui::PushFont(Font);
        ImGui::BulletText(u8"色盘");
        ImGui::PopFont();
        ImGui::Separator();
        for (int index = 0; index < drawer.palette.size(); index++) {
            ImGui::BeginChild(
                (encode::wideToUtf8(L"色盘" + to_wstring(index + 1))).c_str(),
                ImVec2(w_ * 1.0, buttonwidth / kbw),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse
            );
            //ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]);

            if (drawer.getIndex() == index) {
                drawRect(0, 0, w_ * 1.0 + 10.0, buttonwidth / kbw, ImVec4(0.3, 0.3, 0.3, 1));
            }

            ImGui::SetCursorPos({ 5.0,5.0 });
            //ImGui::SameLine();
            if (ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), drawer.palette[index]) && !ButtonLock&&subWindow.size()==0)
            {
                AudioLoader::get().playButton("TAP");
                selectIndex = index;
                col[0] = drawer.palette[index].x;
                col[1] = drawer.palette[index].y;
                col[2] = drawer.palette[index].z;
                //ButtonLock = 1;
                function<void(bool)> window = [&](bool alive)->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.8;
                    h = width / 2.8 + buttonwidth / kbw;
                    x = width / 2;
                    y = height / 2;
                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                    ImGui::BeginChild(
                        u8"选择颜色",
                        ImVec2(w, h),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings// |
                        //ImGuiWindowFlags_NoMove
                    );
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"选择颜色"
                    );
                    ImGui::ColorPicker3(u8"测试", col);
                    ImVec2 Pos = ImGui::GetCursorPos();
                    ImGui::SetCursorPosX(Pos.x + w / 2 - buttonwidth / kbw);
                    if (ImGui::Button(u8"确定", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        drawer.saveCurrentStep();
                        drawer.palette[selectIndex] = ImVec4(col[0], col[1], col[2], 1);
                        drawer.map->setPalette(&drawer.palette);
                        drawer.save();
                        drawer.clearNextStep();
                        subWindow.pop_back();
                        //ButtonLock = 0;
                    }
                    ImGui::SetCursorPosX(Pos.x);
                    ImGui::EndChild();
                    };
                subWindow.push_back({ window,0,0 });
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"X") && !ButtonLock&&subWindow.size()==0)
            {
                AudioLoader::get().playButton("TAP");
                drawer.saveCurrentStep();
                removeIndex.push_back(index);
            }

            ImGui::SetCursorPos({ 0,0 });

            if (PaintBoard(encode::wideToUtf8(L"c" + to_wstring(index)).c_str(), ImVec2(w_ * 1.0, buttonwidth / kbw))) {
                AudioLoader::get().playButton("TAP");
                drawer.setIndex(index);
            }

            if (ButtonLock||(subWindow.size()>0))drawRect(0, 0, w_ * 1.0, buttonwidth / kbw, ImVec4(0, 0, 0, 50));
            ImGui::EndChild();
        }

        if (ImGui::Button(u8"+", { w_ * 1.0f,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
        {
            AudioLoader::get().playButton("TAP");
            drawer.saveCurrentStep();
            drawer.palette.push_back(ImVec4(rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, 1));
            drawer.map->setPalette(&drawer.palette);
            drawer.save();
            drawer.clearNextStep();
        }
        if (ButtonLock||(subWindow.size()>0))drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
        for (auto it = removeIndex.rbegin(); it != removeIndex.rend(); it++) {
            if (drawer.palette.size() <= 0) {
                break;
            }
            //
            drawer.palette.erase(drawer.palette.begin() + *it);
            drawer.map->setPalette(&drawer.palette);
            drawer.colorsShift(*it);
            drawer.removeInvaildColors();

            if (drawer.getIndex() == *it) {
                drawer.setIndex(-1);
            }
        }
        if (removeIndex.size()) {
            drawer.save();
            drawer.clearNextStep();
        }
        removeIndex.clear();
        //ImGui::ColorPicker3(u8"测试", col);
        //ImGui::Text(to_string(col[0]).c_str());
        //ImGui::Text(to_string(col[1]).c_str());
        //ImGui::Text(to_string(col[2]).c_str());

        ImGui::PopStyleColor();
        if (ButtonLock||(subWindow.size()>0))drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
    }

    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
    ///////////////drawer状态切换///////////////
    w_ = width / 6.0f;
    h_ = height * 2 / 3.0f;
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    ImGui::SetCursorPos({ 0,height / 2.0f - h_ / 2.0f });
    ImGui::BeginChild(
        u8"工具集",
        ImVec2(w_ * 1.0, h_ * 1.0),
        1,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings// |
        //ImGuiWindowFlags_NoMove
    );
    {
        ImGui::PushFont(Font);
        ImGui::BulletText(u8"工具");
        ImGui::PopFont();
        ImGui::Separator();

        //ImGui::Image(imageLoader->IMG["drawpix"].image, ImVec2(64, 64));
        ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == DRAWDOT ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == DRAWDOT ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == DRAWDOT ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            u8"绘制",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["drawpix"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            drawer.setState(DRAWDOT);
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == ERASE ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == ERASE ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == ERASE ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            u8"擦除",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["eraser"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            drawer.setState(ERASE);
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        //==============子工具集切换===============
        ImGui::PushStyleColor(ImGuiCol_Button, (drawer.getState() == LINE || drawer.getState() == SQURA || drawer.getState() == SQURAF || drawer.getState() == CIRC || drawer.getState() == CIRCF) ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (drawer.getState() == LINE || drawer.getState() == SQURA || drawer.getState() == SQURAF || drawer.getState() == CIRC || drawer.getState() == CIRCF) ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (drawer.getState() == LINE || drawer.getState() == SQURA || drawer.getState() == SQURAF || drawer.getState() == CIRC || drawer.getState() == CIRCF) ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            (drawer.getSubState() == LINE ? u8"工具:绘制直线" : (drawer.getSubState() == SQURA ? u8"工具:绘制矩形" : (drawer.getSubState() == SQURAF ? u8"工具:绘制填充矩形" : (drawer.getSubState() == CIRC ? u8"工具:绘制圆形" : u8"工具:绘制填充圆形")))),
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG[(drawer.getSubState() == LINE ? "line" : (drawer.getSubState() == SQURA ? "squra" : (drawer.getSubState() == SQURAF ? "squraf" : (drawer.getSubState() == CIRC ? "circ" : "circf"))))].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");

            if (drawer.getState() == LINE ||
                drawer.getState() == SQURA ||
                drawer.getState() == SQURAF ||
                drawer.getState() == CIRC ||
                drawer.getState() == CIRCF) {
                drawer.displaySubToolBar ^= 1;
            }
            else {
                drawer.setState(drawer.getSubState());
            }
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        if (drawer.displaySubToolBar) {
            cursor = ImGui::GetCursorPos();
            ImGui::SameLine();
            ImGui::BeginChild(
                u8"子工具集",
                ImVec2(buttonwidth / kbw * 1.4f, buttonwidth / kbw * 1.2f * 5),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );
            {
                ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == LINE ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == LINE ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == LINE ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
                if (ImageButton(
                    u8"绘制直线",
                    { buttonwidth / kbw ,buttonwidth / kbw },
                    imageLoader->IMG["line"].image,
                    true,
                    { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
                ) && subWindow.empty()) {
                    AudioLoader::get().playButton("TAP");
                    drawer.displaySubToolBar ^= 1;
                    drawer.setState(LINE);
                    drawer.setSubState(LINE);
                }
                ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == SQURA ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == SQURA ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == SQURA ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
                if (ImageButton(
                    u8"绘制矩形",
                    { buttonwidth / kbw ,buttonwidth / kbw },
                    imageLoader->IMG["squra"].image,
                    true,
                    { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
                ) && subWindow.empty()) {
                    AudioLoader::get().playButton("TAP");
                    drawer.displaySubToolBar ^= 1;
                    drawer.setState(SQURA);
                    drawer.setSubState(SQURA);
                }
                ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == SQURAF ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == SQURAF ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == SQURAF ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
                if (ImageButton(
                    u8"绘制填充矩形",
                    { buttonwidth / kbw ,buttonwidth / kbw },
                    imageLoader->IMG["squraf"].image,
                    true,
                    { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
                ) && subWindow.empty()) {
                    AudioLoader::get().playButton("TAP");
                    drawer.displaySubToolBar ^= 1;
                    drawer.setState(SQURAF);
                    drawer.setSubState(SQURAF);
                }
                ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == CIRC ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == CIRC ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == CIRC ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
                if (ImageButton(
                    u8"绘制圆形",
                    { buttonwidth / kbw ,buttonwidth / kbw },
                    imageLoader->IMG["circ"].image,
                    true,
                    { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
                ) && subWindow.empty()) {
                    AudioLoader::get().playButton("TAP");
                    drawer.displaySubToolBar ^= 1;
                    drawer.setState(CIRC);
                    drawer.setSubState(CIRC);
                }
                ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == CIRCF ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == CIRCF ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == CIRCF ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
                if (ImageButton(
                    u8"绘制填充圆形",
                    { buttonwidth / kbw ,buttonwidth / kbw },
                    imageLoader->IMG["circf"].image,
                    true,
                    { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
                )&& subWindow.empty()) {
                    AudioLoader::get().playButton("TAP");
                    drawer.displaySubToolBar ^= 1;
                    drawer.setState(CIRCF);
                    drawer.setSubState(CIRCF);
                }
                ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();
            }

            drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
            if(subWindow.size())drawRect(0, 0, width, height, ImColor(0, 0, 0, 200));
            ImGui::EndChild();
            ImGui::SetCursorPos(cursor);
        }
        //=============================

        ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == FILL ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == FILL ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == FILL ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            u8"填充",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["fill"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            drawer.setState(FILL);
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == SELECTOR ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == SELECTOR ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == SELECTOR ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            u8"取色器",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["colorpicker"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            drawer.setState(SELECTOR);
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == MOVE ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == MOVE ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == MOVE ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            u8"移动",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["move"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            drawer.setState(MOVE);
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        if (ImageButton(
            u8"画布居中",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["center"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            drawer.cx = width / 3.0f;
            drawer.cy = height / 3.0f;
        }

        ImGui::PushStyleColor(ImGuiCol_Button, MapDisplayer.isShowGrid() ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MapDisplayer.isShowGrid() ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, MapDisplayer.isShowGrid() ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
        if (ImageButton(
            MapDisplayer.isShowGrid() ? u8"隐藏网格" : u8"显示网格",
            { buttonwidth / kbw ,buttonwidth / kbw },
            imageLoader->IMG["grid"].image,
            true,
            { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
        ) && subWindow.empty()) {
            AudioLoader::get().playButton("TAP");
            MapDisplayer.setShowGrid(!MapDisplayer.isShowGrid());
        }
        ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

        ImGui::PopStyleColor();
        if (ButtonLock||(subWindow.size()>0))drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
    }

    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
    ///////////////////////设置/////////////////////////
    static int X, Y, P = 5;//  临时变量，供滑块使用
    ImGui::SetCursorPos({ 5.0f,height - buttonwidth / kbw - 10.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"设置", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        //subwindow;
        SettingWindow = [&](bool alive) ->void {
            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
            float w, h, x, y;//长宽，中心坐标
            w = width / 2.8;
            h = Screen_Width / 1024 * 24.0f + buttonwidth / kbw * 4 + 20;
            x = width / 2;
            y = height / 2;
            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
            ImGui::BeginChild(
                u8"设置   ",
                ImVec2(w, h),
                1,
                ImGuiWindowFlags_NoResize //|
                //ImGuiWindowFlags_NoSavedSettings
            );
            {
                //drawRect(0, 0, w, h, ImVec4(1, 0, 0, 50));//
                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    u8"设置"
                );
                /////////////////////////////////////////////////////////////////////////
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2,ImGui::GetCursorPosY() });
                if (ImGui::Button(u8"返回", { buttonwidth ,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    subWindow.pop_back();//
                    //ButtonLock = 0;
                    //cout << "取消";
                }
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2,ImGui::GetCursorPosY() });
                if (ImGui::Button(u8"设置画布大小", { buttonwidth ,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    drawer.map->getSize(X, Y);
                    function<void(bool)> window = [&](bool alive)->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.8;
                        h = Screen_Width / 1024 * 24.0f + buttonwidth / kbw * 4 + 20;
                        x = width / 2;
                        y = height / 2;
                        ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                        ImGui::BeginChild(
                            u8"子设置   ",
                            ImVec2(w, h),
                            1,
                            ImGuiWindowFlags_NoResize //|
                            //ImGuiWindowFlags_NoSavedSettings
                        );
                        {
                            //drawRect(0, 0, w, h, ImVec4(1, 0, 0, 50));//
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"设置新的大小"
                            );
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2, ImGui::GetCursorPosY() });
                            ImGui::SetNextItemWidth(buttonwidth);
                            ImGui::SliderInt(u8"宽度", &X, 3, MAX_WIDTH);
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2, ImGui::GetCursorPosY() });
                            ImGui::SetNextItemWidth(buttonwidth);
                            ImGui::SliderInt(u8"高度", &Y, 3, MAX_HEIGHT);
                            drawText(
                                w / 2, ImGui::GetCursorPosY() + 10.0f,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 0, 0, 255),
                                u8"注意，这会重置你当前的画布"
                            );
                            //ImGui::TextColored(ImVec4(255, 0, 0, 255), u8"注意，这将同时重置你的画布");
                            /////////////////////////////////////////////////////////////////////////
                            float cursorY = ImGui::GetCursorPosY();
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - 5.0f,cursorY });
                            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                            {
                                AudioLoader::get().playButton("TAP");
                                drawer.clearMemory();
                                drawer.map->setSize(X, Y);
                                drawer.palette.clear();
                                drawer.setIndex(-1);

                                drawer.cx = width / 3.0f;
                                drawer.cy = height / 3.0f;

                                subWindow.clear();
                                //ButtonLock = 0;
                            }
                            ImGui::SetCursorPos({ w / 2 + 5.0f,cursorY });
                            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                            {
                                AudioLoader::get().playButton("TAP");
                                subWindow.pop_back();//
                                //ButtonLock = 0;
                                //cout << "返回父设置";
                            }
                        }
                        ImGui::EndChild();
                        };
                    subWindow.push_back({ window,0,0 });
                    //ButtonLock = 0;
                    //cout << "进入子设置";
                }
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2,ImGui::GetCursorPosY() });
                if (ImGui::Button(u8"清空画布", { buttonwidth, buttonwidth / kbw }))
                {
                    AudioLoader::get().playButton("TAP");
                    function<void(bool)> window = [&](bool alive)->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.8;
                        h = Screen_Width / 1024 * 24.0f + buttonwidth / kbw + 20;
                        x = width / 2;
                        y = height / 2;
                        ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                        ImGui::BeginChild(
                            u8"子设置   ",
                            ImVec2(w, h),
                            1,
                            ImGuiWindowFlags_NoResize //|
                            //ImGuiWindowFlags_NoSavedSettings
                        );
                        {
                            //drawRect(0, 0, w, h, ImVec4(1, 0, 0, 50));//
                            drawText(
                                w / 2, Screen_Width / 1024 * 24.0f / 2,
                                Screen_Width / 1024 * 24.0f / 2,
                                ImVec4(255, 255, 255, 255),
                                u8"你确定要清空画布吗？"
                            );
                            /////////////////////////////////////////////////////////////////////////
                            float cursorY = ImGui::GetCursorPosY();
                            ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - 5.0f,cursorY });
                            if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                            {
                                AudioLoader::get().playButton("TAP");
                                drawer.clearMemory();
                                int w, h;
                                drawer.map->getSize(w, h);
                                drawer.map->setSize(w, h);
                                drawer.palette.clear();
                                drawer.setIndex(-1);

                                drawer.cx = width / 3.0f;
                                drawer.cy = height / 3.0f;

                                subWindow.clear();
                                //ButtonLock = 0;
                            }
                            ImGui::SetCursorPos({ w / 2 + 5.0f,cursorY });
                            if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                            {
                                AudioLoader::get().playButton("TAP");
                                subWindow.pop_back();//
                                //ButtonLock = 0;
                                //cout << "返回父设置";
                            }
                        }
                        ImGui::EndChild();
                    };//
                    //ButtonLock = 0;
                    subWindow.push_back({ window ,0,0 });
                    //cout << "进入子设置";
                }
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2,ImGui::GetCursorPosY() });
                if (ImGui::Button(u8"导入图像", { buttonwidth, buttonwidth / kbw }))
                {
                    AudioLoader::get().playButton("TAP");
                    ifd::FileDialog::Instance().Open("TextureOpenDialog", u8"选择一张图片", "Image file (*.png;*.jpg;*.jpeg;*.bmp;*.tga){.png,.jpg,.jpeg,.bmp,.tga}");
                    subWindow.pop_back();//
                    //ButtonLock = 0;
                    X = 10, Y = 10, P = 5;
                    //cout << "先关了窗口再说";
                }
            }
            ImGui::EndChild();
        };
        subWindow.push_back({ SettingWindow,0,0 });
        //ButtonLock = 1;
        //cout << "喵7";
        //cout << "没写";
        //ifd::FileDialog::Instance().Open("TextureOpenDialog", u8"测试用", "Image file (*.png;*.jpg;*.jpeg;*.bmp;*.tga){.png,.jpg,.jpeg,.bmp,.tga},.*");
    }
    ImGui::PopStyleColor();
    //////////////////////设置 - 导入图片///////////////////
    if (ifd::FileDialog::Instance().IsDone("TextureOpenDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string res = ifd::FileDialog::Instance().GetResult().u8string();
            printf("OPEN[%s]\n", encode::UTF8_To_String(res).c_str());
            
            if (imageLoader->IMG.count("OPEN")) {
                imageLoader->remove("OPEN");
            }
            imageLoader->LoadImageToTexture("OPEN", encode::UTF8_To_String(res).c_str());

            customManager->palette.clear();
            isScale = 0;
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width * 0.75;
                h = height * 0.75;
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"量化设置",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize //|
                    //ImGuiWindowFlags_NoSavedSettings
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"图像量化参数设置"
                    );
                    //  左边展示的原图
                    int w_ = imageLoader->IMG["OPEN"].w, h_ = imageLoader->IMG["OPEN"].h;
                    float k = w * 2 / 5 * 0.8f / MAX(w_, h_);
                    ImGui::SetCursorPos({ w / 5 - k * w_ / 2, w / 5 + 10.0f - k * h_ / 2 });
                    ImGui::Image(imageLoader->IMG["OPEN"].image, { k * w_,k * h_ });


                    ImGui::GetWindowDrawList()->AddLine(
                        { ImGui::GetWindowPos().x + w * 2 / 5,ImGui::GetWindowPos().y + Screen_Width / 1024 * 24.0f + 5.0f },
                        { ImGui::GetWindowPos().x + w * 2 / 5,ImGui::GetWindowPos().y + h - buttonwidth / kbw - 20.0f }, 
                        ImColor(100, 100, 100, 255)
                    );
                    //  中间展示的量化后的图
                    
                    if (imageLoader->IMG.count("OPEN_PIX")) {
                        //int w_ = imageLoader->IMG["OPEN_PIX"].w, h_ = imageLoader->IMG["OPEN_PIX"].h;
                        //float k = w * 2 / 5 * 0.8f / MAX(w_, h_);
                        //ImGui::SetCursorPos({ w * 3 / 5 - k * w_ / 2, w / 5 + 10.0f - k * h_ / 2 });
                        //ImGui::Image(imageLoader->IMG["OPEN_PIX"].image, { k * w_,k * h_ });
                        MapDisplayer.setMap(customManager->getBegin());//
                        MapDisplayer.setCenter(w * 3 / 5, w / 5 + 10.0f);
                        MapDisplayer.setSize(w * 2 / 5 * 0.8f);
                        MapDisplayer.display(true);
                        //cout << "!";
                    }

                    if (alive) {
                        ImGui::SetCursorPos({ w * 3 / 5 - (w * 2 / 5 * 0.9f) / 2, w * 2 / 5 });
                        ImGui::SetNextItemWidth((w * 2 / 5) * 0.9 - 2 * ImGui::CalcTextSize("宽度", NULL, true).x);
                        ImGui::SliderInt(u8"宽度", &X, 3, MAX_WIDTH);
                        ImGui::SetCursorPos({ w * 3 / 5 - (w * 2 / 5 * 0.9f) / 2, ImGui::GetCursorPosY() });
                        ImGui::SetNextItemWidth((w * 2 / 5) * 0.9 - 2 * ImGui::CalcTextSize("宽度", NULL, true).x);
                        ImGui::SliderInt(u8"高度", &Y, 3, MAX_HEIGHT);
                        ImGui::SetCursorPos({ w * 3 / 5 - (w * 2 / 5 * 0.9f) / 2, ImGui::GetCursorPosY() });
                        ImGui::SetNextItemWidth((w * 2 / 5) * 0.9 - 2 * ImGui::CalcTextSize("色盘大小", NULL, true).x);
                        ImGui::SliderInt(u8"色盘大小", &P, 2, 20);
                    }

                    ImGui::SetCursorPos({ w * 3 / 5 - buttonwidth / 2 - 5.0f, ImGui::GetCursorPosY() });
                    if (ImGui::Button(u8"直接量化", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        
                        function<void(bool)> window = [&](bool alive)->void {
                            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                            float w, h, x, y;//长宽，中心坐标
                            w = width / 2.6;
                            h = height / 3.8 * 0.5;//
                            x = width / 2;
                            y = height / 2;
                            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                            ImGui::BeginChild(
                                u8"finish",
                                ImVec2(w, h),
                                1,
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings// |
                                //ImGuiWindowFlags_NoMove
                            );
                            {
                                stringstream ss;
                                ss << setprecision(2) << fixed << imageLoader->percent;//   显示图像缩放进程
                                drawText(
                                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                                    Screen_Width / 1024 * 24.0f / 2,
                                    ImVec4(255, 255, 255, 255),
                                    u8"正在量化图像中，请耐心等待(" + ss.str() + u8"%%)"
                                );
                                drawRect(
                                    0, Screen_Width / 1024 * 24.0f, imageLoader->percent / 100 * w, buttonwidth/kbw,
                                    ImVec4(0,1,0,1)
                                );
                            }
                            ImGui::EndChild();
                            };
                        

                        std::thread t([&]() {   //  开支线程，防止用户窗口卡死
                            imageLoader->ScaleImage("OPEN", "OPEN_PIX", X, Y);//    图像缩放
                            customManager->LoadMapFromImage(imageLoader->IMG["OPEN_PIX"]);//    将图像存到地图里
                            customManager->palette = kMeans(customManager->getBegin(), P);//    对该地图进行Kmeans
                            isScale = 1;
                            subWindow.pop_back();
                            });
                        t.detach();
                        
                        subWindow.push_back({ window,0,0 });
                        //cout << 123;

                        //subwindow = nullptr;
                        //ButtonLock = 0;
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPos({ w * 3 / 5 + 5.0f, ImGui::GetCursorPosY() });
                    if (ImGui::Button(u8"按初始色量化", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");

                        function<void(bool)> window = [&](bool alive)->void {
                            drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                            float w, h, x, y;//长宽，中心坐标
                            w = width / 2.6;
                            h = height / 3.8 * 0.5;//
                            x = width / 2;
                            y = height / 2;
                            ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                            ImGui::BeginChild(
                                u8"finish",
                                ImVec2(w, h),
                                1,
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings// |
                                //ImGuiWindowFlags_NoMove
                            );
                            {
                                stringstream ss;
                                ss << setprecision(2) << fixed << imageLoader->percent;
                                drawText(
                                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                                    Screen_Width / 1024 * 24.0f / 2,
                                    ImVec4(255, 255, 255, 255),
                                    u8"正在量化图像中，请耐心等待(" + ss.str() + u8"%%)"
                                );
                                drawRect(
                                    0, Screen_Width / 1024 * 24.0f, imageLoader->percent / 100 * w, buttonwidth / kbw,
                                    ImVec4(0, 1, 0, 1)
                                );
                            }
                            ImGui::EndChild();
                            };

                        std::thread t([&]() {
                            imageLoader->ScaleImage("OPEN", "OPEN_PIX", X, Y);
                            customManager->LoadMapFromImage(imageLoader->IMG["OPEN_PIX"]);
                            customManager->palette = kMeans(customManager->getBegin(), P, customManager->palette);
                            isScale = 1;
                            subWindow.pop_back();
                            });
                        t.detach();

                        subWindow.push_back({ window,0,0 });

                        //subwindow = SettingWindow;//
                        //ButtonLock = 0;
                        //cout << "返回父设置";
                    }

                    ImGui::GetWindowDrawList()->AddLine(
                        { ImGui::GetWindowPos().x + w * 4 / 5,ImGui::GetWindowPos().y + Screen_Width / 1024 * 24.0f + 5.0f },
                        { ImGui::GetWindowPos().x + w * 4 / 5,ImGui::GetWindowPos().y + h - buttonwidth / kbw - 20.0f },
                        ImColor(100, 100, 100, 255)
                    );
                    ///////////////////////展示色盘/////////////////////////
                    ImGui::SetCursorPos({ w * 4 / 5, Screen_Width / 1024 * 24.0f + 5.0f });
                    w_ = w / 5;
                    h_ = h - buttonwidth / kbw - 20.0f - (Screen_Width / 1024 * 24.0f + 5.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                    //ImGui::SetCursorPos({ width / 2.0f + width / 3.0f,height / 2.0f - h_ / 2.0f });
                    ImGui::BeginChild(
                        u8"色盘区",
                        ImVec2(w_ * 1.0, h_ * 1.0),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings 
                    );
                    {
                        ImGui::PushFont(Font);
                        ImGui::BulletText(u8"色盘");
                        ImGui::PopFont();
                        ImGui::Separator();
                        for (int index = 0; index < customManager->palette.size(); index++) {
                            ImGui::BeginChild(
                                (encode::wideToUtf8(L"色盘" + to_wstring(index + 1))).c_str(),
                                ImVec2(w_ * 1.0, buttonwidth / kbw),
                                1,
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoSavedSettings |
                                ImGuiWindowFlags_NoScrollbar |
                                ImGuiWindowFlags_NoScrollWithMouse
                            );

                            ImGui::SetCursorPos({ 5.0,5.0 });
                            //ImGui::SameLine();
                            if (ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]) && alive)
                            {
                                AudioLoader::get().playButton("TAP");
                                selectIndex = index;
                                col[0] = customManager->palette[index].x;
                                col[1] = customManager->palette[index].y;
                                col[2] = customManager->palette[index].z;
                                //ButtonLock = 1;
                                function<void(bool)> window = [&](bool alive)->void {
                                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                                    float w, h, x, y;//长宽，中心坐标
                                    w = width / 2.8;
                                    h = width / 2.8 + buttonwidth / kbw;
                                    x = width / 2;
                                    y = height / 2;
                                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                                    ImGui::BeginChild(
                                        u8"选择颜色",
                                        ImVec2(w, h),
                                        1,
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoSavedSettings// |
                                        //ImGuiWindowFlags_NoMove
                                    );
                                    drawText(
                                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                                        Screen_Width / 1024 * 24.0f / 2,
                                        ImVec4(255, 255, 255, 255),
                                        u8"选择颜色"
                                    );
                                    ImGui::ColorPicker3(u8"测试", col);
                                    ImVec2 Pos = ImGui::GetCursorPos();
                                    ImGui::SetCursorPosX(Pos.x + w / 2 - buttonwidth / kbw);
                                    if (ImGui::Button(u8"确定", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && alive)
                                    {
                                        AudioLoader::get().playButton("TAP");
                                        customManager->palette[selectIndex] = ImVec4(col[0], col[1], col[2], 1);
                                        //subwindow = nullptr;
                                        subWindow.pop_back();
                                        //ButtonLock = 0;
                                    }
                                    ImGui::SetCursorPosX(Pos.x);
                                    ImGui::EndChild();
                                    };
                                subWindow.push_back({ window ,0,0 });
                            }
                            ImGui::SameLine();
                            if (ImGui::Button(u8"X") && alive)
                            {
                                AudioLoader::get().playButton("TAP");
                                removeIndex.push_back(index);
                            }

                            ImGui::SetCursorPos({ 0,0 });

                            if (ButtonLock||!alive)drawRect(0, 0, w_ * 1.0, buttonwidth / kbw, ImVec4(0, 0, 0, 50));

                            drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                            ImGui::EndChild();
                        }

                        if (ImGui::Button(u8"+", { w_ * 1.0f,buttonwidth / kbw }) && alive)
                        {
                            AudioLoader::get().playButton("TAP");
                            customManager->palette.push_back(ImVec4(rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, 1));
                        }
                        if (ButtonLock||!alive)drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
                        for (auto it = removeIndex.rbegin(); it != removeIndex.rend(); it++) {
                            if (customManager->palette.size() <= 0) {
                                break;
                            }
                            //
                            customManager->palette.erase(customManager->palette.begin() + *it);
                        }
                        removeIndex.clear();
                        //ImGui::ColorPicker3(u8"测试", col);
                        //ImGui::Text(to_string(col[0]).c_str());
                        //ImGui::Text(to_string(col[1]).c_str());
                        //ImGui::Text(to_string(col[2]).c_str());

                        ImGui::PopStyleColor();
                        if (ButtonLock||!alive)drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
                    }

                    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                    ImGui::EndChild();


                    ///////////////////////////////////底部确认与取消//////////////////////////////////////
                    //float cursorY = ImGui::GetCursorPosY();
                    ImGui::SetCursorPos({ 5.0f,h - buttonwidth / kbw - 10.0f });
                    if (ImGui::Button(u8"重新选择", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        //customManager->reset();
                        customManager->palette.clear();
                        imageLoader->remove("OPEN_PIX");
                        subWindow.clear();//其实pop完就空了，所以不如clear()，保险一点
                        ifd::FileDialog::Instance().Open("TextureOpenDialog", u8"选择一张图片", "Image file (*.png;*.jpg;*.jpeg;*.bmp;*.tga){.png,.jpg,.jpeg,.bmp,.tga}");
                    }

                    ImGui::SetCursorPos({ w - 2*(buttonwidth / 2 + 5.0f),h - buttonwidth / kbw - 10.0f });
                    if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        customManager->palette.clear();
                        imageLoader->remove("OPEN_PIX");
                        subWindow.clear();//其实pop完就空了，所以不如clear()，保险一点
                        subWindow.push_back({ SettingWindow ,0,0 });
                        //ButtonLock = 0;
                        //cout << "返回父设置";
                    }
                    ImGui::SetCursorPos({ w - buttonwidth / 2 - 5.0f,h-buttonwidth / kbw-10.0f });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        //cout << 123;
                        if (isScale) {
                            drawer.palette = customManager->palette;
                            drawer.clearMemory();
                            delete drawer.map;
                            drawer.map = new CMap(customManager->getBegin());

                            customManager->palette.clear();
                            imageLoader->remove("OPEN_PIX");
                            subWindow.clear();
                        }
                        else {
                            function<void(bool)> window = [&](bool alive)->void {
                                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                                static char levelname[16];
                                float w, h, x, y;//长宽，中心坐标
                                w = width / 2.8;
                                h = height / 3.8 * 0.5;//
                                x = width / 2;
                                y = height / 2;
                                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                                ImGui::BeginChild(
                                    u8"error",
                                    ImVec2(w, h),
                                    1,
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoSavedSettings// |
                                    //ImGuiWindowFlags_NoMove
                                );
                                {
                                    drawText(
                                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                                        Screen_Width / 1024 * 24.0f / 2,
                                        ImVec4(255, 255, 255, 255),
                                        u8"请先进行量化操作"
                                    );
                                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                                    {
                                        AudioLoader::get().playButton("TAP");
                                        subWindow.pop_back();
                                    }
                                }

                                ImGui::EndChild();
                            };
                            subWindow.push_back({ window,1,0 });
                        }
                        //ButtonLock = 0;
                    }

                    if (ButtonLock || !alive)drawRect(0, 0, width * 0.75, height * 0.75, ImVec4(0, 0, 0, 50));
                }

                drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                ImGui::EndChild();
                
                };
            //ButtonLock = 1;
            subWindow.push_back({ window,1,0 });

            //cout << imageLoader->IMG["OPEN"].w << " " << imageLoader->IMG["OPEN"].h << "}\n";
        }
        else {
            subWindow.push_back({ SettingWindow ,0,0 });
            //ButtonLock = 1;
        }
        ifd::FileDialog::Instance().Close();
    }
    /////////////////////边角的按钮/////////////////////
    ImGui::SetCursorPos({ width - 2 * buttonwidth / kbw - 10.0f,height - buttonwidth / kbw - 15.0f });
    if (drawer.canBack() && ImageButton(
        u8"上一步",
        { buttonwidth / kbw ,buttonwidth / kbw },
        imageLoader->IMG["back"].image,
        true,
        { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
    ) && subWindow.empty()) {
        AudioLoader::get().playButton("TAP");
        drawer.retreat();
    }

    ImGui::SetCursorPos({ width - buttonwidth / kbw - 5.0f,height - buttonwidth / kbw - 15.0f });
    if (drawer.canNext() && ImageButton(
        u8"下一步",
        { buttonwidth / kbw ,buttonwidth / kbw },
        imageLoader->IMG["forward"].image,
        true,
        { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
    ) && subWindow.empty()) {
        AudioLoader::get().playButton("TAP");
        drawer.reretreat();
    }

    ImGui::SetCursorPos({ width - buttonwidth * 2 / kbw - 5.0f,5.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"生成", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        //cout << "没写";//
        //  记得写个特判，判断地图是否合法//
        drawer.map->getSize(W, H);
        //drawer.map->setPalette(&drawer.palette);
        customManager->palette = drawer.palette;
        customManager->w = W;
        customManager->h = H;
        customManager->setEnd(drawer.map);
        int error = customManager->checkMap();
        if (error == 0) {
            if (userManager->getRawData()["noColorCheck"] == Json::nullValue) {
                userManager->getRawData()["noColorCheck"] = 0;
            }
            if (userManager->getRawData()["noColorCheck"].asInt() == 1 || customManager->checkPalette()) {
                customManager->R_Random();
                MapDisplayer.setShowGrid(false);

                ButtonLock = 1;
                swicthFrameR = swicthFrameL = 0;
                animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                    ButtonLock = 0;
                    Tab = TAB::Editor_Generator;
                    animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
                });
            }
            else {
                function<void(bool)> window = [&](bool alive)->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    static char levelname[16];
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.6 + width/120*6;
                    h = height / 3.8 * 0.5;//
                    x = width / 2;
                    y = height / 2;
                    ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                    ImGui::BeginChild(
                        u8"error",
                        ImVec2(w, h),
                        1,
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings// |
                        //ImGuiWindowFlags_NoMove
                    );
                    {
                        drawText(
                            w / 2, Screen_Width / 1024 * 24.0f / 2,
                            Screen_Width / 1024 * 24.0f / 2,
                            ImVec4(255, 255, 255, 255),
                            u8"生成失败，请检查色盘中是否有过于接近的颜色"
                        );
                        ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                        {
                            AudioLoader::get().playButton("TAP");
                            subWindow.pop_back();
                        }
                    }

                    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                    ImGui::EndChild();
                    };
                subWindow.push_back({ window,0,0 });
            }
        }
        else if (error == -1) {
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8 + width / 120 * 6;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"无法生成，所提供图像中未找到连续的色块"
                    );

                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        subWindow.pop_back();
                    }
                }
                ImGui::EndChild();
            };
            subWindow.push_back({ window,0,0 });
        }
        else if (error == -2) {
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"无法生成，所提供色盘太小，至少应为 2"
                    );

                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        subWindow.pop_back();
                    }
                }
                ImGui::EndChild();
                };
            subWindow.push_back({ window,0,0 });
        }
        else {
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"error",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                {
                    drawText(
                        w / 2, Screen_Width / 1024 * 24.0f / 2,
                        Screen_Width / 1024 * 24.0f / 2,
                        ImVec4(255, 255, 255, 255),
                        u8"无法生成，所提供图像中存在透明色块"
                    );
                    ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                    {
                        AudioLoader::get().playButton("TAP");
                        subWindow.pop_back();
                    }
                }
                ImGui::EndChild();
                };
            subWindow.push_back({ window,0,0 });
        }
    }
    ImGui::PopStyleColor();
    
    ImGui::SetCursorPos({ 5.0f,5.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        drawer.clearMemory();
        drawer.palette.clear();
        drawer.setIndex(-1);

        MapDisplayer.setShowGrid(false);

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::EditorSelect;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                customManager->reset();
            });
        });
    }
    ImGui::PopStyleColor();
}

void EDITORGENERATOR() {
    ///////////////////////中间的展示///////////////////////
    ImGui::SetCursorPos({ 5.0f ,10.0f + buttonwidth / kbw });
    ImGui::BeginChild(
        u8"展示",
        ImVec2(width-10.0f, height - 20.0f - buttonwidth / kbw),
        1,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings// |
        //ImGuiWindowFlags_NoMove
    );
    {
        w_ = width - 10.0f;
        h_ = height - 20.0f - buttonwidth / kbw;
        MapDisplayer.setSize(w_ * 0.5 * 0.9);
        MapDisplayer.setMap(customManager->getBegin());
        MapDisplayer.setCenter(w_ * 0.25, h_ * 0.5 - buttonwidth / kbw);
        MapDisplayer.display();

        MapDisplayer.setMap(customManager->getEnd());
        MapDisplayer.setCenter(w_ * 0.75, h_ * 0.5 - buttonwidth / kbw);
        MapDisplayer.display(true);

        ImGui::SetCursorPos({ w_ * 0.25f - buttonwidth / 2, h_ * 0.5f + w_ * 0.5f * 0.9f / 2 + 5.0f - buttonwidth / kbw });
        ImGui::SetNextItemWidth(buttonwidth);
        ImGui::SliderInt(u8"最大期望步数", &customManager->step, 2, 399);

        ImGui::SetCursorPos({ w_ * 0.5f - buttonwidth - 5.0f, h_ * 0.5f + w_ * 0.25f });
        if (ImGui::Button(u8"重新生成", { buttonwidth,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            customManager->R_Random();
        }
        ImGui::SetCursorPos({ w_ * 0.5f + 5.0f, h_ * 0.5f + w_ * 0.25f });
        if (ImGui::Button(u8"保存关卡", { buttonwidth,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
        {
            AudioLoader::get().playButton("TAP");
            savedMapName = to_string(time(0));
            customManager->SaveMap("Map/C" + savedMapName + ".dat", "C" + savedMapName, stoi(savedMapName));
            function<void(bool)> window = [&](bool alive)->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char levelname[16];
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 3.8 * 0.5;//
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"保存成功 ",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    encode::string_To_UTF8("关卡已保存为：C" + savedMapName).c_str()
                );

                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 / 2,h - 10.0f - buttonwidth / kbw });
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    Maps.push_back(new Map("C" + savedMapName + ".dat"));
                    IDtoPath[(*Maps.rbegin())->getID()] = "C" + savedMapName + ".dat";
                    refreshMapIndex();
                    
                    //delete customManager->getBegin();
                    //customManager->getBegin() = nullptr;
                    //customManager->R_Random();
                    drawer.clearMemory();
                    drawer.palette.clear();
                    drawer.setIndex(-1);

                    ButtonLock = 1;
                    swicthFrameR = swicthFrameL = 0;
                    animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
                        ButtonLock = 0;
                        Tab = TAB::EditorSelect;
                        subWindow.pop_back();
                        animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                            customManager->reset();
                        });
                    });
                    //subwindow = nullptr;
                    //ButtonLock = 0;
                }

                drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
                ImGui::EndChild();
                };
            subWindow.push_back({ window,0,0 });
            //ButtonLock = 1;
        }
        if (ButtonLock || (subWindow.size() > 0))drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
    }

    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
    /////////////////////边角按钮//////////////////////
    ImGui::SetCursorPos({ 5.0f,5.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回修改", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        

        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Editor_Custom;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                customManager->reset();
            });
        });

    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({ width - buttonwidth * 2 / kbw - 5.0f, 5.0f });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        
        AudioLoader::get().playButton("TAP");
        //delete customManager->getBegin();
        //customManager->getBegin() = nullptr;
        //customManager->R_Random();
        drawer.clearMemory();
        drawer.palette.clear();
        drawer.setIndex(-1);

        MapDisplayer.setShowGrid(false);

        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::EditorSelect;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine, 0, [&]() {
                customManager->reset();
            });
        });
    }
    ImGui::PopStyleColor();
}

void ABORT() {
    ImGui::BeginChild(u8"关于 ", { width,height },0,ImGuiWindowFlags_AlwaysVerticalScrollbar);
    drawText(
        width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
        Screen_Width / 1024 * 24.0f,
        ImVec4(255, 255, 255, 255),
        u8"关于"
    );
    ImGui::Indent(width / 5);
    drawText_autoNewLine(
        width / 5,
        Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50 + 5 * width / 120,
        Screen_Width / 1024 * 15.0f, 
        ImVec4(1, 1, 1, 1),
        encode::string_To_UTF8(R"(·基本玩法
    在燃烧的画中，你需要通过旋转，燃烧的操作，将一副画烧成目标图像。
    当你进行“顺时针旋转”操作时，原图像将顺时针旋转
    当你进行“逆时针旋转”操作时，原图像将逆时针旋转
    当你进行燃烧操作时，所有数字大于一的格子都将向下平移一格，同时数字减一。当格子平移出屏幕时，不能再平移回来。

·Rating模式
    Rating模式下，你有五分钟的时间完成尽可能多的关卡，关卡难度随着通过的关卡数而增加。玩家最终Rating变化将由玩家原本的Rating以及本轮通过的关卡数计算。

·联机模式
    在联机模式中，你可以和好友一同竞速，或是右键地图列表，一键分享自己创作的地图。（注意，只能在同一个局域网环境下（连接同一个Wifi或者热点），通过输入IP的形式进行联机，不同局域网无法联机）

·关卡编辑器
    你可以在关卡编辑器中创作属于你自己的关卡，有两种模式可选，其中“随机生成模式”中，会根据你设定的地图参数和色盘，随机生成地图。
    “自定义生成模式”中则可以自由创作或是导入一张图像，由该图像及其色盘随机生成一张地图。（注意，在自定义生成模式下，你需要点击右侧色盘中，你需要使用的颜色所在格子，才能选中该颜色）

)"),
        width * 4 / 5,u8" ",0);
    ImGui::Separator();

    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock && subWindow.size() == 0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Main;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
            });

    }
    ImGui::PopStyleColor();
    //界面切换用的FRAMEMASK
    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::EndChild();
}

void SETTINGS() {
    //
    drawText(
        width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
        Screen_Width / 1024 * 24.0f,
        ImVec4(255, 255, 255, 255),
        u8"设置"
    );
    ImGui::Indent(50 + buttonwidth * 2 / kbw);
    ImGui::SetCursorPos({ 50 + buttonwidth * 2 / kbw, 50 + buttonwidth / kbw });
    if (userManager->getName() != "未登录") {
        if (userManager->getName() != "未登录" &&
            (userManager->getRawData()["User"][userManager->getName()]["avator"] == Json::nullValue ||
                !IDtoMaps.count(userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()))
            ) {
            userManager->getRawData()["User"][userManager->getName()]["avator"] = -1;
        }
        MapDisplayer.setMap((userManager->getName() == "未登录" || userManager->getRawData()["User"][userManager->getName()]["avator"] == -1) ?
            defaultAvator :
            Maps[IDtoMaps[userManager->getRawData()["User"][userManager->getName()]["avator"].asInt()]]
        );//refreshMapIndex()

        MapDisplayer.setCenter(50 + buttonwidth * 2 / kbw + (1.0f * width / 4) / 2 + width/120, 50 + buttonwidth / kbw + (1.0f * width / 4) / 2);
        MapDisplayer.setSize(1.0f * width / 4);
        MapDisplayer.displayFinish();

        if (userManager->getRawData()["User"][userManager->getName()]["COUNT"] == Json::nullValue) {
            userManager->getRawData()["User"][userManager->getName()]["COUNT"] = 0;
        }
        drawText(
            50 + buttonwidth * 2 / kbw + (1.0f * width / 4) + width / 120 * 2,
            50 + buttonwidth / kbw + (1.0f * width / 4) / 2,
            Screen_Width / 1024 * 16.0f,
            ImVec4(1, 1, 1, 1),
            encode::string_To_UTF8("总通关局次：") + to_string(userManager->getRawData()["User"][userManager->getName()]["COUNT"].asInt()),
            0
        );

        if (userManager->getRawData()["User"][userManager->getName()]["Rating"] == Json::nullValue) {
            userManager->getRawData()["User"][userManager->getName()]["Rating"] = 0.0f;
        }
        stringstream ss;
        ss << setprecision(2) << fixed << userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat();
        drawText(
            50 + buttonwidth * 2 / kbw + (1.0f * width / 4) + width / 120 * 2,
            50 + buttonwidth / kbw + (1.0f * width / 4) / 2 + width/120 + Screen_Width / 1024 * 16.0f,
            Screen_Width / 1024 * 16.0f,
            ImVec4(1, 1, 1, 1),
            encode::string_To_UTF8("Rating：") + ss.str(),
            0
        );

        ImGui::SetCursorPos({ 50 + buttonwidth * 2 / kbw, 50 + buttonwidth / kbw + 1.0f * width / 4 + width / 120 });
    }
    ImGui::PushFont(Font);
    ImGui::Text((string(u8" 当前账号：") + encode::string_To_UTF8(userManager->getName())).c_str());//
    ImGui::PopFont();

    if (userManager->getName() == userManager->nulluser) {
        //ImGui::SetCursorPos({ width - (50 + buttonwidth), 50 + buttonwidth / kbw });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"登录", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
        {
            AudioLoader::get().playButton("TAP");
            //Tab = TAB::Main;

            function<void(bool)> window = [&](bool alive) ->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char Username[16], Password[21];
                static bool showPassword = 0;
                static string tip = "";
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 3.8;
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"登录账号",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //cout << "ok";
                //ImGui::SetCursorPos({ width / 2, height / 2 });

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    u8"登录信息"
                );
                ImGui::SetCursorPos({ w / 10, Screen_Width / 1024 * 24.0f });
                ImGui::Indent(w / 10);
                ImGui::SetNextWindowSize(ImVec2(width / 4, 10));
                if (ImGui::InputText(
                    u8"账号名",
                    //u8"1到15个字符",
                    Username, IM_ARRAYSIZE(Username)
                )) {
                    //ImGui::GetInputTextState("账号名");
                    //cout << encode::UTF8_To_String(Username) << "]";
                }
                ImGui::SetNextWindowSize(ImVec2(width / 4, 10));
                if (ImGui::InputText(
                    u8"密码",
                    //u8"6到20个字符",
                    Password, IM_ARRAYSIZE(Password),
                    ImGuiInputTextFlags_Password * (!showPassword)
                )) {
                    //ImGui::GetInputTextState("账号名");
                    //cout << encode::UTF8_To_String(Username) << "]";
                }
                ImGui::SameLine();
                ImGui::Checkbox(u8" ", &showPassword);
                /////////////////////////////////////////////////////////////////////////
                ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());// 错误提示信息
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    //subwindow = nullptr;
                    //ButtonLock = 0;
                    if (userManager->Login(encode::UTF8_To_String(Username), encode::UTF8_To_String(Password))) {
                        subWindow.pop_back();
                        //ButtonLock = 0;
                        tip = u8"";
                        showPassword = 0;
                        memset(Username, '\0', sizeof(Username));
                        memset(Password, '\0', sizeof(Password));
                        cerr << "登录成功";
                    }
                    else {
                        tip = u8"用户名或密码有误";
                        cerr << "登录失败";
                    }
                    //cout << encode::UTF8_To_String(Username) << "]";
                    //cout << "确认";
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    subWindow.pop_back();
                    //ButtonLock = 0;
                    tip = u8"";
                    showPassword = 0;
                    memset(Username, '\0', sizeof(Username));
                    memset(Password, '\0', sizeof(Password));
                    //cout << "取消";
                }

                ImGui::PopStyleColor();
                ImGui::EndChild();

                };
            //ButtonLock = 1;
            subWindow.push_back({ window,0,0 });
            //cout << "喵7";
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"注册", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
        {
            AudioLoader::get().playButton("TAP");
            //Tab = TAB::Main;
            function<void(bool)> window = [&](bool alive) ->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                static char Username[16], Password[21], ComfirmPassword[21];
                static bool showPassword = 0;
                static string tip = "";
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.6;
                h = height / 3.2;
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"注册账号",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //cout << "ok";
                //ImGui::SetCursorPos({ width / 2, height / 2 });

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    u8"账号信息"
                );
                ImGui::SetCursorPos({ w / 10, Screen_Width / 1024 * 24.0f });
                ImGui::Indent(w / 10);
                ImGui::SetNextWindowSize(ImVec2(width / 4, 10));
                if (ImGui::InputTextWithHint(
                    u8"账号名",
                    u8"1到15个字符",
                    Username, IM_ARRAYSIZE(Username)
                )) {
                    //ImGui::GetInputTextState("账号名");
                    //cout << encode::UTF8_To_String(Username) << "]";
                }
                ImGui::SetNextWindowSize(ImVec2(width / 4, 10));
                if (ImGui::InputTextWithHint(
                    u8"密码",
                    u8"6到20个字符",
                    Password, IM_ARRAYSIZE(Password),
                    ImGuiInputTextFlags_Password * (!showPassword)
                )) {
                    //ImGui::GetInputTextState("账号名");
                    //cout << encode::UTF8_To_String(Username) << "]";
                }
                ImGui::SameLine();
                ImGui::Checkbox(u8" ", &showPassword);

                ImGui::SetNextWindowSize(ImVec2(width / 4, 10));
                if (ImGui::InputTextWithHint(
                    u8"确认密码",
                    u8"",
                    ComfirmPassword, IM_ARRAYSIZE(ComfirmPassword),
                    ImGuiInputTextFlags_Password
                )) {
                    //ImGui::GetInputTextState("账号名");
                    //cout << encode::UTF8_To_String(Username) << "]";
                }
                /////////////////////////////////////////////////////////////////////////
                ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());// 错误提示信息
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    //subwindow = nullptr;
                    //ButtonLock = 0;
                    int statu;
                    if (encode::UTF8_To_String(Password) != encode::UTF8_To_String(ComfirmPassword))statu = -3;
                    else statu = userManager->Register(encode::UTF8_To_String(Username), encode::UTF8_To_String(Password));
                    if (statu == 1) {
                        subWindow.pop_back();
                        //ButtonLock = 0;
                        tip = u8"";
                        showPassword = 0;
                        memset(Username, '\0', sizeof(Username));
                        memset(Password, '\0', sizeof(Password));
                        memset(ComfirmPassword, '\0', sizeof(ComfirmPassword));
                        cerr << "注册成功";
                    }
                    else if (statu == -3) {
                        tip = u8"两次密码输入不一致";
                        cerr << "注册失败";
                    }
                    else if (statu == -2) {
                        tip = u8"该用户名已被占用";
                        cerr << "注册失败";
                    }
                    else if (statu == -1) {
                        tip = u8"名字格式不符合规范";
                        cerr << "注册失败";
                    }
                    else if (statu == 0) {
                        tip = u8"密码格式不符合规范";
                        cerr << "注册失败";
                    }
                    else {
                        tip = u8"未知错误";
                        cerr << "注册失败";
                    }
                    //cout << encode::UTF8_To_String(Username) << "]";
                    //cout << "确认";
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    subWindow.pop_back();
                    //ButtonLock = 0;
                    tip = u8"";
                    showPassword = 0;
                    memset(Username, '\0', sizeof(Username));
                    memset(Password, '\0', sizeof(Password));
                    memset(ComfirmPassword, '\0', sizeof(ComfirmPassword));
                    //cout << "取消";
                }

                ImGui::PopStyleColor();
                ImGui::EndChild();

                };
            //ButtonLock = 1;
            subWindow.push_back({ window,0,0 });
            //cout << "喵8";
        }
        ImGui::PopStyleColor();
    }
    else {
        //ImGui::SetCursorPos({ width - (50 + buttonwidth), 50 + buttonwidth / kbw });
        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
        if (ImGui::Button(u8"退出登录", { buttonwidth, buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
        {
            AudioLoader::get().playButton("TAP");
            //Tab = TAB::Main;
            userManager->Quit();
            //cout << "退出登录";
        }

        //ImGui::SetCursorPos({ width - (50 + buttonwidth), 50 + buttonwidth / kbw * 2 });
        if (ImGui::Button(u8"重置存档", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
        {
            AudioLoader::get().playButton("TAP");
            //Tab = TAB::Main;

            function<void(bool)> window = [&](bool alive) ->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 6;
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"确认窗口",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //cout << "ok";
                //ImGui::SetCursorPos({ width / 2, height / 2 });

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 255, 255, 255),
                    u8"你确定要清空当前账号的所有记录吗？"
                );
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    subWindow.pop_back();
                    //ButtonLock = 0;
                    userManager->ClearHistory();
                    //cout << encode::UTF8_To_String(Username) << "]";
                    //cout << "已清空";
                }
                ImGui::SetCursorPos({ w / 2 + buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
                if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    subWindow.pop_back();
                    //ButtonLock = 0;
                    //cout << "取消";
                }

                ImGui::PopStyleColor();
                ImGui::EndChild();

                };
            //ButtonLock = 1;
            subWindow.push_back({ window,0,0 });
            //cout << "喵7";
        }
        ImGui::SameLine();
        ImGui::SetCursorPos({ 50 + buttonwidth * 2 / kbw + buttonwidth / 2 + width/120, ImGui::GetCursorPosY()});
        //ImGui::SetCursorPos({ width - (50 + buttonwidth / 2), 50 + buttonwidth / kbw * 2 });
        if (ImGui::Button(u8"删除账号", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
        {
            AudioLoader::get().playButton("TAP");
            //Tab = TAB::Main;

            function<void(bool)> window = [&](bool alive) ->void {
                drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                float w, h, x, y;//长宽，中心坐标
                w = width / 2.8;
                h = height / 6;
                x = width / 2;
                y = height / 2;
                ImGui::SetCursorPos({ x - w / 2, y - h / 2 });
                ImGui::BeginChild(
                    u8"确认窗口",
                    ImVec2(w, h),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //cout << "ok";
                //ImGui::SetCursorPos({ width / 2, height / 2 });

                drawText(
                    w / 2, Screen_Width / 1024 * 24.0f / 2,
                    Screen_Width / 1024 * 24.0f / 2,
                    ImVec4(255, 0, 0, 255),
                    u8"你确定要删除当前账号吗？"
                );
                ImGui::SetCursorPos({ w / 2 - buttonwidth / 2 - buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    subWindow.pop_back();
                    //ButtonLock = 0;
                    userManager->DestoryUser();
                    //cout << encode::UTF8_To_String(Username) << "]";
                    //cout << "已删除";
                }
                ImGui::SetCursorPos({ w / 2 + buttonwidth / kbw * 0.02f, h - buttonwidth / kbw * 1.3f });
                if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }) && alive)
                {
                    AudioLoader::get().playButton("TAP");
                    //Tab = TAB::Main;
                    subWindow.pop_back();
                    //ButtonLock = 0;
                    //cout << "取消";
                }

                ImGui::PopStyleColor();
                ImGui::EndChild();

                };
            //ButtonLock = 1;
            subWindow.push_back({ window,0,0 });
            //cout << "喵8";
        }

        ImGui::PopStyleColor();
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////
    ImGui::Separator();
    if (userManager->getRawData()["FX Volume"] == Json::nullValue) {
        userManager->getRawData()["FX Volume"] = 130;
        userManager->getRawData()["BGM Volume"] = 220;
        userManager->getRawData()["noColorCheck"] = 0;
    }

    static int FX = userManager->getRawData()["FX Volume"].asInt();
    static int BGM = userManager->getRawData()["BGM Volume"].asInt();
    static bool colorCheck = userManager->getRawData()["noColorCheck"].asInt();

    ImGui::SetNextItemWidth(width / 3);
    
    if (ImGui::SliderInt(u8"音效音量", &FX, 0, 1000)) {
        AudioLoader::get().SetButtonVolume(FX);
        userManager->getRawData()["FX Volume"] = FX;
    }

    ImGui::SetNextItemWidth(width / 3);
    if (ImGui::SliderInt(u8"背景音乐音量", &BGM, 0, 1000)) {
        AudioLoader::get().SetBGMVolume(BGM);
        userManager->getRawData()["BGM Volume"] = BGM;
    }
    ImGui::Separator();
    if (ImGui::Checkbox(u8"禁用颜色相似度检查", &colorCheck)) {
        userManager->getRawData()["noColorCheck"] = colorCheck;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
    ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
    if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock&&subWindow.size()==0)
    {
        AudioLoader::get().playButton("TAP");
        ButtonLock = 1;
        swicthFrameR = swicthFrameL = 0;
        animeManager.addAnime(&swicthFrameR, 0, width, Framedelay, OutSine, 0, [&]() {
            ButtonLock = 0;
            Tab = TAB::Main;
            animeManager.addAnime(&swicthFrameL, 0, width, Framedelay, InSine);
        });
        //cout << "喵7";
    }
    ImGui::PopStyleColor();
}
////////////////////////////
void MainUI() {
    ImGuiStyle& Style = ImGui::GetStyle();
    Color = Style.Colors;

    switch (Color_)
    {
    case Color_::Red:
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;

        Color[ImGuiCol_Button] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(212, 71, 94, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(172, 31, 54, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(192, 51, 74, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(172, 31, 54, 255);

        Color[ImGuiCol_Header] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(212, 71, 94, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(172, 31, 54, 255);

        Color[ImGuiCol_TitleBg] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_TitleBgActive] = ImColor(172, 31, 54, 255);
        Color[ImGuiCol_ResizeGrip] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_ResizeGripHovered] = ImColor(212, 71, 94, 255);
        Color[ImGuiCol_ResizeGripActive] = ImColor(172, 31, 54, 255);

        //ImGuiCol_Separator,
        Color[ImGuiCol_SeparatorHovered] = ImColor(212, 71, 94, 255);
        Color[ImGuiCol_SeparatorActive] = ImColor(172, 31, 54, 255);
        break;
    case Color_::Green:
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;

        Color[ImGuiCol_Button] = ImColor(10, 105, 56, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(30, 125, 76, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(0, 95, 46, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(10, 105, 56, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(10, 105, 56, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(0, 95, 46, 255);

        Color[ImGuiCol_Header] = ImColor(10, 105, 56, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(30, 125, 76, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(0, 95, 46, 255);

        break;
    case Color_::Blue:
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;

        Color[ImGuiCol_Button] = ImColor(51, 120, 255, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(71, 140, 255, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(31, 100, 225, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(51, 120, 255, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(51, 120, 255, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(31, 100, 225, 255);

        Color[ImGuiCol_Header] = ImColor(51, 120, 255, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(71, 140, 255, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(31, 100, 225, 255);

        break;
    case Color_::Orange://233,87,33
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;

        Color[ImGuiCol_Button] = ImColor(233, 87, 33, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(253, 107, 53, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(213, 67, 13, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(233, 87, 33, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(233, 87, 33, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(213, 67, 13, 255);

        Color[ImGuiCol_Header] = ImColor(233, 87, 33, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(253, 107, 53, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(213, 67, 13, 255);

        break;
    }

    ImGui::Begin(
        "Burning Canvas",
        NULL,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |//|
        //ImGuiWindowFlags_NoMove//请添加移动条谢谢
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );
    ImGui::SetWindowSize({ Screen_Width * 0.6f, Screen_Width * 0.4f });//设置窗口大小

    switch (Tab)
    {
    case TAB::Main:
        MAIN();
        break;
    case TAB::GameSelect:
        GAMESELECT();
        break;
    case TAB::Game:
        GAME();
        break;
    case TAB::MoreMode:
        MOREMODE();
        break;
    case TAB::RatingMode:
        RATINGMODE();
        break;
    case TAB::NetModeSelect:
        NETMODESELECT();
        break;
    case TAB::ChatRoom:
        CHATROOM();
        break;
    case TAB::PkMode:
        PKMODE();
        break;
    case TAB::EditorSelect:
        EDITORSELECT();
        break;
    case TAB::Editor_Random:
        EDITORRANDOM();
        break;
    case TAB::Editor_Custom:
        EDITORCUSTOM();
        break;
    case TAB::Editor_Generator:
        EDITORGENERATOR();
        break;
    case TAB::About:
        ABORT();
        break;
    case TAB::Settings:
        SETTINGS();
        break;
    }
    //if (subwindow != nullptr) {
    //    subwindow();
    //}
    if (subWindow.size()) {//   更新更优秀的（应该）子窗口管理系统，使用了vector作为“可下标访问的stack”来实现
        for (int index = 0; index < subWindow.size(); index++) {
            if (index < subWindow.size() - 1) {
                if (subWindow[index].isDraw) {
                    subWindow[index].window(subWindow[index].isAct);
                }
            }
            else {
                subWindow[index].window(true);
            }
        }
    }

    //界面切换用的FRAMEMASK
    drawRect(swicthFrameL, 0, swicthFrameR, height, ImColor(0, 0, 0, 255));
    ImGui::End();
    animeManager.updateAnime();
    //cout << 123 << "]";
    //system("pause");
}
//<-行数