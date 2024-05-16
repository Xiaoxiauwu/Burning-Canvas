#include"UI.h"
#include <string>
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

void clearMap() {
    for (Map*& _map : Maps) {
        delete _map;
    }
    Maps.clear();
}

void EXIT() {
    clearMap();
    exit(0);
}

namespace encode
{
    std::string wideToMulti(std::wstring_view sourceStr, UINT pagecode)
    {
        auto newLen = WideCharToMultiByte(pagecode, 0, sourceStr.data(), sourceStr.size(),
            nullptr, 0, nullptr, nullptr);

        std::string targetStr;
        targetStr.resize(newLen);
        WideCharToMultiByte(pagecode, 0, sourceStr.data(), sourceStr.size(),
            &targetStr[0], targetStr.size(), nullptr, nullptr);
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
    MapNames =GetAllFiles(L"Map\\*.txt");
    clearMap();
    for (wstring level : MapNames) {
        Maps.push_back(new Map(level));
    }
    cout << MapNames[0];
}

void init() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;
    ImFontConfig Font_cfg;
    Font_cfg.FontDataOwnedByAtlas = false;

    //ImFont* Font = io.Fonts->AddFontFromFileTTF("..\\ImGui Tool\\Font.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    Font = io.Fonts->AddFontFromMemoryTTF((void*)Font_data, Font_size, Screen_Width / 1024 * 18.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    Font_Big = io.Fonts->AddFontFromMemoryTTF((void*)Font_data, Font_size, Screen_Width / 1024 * 24.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    setFont(Font);

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
            Editor
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

        ImGui::Begin("Mywindows", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowSize({ Screen_Width * 0.6f, Screen_Width * 0.4f });//设置窗口大小
        static float width = Screen_Width * 0.6f;
        static float height = Screen_Width * 0.4f;
        float buttonwidth = width / 4;
        float kbw = 6.0;
        string space = " ";
        int d = 0;
        int baseoffset = Screen_Width / 1024 * 24.0f + width / 25;
        switch (Tab)
        {
        case Tab::Main:
            ImGui::SetCursorPos({ width*0.3f- buttonwidth /2,height*0.6f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"开始游戏", { buttonwidth,buttonwidth/ kbw }))
            {
                Tab = Tab::GameSelect;
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * (0.6f+0.07f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"关卡编辑器", { buttonwidth,buttonwidth / kbw }))
            {
                Tab = Tab::Editor;
                cout << "喵3";
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.3f - buttonwidth / 2,height * (0.6f + 0.14f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"关于", { buttonwidth,buttonwidth / kbw }))
            {
                //Tab = Tab::Panel;
                cout << "喵4";
            }
            ImGui::PopStyleColor();


            ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * 0.6f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"排行榜", { buttonwidth,buttonwidth / kbw }))
            {
                //Tab = Tab::Panel;
                cout << "喵2";
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * (0.6f + 0.07f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"设置", { buttonwidth,buttonwidth / kbw }))
            {
                //Tab = Tab::Panel;
                cout << "喵6";
            }
            ImGui::PopStyleColor();

            ImGui::SetCursorPos({ width * 0.7f - buttonwidth / 2,height * (0.6f + 0.14f) });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"退出", { buttonwidth,buttonwidth / kbw }))
            {
                EXIT();
            }
            ImGui::PopStyleColor();
            break;
        case Tab::GameSelect:
            ImGui::PushFont(Font_Big);
            ImGui::SetCursorPos({ width/2- Screen_Width / 1024 * 24.0f*2,height/50 });/////////////////
            //ImGui::TextColored(Color[ImGuiCol_Button], u8"选择关卡");//\u9B08
            ImGui::Text(u8"选择关卡");
            ImGui::PopFont();
            
            for (auto level : Maps) {
                ImGui::SetCursorPos({ width * (0.5f - 0.35f),baseoffset + width * 0.17f * d });/////////////////50.0f->width/20
                ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
                bool leave = 0;
                if (ImGui::Button(space.c_str(), {width * 0.7f,width * 0.15f}))
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
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }))
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
            if (ImGui::Button(u8"逆时针旋转", { buttonwidth / 2,buttonwidth / kbw }))
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
            if (ImGui::Button(u8"顺时针旋转", { buttonwidth / 2,buttonwidth / kbw }))
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
            if (ImGui::Button(u8"燃烧", { buttonwidth / 2,buttonwidth / kbw }))
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
            if (ImGui::Button(u8"撤回", { buttonwidth / 2,buttonwidth / kbw }))
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
            if (ImGui::Button(u8"提示", { buttonwidth / 2,buttonwidth / kbw }))
            {
                hintstate = MapDisplayer.myMap->Hint();
                scoreboard->hint();
            }
            ImGui::PopStyleColor();
            /////////////////////////////////////////////////////////////////////////
            ImGui::SetCursorPos({ width * 0.86f - buttonwidth / 2, height * 0.4f + 4.5f * buttonwidth / kbw });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"提交答案", { buttonwidth, buttonwidth / kbw }))
            {
                hintstate = NONE;
                //LastMap.push(new Map(MapDisplayer.myMap));
                if (!MapDisplayer.myMap->Submit()) {
                    scoreboard->submit();
                }
                else {
                    //还没写好
                    hintstate = NONE;
                    MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);
                    Tab = Tab::GameSelect;
                    cout << "喵7";
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
            if (ImGui::Button(u8"重试", { buttonwidth * 2 / kbw, buttonwidth / kbw }))
            {
                hintstate = NONE;
                MapDisplayer.setMap(new Map(SelectMap));
                while (!LastMap.empty()) {
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
            if (ImGui::Button(u8"退出", { buttonwidth * 2 / kbw,buttonwidth / kbw }))
            {
                hintstate = NONE;
                MapDisplayer.setSize(Screen_Width * 0.6f * 0.15f * 0.8f);
                Tab = Tab::GameSelect;
                cout << "喵7";
            }
            ImGui::PopStyleColor();

            break;
        case Tab::Editor:

            drawRect(50, 100, 50, 50, ImVec4(1, 0, 0, 1));

            ImGui::SetCursorPos({ 5.0f,5.0f });
            ImGui::PushStyleColor(ImGuiCol_Button, Color[ImGuiCol_Button]);
            if (ImGui::Button(u8"返回", { buttonwidth * 2 / kbw,buttonwidth / kbw }))
            {
                Tab = Tab::Main;
                cout << "喵7";
            }
            ImGui::PopStyleColor();
            break;
        }
        

        ImGui::End();
    }
}