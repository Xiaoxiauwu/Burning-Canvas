#include"UI.h"
#include "CustomMap.h"

/////////////////////////////////////GLOBAL VARIABLES////////////////////////////////////////
float Screen_Width;
float Screen_Height;
vector<wstring> MapNames;
vector<Map*> Maps;
MapDisplay MapDisplayer;

Map *SelectMap, *GameMap;
stack<Map*> LastMap;
string playerOperation;

ImFont* Font, * Font_Big;

ScoreBoard* scoreboard = new ScoreBoard();

HintState hintstate = NONE;

bool ButtonLock = 0;//  播放过场动画（粒子）时，用于锁定全局按钮的线程锁

UserManager* userManager = new UserManager();

CustomManager* customManager = new CustomManager();
Drawer drawer;
ImageLoader* imageLoader = new ImageLoader();
/////////////////////////////////////MAIN FUNCTION////////////////////////////////////////

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
    exit(0);
}

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

    //std::wstring utf8_to_wstring(const char* utf8) {
    //    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    //    return converter.from_bytes(utf8);
    //}
    //std::string utf8_to_string(const char* utf8) {
    //    return wideToUtf8(utf8_to_wstring(utf8));
    //}
}

//std::ostream& operator<<(std::ostream& os, const wchar_t* strPt)
//{
//    return os << std::wstring_view(strPt);
//}
std::ostream& operator<<(std::ostream& os, std::wstring_view str)
{
    return os << encode::wideToOme(str);
}

// 一个不合规的实现, 直接将 wstring 写入 stdout 避免字符窄化带来的乱码
// std::ostream &operator<<(std::ostream &os, std::wstring_view str)
// {
//     auto static outHd = GetStdHandle(STD_OUTPUT_HANDLE);
//     WriteConsoleW(outHd,str.data(),str.size(),nullptr,NULL);
//     return os;
// }

