#include "Tool.h"

ImFont* Font_tiny;

void setFont(ImFont *F) {
    Font_tiny = F;
}

void drawRect(int x,int y,int w,int h,ImVec4 color) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        IMVEC2_ADD2(ImGui::GetWindowPos(), ImVec2(x+w, -ImGui::GetScrollY()+y+h)),
        IMVEC2_ADD2(ImGui::GetWindowPos(), ImVec2(x, -ImGui::GetScrollY()+y)),
        IMVEC4_CVT_COLU32(color),
        0,//ImGui::GetStyle().FrameRounding,
        NULL
    );
}

void drawText(float x, float y, float size, ImVec4 color, string text) {
    ImGui::PushFont(Font_tiny);
    float Size = ImGui::GetFontSize();
    ImGui::SetWindowFontScale(size/Size);
    ImGui::SetCursorPos({ x - size * text.length() / 4, y - size / 2 });
    ImGui::TextColored(color, text.c_str());
    ImGui::PopFont();
    ImGui::SetWindowFontScale(1.0);
}

ScoreBoard::~ScoreBoard() {

}

ScoreBoard::ScoreBoard() {
    baseTime = clock();
    stepCount = 0;
    regretCount = 0;
    hintCount = 0;
    submitFailedCount = 0;
}

void ScoreBoard::Reset() {
    baseTime = clock();
    stepCount = 0;
    regretCount = 0;
    hintCount = 0;
    submitFailedCount = 0;
}

int ScoreBoard::GetTime() {
    return (clock() - baseTime) / CLOCKS_PER_SEC;
}

int ScoreBoard::calcScore() {
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

string sec2time(int sec) {
    string m = to_string(sec / 60);
    string s = to_string(sec % 60);
    if (m.length() < 2)m = "0" + m;
    if (s.length() < 2)s = "0" + s;
    return m + ":" + s;
}