#pragma once
#include<vector>
#include<map>
#include<functional>
enum MotionType {
    Linear,
    OutSine,
    InSine,
    InOutSine,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    InQuart,
    OutQuart,
    InOutQuart,
    InQuint,
    OutQuint,
    InOutQuint,
    InExpo,
    OutExpo,
    InOutExpo,
    InCirc,
    OutCirc,
    InOutCirc,
    InElastic,
    OutElastic,
    InOutElastic,
    InBack,
    OutBack,
    InOutBack,
    InBounce,
    OutBounce,
    InOutBounce,
    CustomMotion
};
struct ANIME {
    //  ��time(0)Ϊ��׼�����Կ�long
	long startTime;     //  ��ʼʱ��
    long duringTime;    //  ����ʱ��
	float* Value;       //  �󶨵�float����
    float beginValue;   //  ��ʼֵ
    float endValue;     //  ����ֵ
    MotionType mType;   //  ʹ�õĻ�����������
    std::function<void()> endFunc = nullptr;//  �ö���������ִ�еĺ���
    std::function<long double(long, long, long)> customMotion = nullptr;//  �Զ��建������
};

class AnimeManager {
public:
	AnimeManager();
	~AnimeManager();
	void updateAnime();
    void addAnime(float* Value, float beginValue, float endValue, long during, MotionType mt, long offset = 0, std::function<void()> endFunc = nullptr, std::function<long double(long, long, long)> customMotion = nullptr);
    void unBind(float* Value);
private:
    std::map<float*, ANIME> animeList;
};

extern AnimeManager animeManager;