vector <wstring> GetAllFiles(wstring path)
{
    vector<wstring> files;
    HANDLE hFind;
    WIN32_FIND_DATA findData;
    LARGE_INTEGER size;
    hFind = FindFirstFile(path.c_str(), &findData); //搜索第一个文件，创建并返回搜索句柄，有则返回TRUE
    if (hFind == INVALID_HANDLE_VALUE)
    {
        cout << "Failed to find first file!\n";
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
    MapNames =GetAllFiles(L"Map\\*.dat");
    clearMap();
    for (wstring level : MapNames) {
        Maps.push_back(new Map(encode::wideToOme(level)));
        if ((*Maps.rbegin())->getID() == -1) {
            Maps.erase(--Maps.end());
        }
    }
    cout << MapNames[0];

    userManager->LoadInfo();
    cout << userManager->getName();
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
    Font = io.Fonts->AddFontFromFileTTF("Source Han Sans & Saira Hybrid-Regular.ttf", Screen_Width / 1024 * 18.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    //Font_Big = io.Fonts->AddFontFromFileTTF("Source Han Sans & Saira Hybrid-Regular.ttf", Screen_Width / 1024 * 24.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());

    setFont(Font);

    //  加载图片资源
    int w, h;//占位用，没啥用
    imageLoader->Init(g_pd3dDevice);
    imageLoader->LoadImageToTexture("drawpix", "res/drawpix.png", w, h);
    imageLoader->LoadImageToTexture("eraser", "res/eraser.png", w, h);
    imageLoader->LoadImageToTexture("line", "res/line.png", w, h);
    imageLoader->LoadImageToTexture("squra", "res/squra.png", w, h);
    imageLoader->LoadImageToTexture("squraf", "res/squraf.png", w, h);
    imageLoader->LoadImageToTexture("circ", "res/circ.png", w, h);
    imageLoader->LoadImageToTexture("circf", "res/circf.png", w, h);
    imageLoader->LoadImageToTexture("fill", "res/fill.png", w, h);
    imageLoader->LoadImageToTexture("select", "res/select.png", w, h);
    imageLoader->LoadImageToTexture("move", "res/move.png", w, h);
    imageLoader->LoadImageToTexture("center", "res/center.png", w, h);

    imageLoader->LoadImageToTexture("back", "res/back.png", w, h);
    imageLoader->LoadImageToTexture("forward", "res/forward.png", w, h);


    imageLoader->LoadImageToTexture("title0", "res/title0.png", w, h);
    imageLoader->LoadImageToTexture("title1", "res/title1.png", w, h);
    imageLoader->LoadImageToTexture("title2", "res/title2.png", w, h);

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
    MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);
}

void getScreenRect() {
    Screen_Width = GetSystemMetrics(SM_CXSCREEN);//获取显示器的宽
    Screen_Height = GetSystemMetrics(SM_CYSCREEN);//获取显示器的高
}

void MainUI() {
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        auto Color = Style.Colors;
        static int Tab = 0;
        enum Tab
        {
            Main,
            GameSelect,
            Game,
            EditorSelect,
            Editor_Random,
            Editor_Custom,
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
            "Mywindows",
            NULL,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings //|
            //ImGuiWindowFlags_NoMove//请添加移动条谢谢
        );
        ImGui::SetWindowSize({ Screen_Width * 0.6f, Screen_Width * 0.4f });//设置窗口大小
        static float width = Screen_Width * 0.6f;
        static float height = Screen_Width * 0.4f;
        float buttonwidth = width / 4;
        float kbw = 6.0;
        string space = " ";
        int d = 0;
        int baseoffset = Screen_Width / 1024 * 24.0f + width / 25;

        static function<void()> subwindow = nullptr;//子窗口

        static vector<int> removeIndex;//色盘相关参数
        static float col[3] = {0,0,0};
        static int selectIndex = 0;

        static int h_ = 0, w_ = 0;//谁都可以用的临时变量
        static ImVec2 cursor;

        static string savedMapName = "";

        static ImVec2 last_mouse_hit_pos = ImVec2(0, 0);//鼠标信息
        static ImGuiContext& g = *GImGui;
        static ImGuiIO& io = g.IO;
        int H=0, W=0;

        float freq = 0.1;

        switch (Tab)
        {
        case Tab::Main:
            W = width * 3 / 5; H = W * 465 / 1213;
            ImGui::SetCursorPos({ width * 0.5f - W / 2,height * 0.25f - H / 2 });
            if (clock() % int(3 * CLOCKS_PER_SEC* freq) < 1000* freq) {
                ImGui::Image(imageLoader->IMG["title0"].image, { W * 1.0f,H * 1.0f });
            }
            else if(clock() % int(3 * CLOCKS_PER_SEC* freq) > 2000* freq) {
                ImGui::Image(imageLoader->IMG["title2"].image, { W * 1.0f,H * 1.0f });
            }
            else {
                ImGui::Image(imageLoader->IMG["title1"].image, { W * 1.0f,H * 1.0f });
            }
            

            ImGui::SetCursorPos({ width*0.3f- buttonwidth /2,height*0.6f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"开始游戏", { buttonwidth,buttonwidth/ kbw }) && !ButtonLock)
            {
                Tab = Tab::GameSelect;
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * (0.6f+0.07f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"关卡编辑器", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                Tab = Tab::EditorSelect;
                cout << "喵3";
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * (0.6f + 0.14f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"关于", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                //Tab = Tab::Panel;
                cout << "喵4";
            }
            ImGui::PopStyleColor();


            ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * 0.6f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"其他模式", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                //Tab = Tab::Panel;
                cout << "喵2";
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * (0.6f + 0.07f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"设置", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                Tab = Tab::Settings;
                cout << "喵6";
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * (0.6f + 0.14f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"退出", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                EXIT();
            }
            ImGui::PopStyleColor();
            break;
        case Tab::GameSelect:
            //ImGui::PushFont(Font_Big);
            //ImGui::SetCursorPos({ width/2- Screen_Width / 1024 * 24.0f*2,height/50 });/////////////////
            //ImGui::TextColored(Color[ImGuiCol_Button], u8"选择关卡");//\u9B08
            drawText(
                width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
                Screen_Width / 1024 * 24.0f,
                ImVec4(255, 255, 255, 255),
                u8"选择关卡"
            );
            //ImGui::Text(u8"选择关卡");
            //ImGui::PopFont();
            MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);
            
            for (auto level : Maps) {
                ImGui::SetCursorPos({ width * (0.5f - 0.35f),baseoffset + width * 0.17f * d });/////////////////50.0f->width/20
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                bool leave = 0;
                if (ImGui::Button(space.c_str(), {width * 0.7f,width * 0.15f}) && !ButtonLock)
                {
                    SelectMap = level;
                    GameMap = new Map(level);
                    MapDisplayer.setMap(GameMap);
                    //initFont(height * 0.7f / max(GameMap->h, GameMap->w));

                    Tab = Tab::Game;
                    //Tab = Tab::Panel;
                    cout << level->getName() <<d<< ImGui::GetWindowPos().x<<" "<< ImGui::GetWindowPos().y;
                    leave = 1;
                    scoreboard->Reset();
                    playerOperation = "";
                }
                space += " ";
                ImGui::PopStyleColor();
                if(leave) break;
                ImGui::SetCursorPos({
                    width * (0.5f - 0.35f) + width * 0.15f,
                    baseoffset + width * 0.15f / 12 + width * 0.17f * d/////////////////
                });
                ImGui::PushFont(Font);
                ImGui::Text(level->getName().c_str());
                ImGui::PopFont();

                MapDisplayer.setCenter(width* (0.5f - 0.35f) + width * 0.15f / 2, baseoffset + width * 0.17f * d + width * 0.15f / 2);
                MapDisplayer.setMap(level);
                MapDisplayer.displayFinish();
                //DrawRect(50,100,50,50,ImVec4(1,0,0,1));
                
                //ImGui::GetWindowDrawList()->AddRectFilled(
                //    IMVEC2_ADD2(ImGui::GetWindowPos(), ImVec2(x + w, y + h)),
                //    IMVEC2_ADD2(ImGui::GetWindowPos(), ImVec2(x, y)),
                //    IMVEC4_CVT_COLU32(color),
                //    0,//ImGui::GetStyle().FrameRounding,
                //    NULL
                //);

                d++;
            }

            ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                Tab = Tab::Main;
            }
            ImGui::PopStyleColor();
            break;
        case Tab::Game:
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
                buttonwidth / kbw*0.6,
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
            if (ImGui::Button(u8"逆时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
            {
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
            if (ImGui::Button(u8"顺时针旋转", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
            {
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
            ImGui::SetCursorPos({ width * 0.86f- buttonwidth / 4, height * 0.4f + 1.5f*buttonwidth / kbw });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"燃烧", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
            {
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
            if (ImGui::Button(u8"撤回", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
            {
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
            if (ImGui::Button(u8"提示", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
            {
                hintstate = MapDisplayer.myMap->Hint();
                scoreboard->hint();
            }
            ImGui::PopStyleColor();
            /////////////////////////////////////////////////////////////////////////
            ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"提交答案", { buttonwidth, buttonwidth / kbw }) && !ButtonLock)
            {
                hintstate = NONE;
                //LastMap.push(new Map(MapDisplayer.myMap));
                if (!MapDisplayer.myMap->Submit()) {
                    scoreboard->submit();
                }
                else {
                    ButtonLock = 1;
                    scoreboard->lock();
                    subwindow = [&]()->void {
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
                        ImGui::SetCursorPos({ 0, Screen_Width / 1024 * 24.0f});
                        ImGui::Indent(width / 12);
                        ImGui::Text(encode::string_To_UTF8((string)"罚时：" + to_string(scoreboard->calcScore())).c_str());
                        ImGui::Text(encode::string_To_UTF8((string)"用时：" + sec2time(scoreboard->GetTime())).c_str());
                        string tip = u8"";
                        if (userManager->getName() == userManager->nulluser)tip = u8"你当前未登录账号，无法保存通关记录";
                        
                        drawText(
                            w / 2, h - buttonwidth / kbw * 1.4f- Screen_Width / 1024 * 24.0f / 2/2,
                            Screen_Width / 1024 * 24.0f / 2,
                            ImVec4(255, 0, 0, 255),
                            tip
                        );
                        //ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());
                        ImGui::SetCursorPos({ w / 2 - buttonwidth / 4, h - buttonwidth / kbw * 1.3f });
                        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            subwindow = nullptr;
                            ButtonLock = 0;
                            hintstate = NONE;
                            MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);
                            Tab = Tab::GameSelect;
                            while (LastMap.size()) {
                                Map* tmp = LastMap.top();
                                LastMap.pop();
                                delete tmp;
                            }

                            if (userManager->getName() != userManager->nulluser) {
                                userManager->saveHistory(
                                    MapDisplayer.myMap->getID(),
                                    scoreboard->GetTime(),
                                    scoreboard->calcScore()
                                );
                            }
                            scoreboard->unlock();
                            cout << "ok";
                        }

                        ImGui::PopStyleColor();
                        ImGui::EndChild();

                    };
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
            if (ImGui::Button(u8"重试", { buttonwidth * 2 / kbw, buttonwidth / kbw }) && !ButtonLock)
            {
                hintstate = NONE;
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
            if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                hintstate = NONE;
                MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);
                Tab = Tab::GameSelect;
                while (LastMap.size()) {
                    Map* tmp = LastMap.top();
                    LastMap.pop();
                    delete tmp;
                }
                cout << "喵7";
            }
            ImGui::PopStyleColor();

            break;
        case Tab::EditorSelect:
            drawText(
                width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
                Screen_Width / 1024 * 24.0f,
                ImVec4(255, 255, 255, 255),
                u8"选择生成模式"
            );

            ImGui::SetCursorPos({ width / 2.0f - width / 5.0f - 5.0f ,height / 2.0f - height * 0.75f / 2 });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"  ", { width / 5.0f, height * 0.75f }) && !ButtonLock)
            {
                Tab = Tab::Editor_Random;
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width / 2.0f+5.0f, height / 2.0f - height * 0.75f / 2 });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"    ", { width / 5.0f, height * 0.75f }) && !ButtonLock)
            {
                drawer.map->setSize(10, 10);
                drawer.cx = width / 3.0f;
                drawer.cy = height / 3.0f;
                drawer.k = 10;
                drawer.size = min(width, height) * 2.0f / 3 * 0.8;
                Tab = Tab::Editor_Custom;
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ 5.0f,5.0f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                Tab = Tab::Main;
                cout << "喵7";
            }
            ImGui::PopStyleColor();
            break;
        case Tab::Editor_Random:
            //drawText(
            //    width / 2, Screen_Width / 1024 * 24.0f / 2 + 2 * height / 50,
            //    Screen_Width / 1024 * 24.0f,
            //    ImVec4(255, 255, 255, 255),
            //    u8"随机生成关卡"
            //);
            ImGui::SetCursorPos({ 5.0f,10.0f+ buttonwidth / kbw });
            ImGui::BeginChild(
                u8"设定",
                ImVec2(width/3.8f, height - 20.0f - buttonwidth / kbw),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            ImGui::SliderInt(u8"宽度", &customManager->w, 3, 20);
            ImGui::SliderInt(u8"高度", &customManager->h, 3, 20);
            ImGui::SliderInt(u8"最小数字", &customManager->maxstep, 1, 400);
            ImGui::SliderInt(u8"最大数字", &customManager->minstep, 2, 400);

            ImGui::PushFont(Font);
            ImGui::BulletText(u8"色盘");
            ImGui::PopFont();
            ImGui::Separator();
            for (int index = 0; index < customManager->palette.size(); index++) {
                ImGui::BeginChild(
                    (encode::wideToUtf8(L"色盘" + to_wstring(index + 1))).c_str(),
                    ImVec2(width / 3.8f-10.0f, buttonwidth / kbw+5.0f),
                    1,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]);

                ImGui::SameLine();
                if (ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]) && !ButtonLock)
                {
                    selectIndex = index;
                    col[0] = customManager->palette[index].x;
                    col[1] = customManager->palette[index].y;
                    col[2] = customManager->palette[index].z;
                    ButtonLock = 1;
                    subwindow = [&]()->void {
                        drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                        float w, h, x, y;//长宽，中心坐标
                        w = width / 2.8;
                        h = width / 2.8+ buttonwidth / kbw;
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
                        ImVec2 Pos=ImGui::GetCursorPos();
                        ImGui::SetCursorPosX(Pos.x +w/2 - buttonwidth / kbw);
                        if (ImGui::Button(u8"确定", { buttonwidth * 2 / kbw,buttonwidth / kbw }))
                        {
                            customManager->palette[selectIndex] = ImVec4(col[0], col[1], col[2], 1);
                            subwindow = nullptr;
                            ButtonLock = 0;
                        }
                        ImGui::SetCursorPosX(Pos.x);
                        ImGui::EndChild();
                        };
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"X") && !ButtonLock)
                {
                    removeIndex.push_back(index);
                }
                if (ButtonLock)drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
                ImGui::EndChild();
            }
            
            if (ImGui::Button(u8"+", { width / 3.8f - 10.0f,buttonwidth / kbw }) && !ButtonLock)
            {
                customManager->palette.push_back(ImVec4(rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, 1));
            }
            if (ButtonLock)drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
            for (auto it = removeIndex.rbegin(); it != removeIndex.rend();it++) {
                if (customManager->palette.size() <= 2) {
                    break;
                }
                customManager->palette.erase(customManager->palette.begin()+*it);
            }
            removeIndex.clear();
            //ImGui::ColorPicker3(u8"测试", col);
            //ImGui::Text(to_string(col[0]).c_str());
            //ImGui::Text(to_string(col[1]).c_str());
            //ImGui::Text(to_string(col[2]).c_str());

            ImGui::PopStyleColor();
            if (ButtonLock)drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
            ImGui::EndChild();
            
            ImGui::SetCursorPos({ 5.0f+ width / 3.8f,10.0f + buttonwidth / kbw });
            ImGui::BeginChild(
                u8"展示",
                ImVec2(width-width / 3.8f-10.0f, height - 20.0f - buttonwidth / kbw),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );
            w_ = width - width / 3.8f - 10.0f;
            h_ = height - 20.0f - buttonwidth / kbw;
            MapDisplayer.setSize(w_ * 0.5 * 0.9);
            MapDisplayer.setMap(customManager->getBegin());
            MapDisplayer.setCenter(w_*0.25,h_*0.5);
            MapDisplayer.display();
            MapDisplayer.setMap(customManager->getEnd());
            MapDisplayer.setCenter(w_ * 0.75, h_ * 0.5);
            MapDisplayer.display();

            ImGui::SetCursorPos({ w_ * 0.5f - buttonwidth - 5.0f, h_ * 0.5f + w_ * 0.25f });
            if (ImGui::Button(u8"重新生成", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                customManager->Random();
            }
            ImGui::SetCursorPos({ w_ * 0.5f + 5.0f, h_ * 0.5f + w_ * 0.25f });
            if (ImGui::Button(u8"保存关卡", { buttonwidth,buttonwidth / kbw }) && !ButtonLock)
            {
                savedMapName = to_string(time(0));
                customManager->SaveMap("Map/R" + savedMapName + ".dat", "R" + savedMapName, stoi(savedMapName));
                subwindow = [&]()->void {
                    drawRect(0, 0, width, height, ImVec4(0, 0, 0, 50));
                    static char levelname[16];
                    float w, h, x, y;//长宽，中心坐标
                    w = width / 2.8;
                    h = height / 3.8*0.5;//
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
                    if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }))
                    {
                        Maps.push_back(new Map("R" + savedMapName+".dat"));

                        customManager->reset();

                        Tab = Tab::EditorSelect;
                        subwindow = nullptr;
                        ButtonLock = 0;
                    }

                    ImGui::EndChild();
                };
                ButtonLock = 1;
            }
            if (ButtonLock)drawRect(0, 0, width - width / 3.8f - 10.0f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
            ImGui::EndChild();

            ImGui::SetCursorPos({ 5.0f,5.0f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                customManager->reset();

                Tab = Tab::EditorSelect;
            }
            ImGui::PopStyleColor();

            break;
        case Tab::Editor_Custom:
            w_ = width * 2 / 3.0f;
            h_ = height * 2 / 3.0f;
            ImGui::SetCursorPos({ width / 2.0f - w_ / 2.0f,height / 2.0f - h_ / 2.0f });

            ImGui::BeginChild(
                u8"绘图区", 
                { w_*1.0f,h_*1.0f },
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse
            );

            MapDisplayer.setMap(drawer.map);
            MapDisplayer.setSize(drawer.size * drawer.k / 10.0);
            MapDisplayer.setCenter(drawer.cx, drawer.cy);
            MapDisplayer.display(true);
            
            drawer.map->getSize(W, H);
            ImGui::SetCursorPos({ drawer.cx - drawer.size / max(W,H) * W * drawer.k / 10.0f / 2,drawer.cy - drawer.size / max(W,H) * H * drawer.k / 10.0f / 2 });
            //  处理拖动
            if (PaintBoard(u8"按钮", { drawer.size / max(W,H) * W * drawer.k / 10.0f,drawer.size/max(W,H) *H * drawer.k / 10.0f })&&drawer.getState()!=MOVE && !ButtonLock) {
                //std::cout << 33;
                cout << ImVec2(
                    int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))),
                    int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H)))
                    )<<"\n";
            }
            if (drawer.getState() == MOVE && ImGui::IsItemActivated() && !ButtonLock) {
                last_mouse_hit_pos = GetMousePos_WithRestrict(w_, h_, 0, 0);
            }
            if (drawer.getState() == MOVE && ImGui::IsItemActive() && !ButtonLock) {
                //cout << GetMousePos_InWindow();
                drawer.cx += (GetMousePos_WithRestrict(w_, h_, 0, 0) - last_mouse_hit_pos).x;
                drawer.cy += (GetMousePos_WithRestrict(w_, h_, 0, 0) - last_mouse_hit_pos).y;
                last_mouse_hit_pos = GetMousePos_WithRestrict(w_, h_, 0, 0);
            }
            if (ImGui::IsItemHovered() && io.MouseWheel != 0 && !ButtonLock) {
                if (drawer.k + io.MouseWheel >= 1 && drawer.k + io.MouseWheel <= 30) {
                    float dw__ = GetMousePos_WithRestrict(w_, h_, 0, 0).x - (ImGui::GetWindowPos().x) - drawer.cx, dh__ = GetMousePos_WithRestrict(w_, h_, 0, 0).y - (ImGui::GetWindowPos().y) - drawer.cy;
                    float dw_ = dw__ / drawer.k * (drawer.k + io.MouseWheel), dh_ = dh__ / drawer.k * (drawer.k + io.MouseWheel);
                    drawer.k += io.MouseWheel;
                    drawer.cx -= (dw_ - dw__);
                    drawer.cy -= (dh_ - dh__);
                }
                //std::cout << io.MouseWheel << "\n";
            }

            //  绘制选择框
            if (ImGui::IsItemHovered() && drawer.getState() != MOVE) {
                MapDisplayer.showUnitBoard(
                    int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                    int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1
                );
            }

            //  处理不同状态下执行的功能
            if (drawer.getState() == DRAWDOT) {
                if (drawer.getIndex() != -1) {
                    if (ImGui::IsItemActivated()) {//这里不需要用到Begin和End，只是先写着，方便之后复制
                        drawer.saveCurrentStep();
                        drawer.setBeginPos({
                            1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                            1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1
                        });
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
                        drawer.setEndPos({
                            1.0f * int((GetMousePos_InWindow().x - (drawer.cx - drawer.size / max(W, H) * W * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1,
                            1.0f * int((GetMousePos_InWindow().y - (drawer.cy - drawer.size / max(W, H) * H * drawer.k / 10.0f / 2)) / (drawer.size * drawer.k / 10.0 / max(W, H))) + 1
                        });
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

            if (ButtonLock)drawRect(0, 0, w_, h_, ImVec4(0, 0, 0, 50));
            ImGui::EndChild();
            ////////////////////////////////////////////
            w_ = width / 6.0f;
            h_ = height * 2 / 3.0f;
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            ImGui::SetCursorPos({ width/2.0f+width/3.0f,height / 2.0f - h_ / 2.0f });
            ImGui::BeginChild(
                u8"色盘区",
                ImVec2(w_ * 1.0, h_ * 1.0),
                1,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings// |
                //ImGuiWindowFlags_NoMove
            );

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
                    ImGuiWindowFlags_NoSavedSettings// |
                    //ImGuiWindowFlags_NoMove
                );
                //ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), customManager->palette[index]);

                if (drawer.getIndex() == index) {
                    drawRect(0, 0, w_ * 1.0+10.0, buttonwidth / kbw, ImVec4(0.3, 0.3, 0.3, 1));
                }

                ImGui::SetCursorPos({ 5.0,5.0 });
                //ImGui::SameLine();
                if (ImGui::ColorButton((encode::wideToUtf8(L"颜色" + to_wstring(index + 1))).c_str(), drawer.palette[index]) && !ButtonLock)
                {
                    selectIndex = index;
                    col[0] = drawer.palette[index].x;
                    col[1] = drawer.palette[index].y;
                    col[2] = drawer.palette[index].z;
                    ButtonLock = 1;
                    subwindow = [&]()->void {
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
                        if (ImGui::Button(u8"确定", { buttonwidth * 2 / kbw,buttonwidth / kbw }))
                        {
                            drawer.saveCurrentStep();
                            drawer.palette[selectIndex] = ImVec4(col[0], col[1], col[2], 1);
                            drawer.map->setPalette(&drawer.palette);
                            drawer.save();
                            drawer.clearNextStep();
                            subwindow = nullptr;
                            ButtonLock = 0;
                        }
                        ImGui::SetCursorPosX(Pos.x);
                        ImGui::EndChild();
                        };
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"X") && !ButtonLock)
                {
                    drawer.saveCurrentStep();
                    removeIndex.push_back(index);
                }

                ImGui::SetCursorPos({ 0,0 });
                
                if (PaintBoard(encode::wideToUtf8(L"c" + to_wstring(index)).c_str(), ImVec2(w_ * 1.0, buttonwidth / kbw))) {
                    drawer.setIndex(index);
                }

                if (ButtonLock)drawRect(0, 0, w_ * 1.0, buttonwidth / kbw, ImVec4(0, 0, 0, 50));
                ImGui::EndChild();
            }

            if (ImGui::Button(u8"+", { w_*1.0f,buttonwidth / kbw }) && !ButtonLock)
            {
                drawer.saveCurrentStep();
                drawer.palette.push_back(ImVec4(rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, rnd(0, 255) / 255.0, 1));
                drawer.map->setPalette(&drawer.palette);
                drawer.save();
                drawer.clearNextStep();
            }
            if (ButtonLock)drawRect(0, 0, width / 3.8f - 10.0f, buttonwidth / kbw + 5.0f, ImVec4(0, 0, 0, 50));
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
            if (ButtonLock)drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
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
            )) {
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
            )) {
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
            )) {

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
                    )) {
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
                    )) {
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
                    )) {
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
                    )) {
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
                    )) {
                        drawer.displaySubToolBar ^= 1;
                        drawer.setState(CIRCF);
                        drawer.setSubState(CIRCF);
                    }
                    ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();
                }
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
            )) {
                drawer.setState(FILL);
            }
            ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Button, drawer.getState() == SELECTOR ? ImVec4(1.0f, 0.6f, 1.0f, 1.0f) : Color[ImGuiCol_Button]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, drawer.getState() == SELECTOR ? ImVec4(1.0f, 0.7f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonHovered]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, drawer.getState() == SELECTOR ? ImVec4(1.0f, 0.75f, 1.0f, 1.0f) : Color[ImGuiCol_ButtonActive]);
            if (ImageButton(
                u8"选择",
                { buttonwidth / kbw ,buttonwidth / kbw },
                imageLoader->IMG["select"].image,
                true,
                { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
            )) {
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
            )) {
                drawer.setState(MOVE);
            }
            ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();

            if (ImageButton(
                u8"画布居中",
                { buttonwidth / kbw ,buttonwidth / kbw },
                imageLoader->IMG["center"].image,
                true,
                { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
            )) {
                drawer.cx = width / 3.0f;
                drawer.cy = height / 3.0f;
            }

            ImGui::PopStyleColor();
            if (ButtonLock)drawRect(0, 0, width / 3.8f, height - 20.0f - buttonwidth / kbw, ImVec4(0, 0, 0, 50));
            ImGui::EndChild();

            //////////////////////////////
            ImGui::SetCursorPos({ 5.0f,height - buttonwidth / kbw - 10.0f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"设置", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                cout << "没写";
                ifd::FileDialog::Instance().Open("TextureOpenDialog", u8"测试用", "Image file (*.png;*.jpg;*.jpeg;*.bmp;*.tga){.png,.jpg,.jpeg,.bmp,.tga},.*");
            }
            ImGui::PopStyleColor();

            if (ifd::FileDialog::Instance().IsDone("TextureOpenDialog")) {
                if (ifd::FileDialog::Instance().HasResult()) {
                    std::string res = ifd::FileDialog::Instance().GetResult().u8string();
                    printf("OPEN[%s]\n", encode::UTF8_To_String(res).c_str());
                }
                ifd::FileDialog::Instance().Close();
            }

            ImGui::SetCursorPos({ width - 2 * buttonwidth / kbw - 10.0f,height - buttonwidth / kbw - 15.0f });
            if (drawer.canBack() && ImageButton(
                u8"上一步",
                { buttonwidth / kbw ,buttonwidth / kbw },
                imageLoader->IMG["back"].image,
                true,
                { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
            )) {
                drawer.retreat();
            }

            ImGui::SetCursorPos({ width - buttonwidth / kbw - 5.0f,height - buttonwidth / kbw - 15.0f });
            if (drawer.canNext() && ImageButton(
                u8"下一步",
                { buttonwidth / kbw ,buttonwidth / kbw },
                imageLoader->IMG["forward"].image,
                true,
                { buttonwidth / kbw * 0.8f ,buttonwidth / kbw * 0.8f }
            )) {
                drawer.reretreat();
            }

            ImGui::SetCursorPos({ width - buttonwidth * 2 / kbw - 5.0f,5.0f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"生成", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                cout << "没写";
            }
            ImGui::PopStyleColor();
            /////////////////////////////////////////////////////
            ImGui::SetCursorPos({ 5.0f,5.0f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                customManager->reset();
                drawer.clearMemory();

                Tab = Tab::EditorSelect;
            }
            ImGui::PopStyleColor();
            break;
        case Tab::Settings:
            //
            drawText(
                width / 2, Screen_Width / 1024 * 24.0f/2+2*height / 50,
                Screen_Width / 1024 * 24.0f,
                ImVec4(255, 255, 255, 255),
                u8"设置"
            );

            ImGui::SetCursorPos({ 50 + buttonwidth * 2 / kbw, 50 + buttonwidth / kbw });
            ImGui::PushFont(Font);
            ImGui::Text((string(u8"当前账号：") + encode::string_To_UTF8(userManager->getName())).c_str());//
            ImGui::PopFont();
            if (userManager->getName() == userManager->nulluser) {
                ImGui::SetCursorPos({ width - (50 + buttonwidth), 50 + buttonwidth / kbw });
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"登录", { buttonwidth/2,buttonwidth / kbw }) && !ButtonLock)
                {
                    //Tab = Tab::Main;

                    subwindow = [&]() ->void {
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
                        ImGui::SetCursorPos({ w/10, Screen_Width / 1024 * 24.0f });
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
                            ImGuiInputTextFlags_Password*(!showPassword)
                        )) {
                            //ImGui::GetInputTextState("账号名");
                            //cout << encode::UTF8_To_String(Username) << "]";
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox(u8" ", &showPassword);
                        /////////////////////////////////////////////////////////////////////////
                        ImGui::TextColored(ImVec4(255, 0, 0, 255), tip.c_str());// 错误提示信息
                        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            //subwindow = nullptr;
                            //ButtonLock = 0;
                            if (userManager->Login(encode::UTF8_To_String(Username), encode::UTF8_To_String(Password))) {
                                subwindow = nullptr;
                                ButtonLock = 0;
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
                        if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            subwindow = nullptr;
                            ButtonLock = 0;
                            tip = u8"";
                            showPassword = 0;
                            memset(Username, '\0', sizeof(Username));
                            memset(Password, '\0', sizeof(Password));
                            cout << "取消";
                        }

                        ImGui::PopStyleColor();
                        ImGui::EndChild();

                    };
                    ButtonLock = 1;
                    cout << "喵7";
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"注册", { buttonwidth/2,buttonwidth / kbw }) && !ButtonLock)
                {
                    //Tab = Tab::Main;
                    subwindow = [&]() ->void {
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
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            //subwindow = nullptr;
                            //ButtonLock = 0;
                            int statu;
                            if (encode::UTF8_To_String(Password) != encode::UTF8_To_String(ComfirmPassword))statu = -3;
                            else statu = userManager->Register(encode::UTF8_To_String(Username), encode::UTF8_To_String(Password));
                            if (statu==1) {
                                subwindow = nullptr;
                                ButtonLock = 0;
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
                        if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            subwindow = nullptr;
                            ButtonLock = 0;
                            tip = u8"";
                            showPassword = 0;
                            memset(Username, '\0', sizeof(Username));
                            memset(Password, '\0', sizeof(Password));
                            memset(ComfirmPassword, '\0', sizeof(ComfirmPassword));
                            cout << "取消";
                        }

                        ImGui::PopStyleColor();
                        ImGui::EndChild();

                        };
                    ButtonLock = 1;
                    cout << "喵8";
                }
                ImGui::PopStyleColor();
            }
            else {
                ImGui::SetCursorPos({ width - (50 + buttonwidth), 50 + buttonwidth / kbw });
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                if (ImGui::Button(u8"退出登录", { buttonwidth, buttonwidth / kbw }) && !ButtonLock)
                {
                    //Tab = Tab::Main;
                    userManager->Quit();
                    cout << "退出登录";
                }

                ImGui::SetCursorPos({ width - (50 + buttonwidth), 50 + buttonwidth / kbw*2 });
                if (ImGui::Button(u8"重置存档", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
                {
                    //Tab = Tab::Main;

                    subwindow = [&]() ->void {
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
                        ImGui::SetCursorPos({ w / 2- buttonwidth / 2 - buttonwidth / kbw * 0.02f, h - buttonwidth / kbw*1.3f });
                        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            subwindow = nullptr;
                            ButtonLock = 0;
                            userManager->ClearHistory();
                            //cout << encode::UTF8_To_String(Username) << "]";
                            cout << "已清空";
                        }
                        ImGui::SetCursorPos({ w / 2 + buttonwidth / kbw * 0.02f, h - buttonwidth / kbw*1.3f });
                        if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            subwindow = nullptr;
                            ButtonLock = 0;
                            cout << "取消";
                        }

                        ImGui::PopStyleColor();
                        ImGui::EndChild();

                    };
                    ButtonLock = 1;
                    cout << "喵7";
                }
                //ImGui::SameLine();
                ImGui::SetCursorPos({ width - (50 + buttonwidth/2), 50 + buttonwidth / kbw * 2 });
                if (ImGui::Button(u8"删除账号", { buttonwidth / 2,buttonwidth / kbw }) && !ButtonLock)
                {
                    //Tab = Tab::Main;

                    subwindow = [&]() ->void {
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
                        ImGui::SetCursorPos({ w / 2 - buttonwidth / 2- buttonwidth / kbw * 0.02f, h - buttonwidth / kbw*1.3f });
                        ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                        if (ImGui::Button(u8"确认", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            subwindow = nullptr;
                            ButtonLock = 0;
                            userManager->DestoryUser();
                            //cout << encode::UTF8_To_String(Username) << "]";
                            cout << "已删除";
                        }
                        ImGui::SetCursorPos({ w / 2 + buttonwidth / kbw * 0.02f, h - buttonwidth / kbw* 1.3f });
                        if (ImGui::Button(u8"取消", { buttonwidth / 2,buttonwidth / kbw }))
                        {
                            //Tab = Tab::Main;
                            subwindow = nullptr;
                            ButtonLock = 0;
                            cout << "取消";
                        }

                        ImGui::PopStyleColor();
                        ImGui::EndChild();

                    };
                    ButtonLock = 1;
                    cout << "喵8";
                }
                
                ImGui::PopStyleColor();
            }
            ImGui::SetCursorPos({ 5.0f,5.0f + ImGui::GetScrollY() });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }) && !ButtonLock)
            {
                Tab = Tab::Main;
                cout << "喵7";
            }
            ImGui::PopStyleColor();


            break;
        }

        if (subwindow != nullptr) {
            subwindow();
        }
        ImGui::End();
    }
}