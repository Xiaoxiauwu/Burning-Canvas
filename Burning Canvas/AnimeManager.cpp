#include <corecrt_math.h>
#include "AnimeManager.h"
#include <ctime>

AnimeManager animeManager;

long double BounceOut(long attime, long dur) {
    long double px = 1.0 * attime / dur;
    if (px < 0.363636374)
        return 7.5625 * px * px;
    if (px < 0.727272749) {
        px = px - 0.545454562;
        return 7.5625 * px * px + 0.75;
    }
    if (px < 0.909090936) {
        px = px - 0.8181818;
        return 7.5625 * px * px + 0.9375;
    }
    px = px - 0.954545438;
    return 7.5625 * px * px + 0.984375;
}

long double motion(long st, long et, long ct, MotionType mt) {
    long dur = et - st;
    long attime = ct - st;
    long double px = 1.0 * attime / dur;
    long double period = 0;
    long double overshootOrAmplitude = 0;

    long double num;
    switch (mt) {// ֻ����һ�ε�switchû��break�ı�Ҫ
    case Linear:
        return px;
    case InSine:
        return 1.0 - long double(cos(px * acos(-1) / 2));
    case OutSine:
        return long double(sin(px * acos(-1) / 2));
    case InOutSine:
        return -0.5 * long double(cos(acos(-1) * px) - 1.0);
    case InQuad:
        return px * px;
    case OutQuad:
        return -px * (px - 2.0);
    case InOutQuad:
        if (px * 2 < 1.0)return 2 * px * px;
        return -0.5 * ((2 * px - 1.0) * (2 * px - 3.0) - 1);
    case InCubic:
        return px* px* px;
    case OutCubic:
        px = px - 1;
        return px * px * px + 1;
    case InOutCubic:
        if (px * 2 < 1.0) return 4 * px * px * px;
        return 0.5 * ((px * 2 - 2.0) * (px * 2 - 2.0) * (px * 2 - 2.0) + 2.0);
    case InQuart:
        return px * px * px * px;
    case OutQuart:
        px = px - 1.0;
        return -(px * px * px * px - 1.0);
    case InOutQuart:
        if (px * 2 < 1.0)return 8 * px * px * px * px;
        px = 2 * px - 2.0;
        return -0.5 * (px * px * px * px - 2.0);
    case InQuint:
        return px * px * px * px * px;
    case OutQuint:
        px = px - 1.0;
        return px * px * px * px * px + 1.0;
    case InOutQuint:
        if (px * 2 < 1.0)return 0.5 * px * px * px * px * px;
        px = px - 2.0;
        return 0.5 * (px * px * px * px * px + 2.0);
    case InExpo:
        if (attime != 0)return pow(2.0, 10.0 * (px - 1.0));
        return 0;
    case OutExpo:
        if (px == 1)return 1;
        return -pow(2.0, -10.0 * px) + 1.0;
    case InOutExpo:
        if (px == 0 || px == 1) return px;
        if (px * 2 < 1.0)return 0.5 * pow(2.0, 10.0 * (px - 1.0));
        return 0.5 * (-pow(2.0, -10.0 * (px - 1.0)) + 2.0);
    case InCirc:
        return -sqrt(1.0 - px * px) + 1.0;
    case OutCirc:
        px = px - 1.0;
        return sqrt(1.0 - px * px);
    case InOutCirc:
        if (px * 2 < 1.0) return -0.5 * (sqrt(1.0 - 4 * px * px) - 1.0);
        px = px * 2 - 2;
        return 0.5 * (sqrt(1 - px * px) + 1.0);
    case InElastic:
        if (px == 0 || px == 1)return px;
        period = dur * 0.3;
        num = 0.0;
        if (overshootOrAmplitude < 1.0) {
            overshootOrAmplitude = 1.0;
            num = period / 4.0;
        }
        else {
            num = period / (2 * acos(-1)) * asin(1.0 / overshootOrAmplitude);
        }
        return -(overshootOrAmplitude * pow(2.0, 10.0 * (px - 1)) * (sin(((px - 1) * dur - num) * (2 * acos(-1)) / period)));
    case OutElastic:
        if (px == 0 || px == 1) return px;
        if (period == 0) {
            period = dur * 0.3;
        }
        num = 0.0;
        if (overshootOrAmplitude < 1.0) {
            overshootOrAmplitude = 1.0;
            num = period / 4.0;
        }
        else {
            num = period / (2 * acos(-1)) * asin(1.0 / overshootOrAmplitude);
        }
        return overshootOrAmplitude * pow(2.0, -10 * px) * sin((px * dur - num) * (2 * acos(-1)) / period) + 1.0;
    case InOutElastic:
        if (px == 0 || px == 1) return px;
        px = px * 2;
        if (period == 0) {
            period = dur * 0.450000018;
        }
        num = 0.0;
        if (overshootOrAmplitude < 1.0) {
            overshootOrAmplitude = 1.0;
            num = period / 4.0;
        }
        else {
            num = period / (2 * acos(-1)) * asin(1.0 / overshootOrAmplitude);
        }
        if (px < 1.0)return -0.5 * (overshootOrAmplitude * pow(2.0, 10.0 * (px - 1)) * (sin(((px - 1) * dur - num) * (2 * acos(-1)) / period)));
        px = px - 1;
        return 0.5 * overshootOrAmplitude * pow(2.0, -10 * px) * sin((px * dur - num) * (2 * acos(-1)) / period) + 1.0;
    case InBack:
        overshootOrAmplitude = 1.70158;
        return px * px * ((overshootOrAmplitude + 1.0) * px - overshootOrAmplitude);
    case OutBack:
        overshootOrAmplitude = 1.70158;
        px = px - 1;
        return  px * px * ((overshootOrAmplitude + 1.0) * px + overshootOrAmplitude) + 1.0;
    case InOutBack:
        overshootOrAmplitude = 1.70158;
        if (px * 2 < 1.0) return 0.5 * (4 * px * px * ((overshootOrAmplitude * 1.525 + 1.0) * 2 * px - overshootOrAmplitude * 1.525));
        px = px * 2 - 2;
        return 0.5 * (px * px * (((overshootOrAmplitude * 1.525) + 1.0) * px + overshootOrAmplitude * 1.525) + 2.0);
    case InBounce:
        return 1- BounceOut(dur-attime, dur);
    case OutBounce:
        return BounceOut(attime, dur);
    case InOutBounce:
        if(2 * attime<dur)return 1 - BounceOut(dur - 2 * attime, dur);
        return 0.5f * BounceOut(attime * 2 - dur, dur) + 0.5f;
    }
}

AnimeManager::AnimeManager()
{
}

AnimeManager::~AnimeManager()
{
}
#include<iostream>
void AnimeManager::updateAnime(){
    long now = clock();
    std::vector<float*> del;
    for (auto& [value, anime] : animeList) {//  ��������Ķ����б���һ���¶���
        if (anime.startTime <= now && anime.startTime + anime.duringTime >= now) {//    ������ڶ������ŵ�ʱ�䷶Χ�ڣ���ô������һ�����ڲ��ŵĶ�����Ҫ���¸ö���
            if (anime.mType != CustomMotion) {//    ���û��ָ�����Զ��建����������ôֱ������Ĭ�ϵ�motiontype���Ȼ����¶������У��������motion()������ͨ��switchʵ�֣�
                *anime.Value = anime.beginValue + (anime.endValue - anime.beginValue) * motion(anime.startTime, anime.startTime + anime.duringTime, now, anime.mType);
            }
            else {//                                ����Ͱ����ṩ�Ļ����������±�����ֵ
                *anime.Value = anime.beginValue + (anime.endValue - anime.beginValue) * anime.customMotion(anime.startTime, anime.startTime + anime.duringTime, now);
            }
        }
        else if (now > anime.startTime + anime.duringTime) {//  ����ö����Ľ���ʱ���Ѿ����ˣ���ô�Ƴ��ö���
            del.push_back(value);
            if (anime.endFunc != nullptr) {//   ����ö������ṩ endFunc����ôִ�����endFunc
                anime.endFunc();
            }
        }
    }

    for (auto value:del) {//    ��������ע�ⲻ����forѭ���ڲ������ᱨ��
        //std::cout <<animeList[value].startTime<< "�¼�������]\n";
        *animeList[value].Value = animeList[value].endValue;
        animeList.erase(value);
    }
}

//     �󶨵ı�����      ��ʼֵ��          ����ֵ��       ����ʱ����     �������ͣ���ʼʱ��ƫ��������ѡ����������������ʱִ�еĺ�������ѡ��������             �Զ��建�����ͣ���ѡ������
void AnimeManager::addAnime(
    float* Value, float beginValue, float endValue, long during, MotionType mt, long offset, std::function<void()> endFunc, std::function<long double(long, long, long)> customMotion
) {
    if (animeList.count(Value)) {// ������ֹ�����ֱ����ǰ����֮ǰ�Ļ�������������д
        if (animeList[Value].endFunc != nullptr) {
            animeList[Value].endFunc();
        }
        *Value = animeList[Value].endValue;
    }
    //std::cout << "������¼���" << clock() + offset << " " << during << "]\n";
    animeList[Value] = { clock() + offset,during,Value,beginValue,endValue,mt,endFunc,customMotion };
}

void AnimeManager::unBind(float* Value){
    if (animeList.count(Value)) {
        animeList.erase(Value);
    }
}