#pragma once
#include <set>
#include <iostream>
#include <string>
#include <winsock.h>
#include <thread>
#include <functional>
#include <map>
#include <vector>
#include <queue>
#pragma comment (lib, "ws2_32.lib")

//#include "Tool.h"
//#include "data.h"
#include "CustomMap.h"

class UserManager;
class CMap;

enum SocketStatu {
	UNACTIVE,
	SERVER,
	CLIENT,
	CLIENT_CONNECTING
};

std::string GetPattern(std::string raw,int k=2);

class SocketManager {
public:
	SocketManager();//	�������óɵ���ģʽ��
	~SocketManager();
	bool GetErrorStatu();//	��ȡ��������״̬
	SocketStatu getStatu();

	void Server_Start(int port_ = -1);//	��ָ���˿�����������
	void Server_Update();//	����������¼����������ͣ�����
	void Server_End();//	�رշ����

	void Client_Start(std::string IP, int port_);//	��ָ���˿�����������
	void Client_ConnetToServer(int* signal);//	��������
	void Client_Update();//	�����ͻ����¼����������ͣ�����
	void Client_End();//	�رտͻ���

	void Update();
	void End();

	int& getPort();
	std::vector<SOCKET> getAllsocket();
	void Broadcast(std::string msg);//	�㲥��Ϣ(�����ר������������Ȼ���������÷����ת�����Կͻ��˵���Ϣ)
	void sendMessage(std::string msg, std::vector<SOCKET> sock = std::vector<SOCKET>(), std::set<SOCKET> ignore = std::set<SOCKET>());
	void Arranger();
	//	������Ϣ
	void SplitSender(std::string msg, std::vector<SOCKET> sock = std::vector<SOCKET>(), std::set<SOCKET> ignore = std::set<SOCKET>(), bool type = 0);//	�ַ���/�ļ��ְ�
	std::string GetLocalHost();
	std::pair<std::string,int> GetClientAddrAndPort(SOCKET sock);

	std::vector<Json::Value>& getMessageList();

	std::vector<Json::Value> MessageList;		//	�û���Ϣ�б�(�ǵ��޶���Ϣ���������С)
	std::map<std::string, Json::Value> UserInfo;//	�û���Ϣ����ID��Ϊ����
	std::map<std::string, CMap*> AvatorInfo;		//	�û�ͷ����Ϣ����ID�������ǵ�delete
	std::map<SOCKET, std::string> SocketToID;	//	�����ר�ã�����˹�������socket������Ҫ�ͻ��˹�
	std::map<std::string, SOCKET> IDToSocket;	//	�����ר�ã�����˹�������socket������Ҫ�ͻ��˹�
	
	std::map<std::string, CMap*> tmpMapInfo;			//	���ջ����õ�ͼ
	std::string selectMapID;

	void setUserManager(UserManager* userManager_);
	void setMapDisplayer(MapDisplay* MapDisplayer_);
	void setMaps(std::vector<Map*>* Maps_);
	void setIDtoMaps(std::map<int, int>* IDtoMaps_);
	void setTmp(Json::Value tmp_);
	void setGAMEMAP(Map** GAMEMAP_);

	bool getFinishStatu();
	void setFinishStatu(bool fs);

	bool GetStart = 0;
	std::map<std::string, Json::Value> UserRanking;	//	�������а�
	//vector<tuple<string, int, int>>& GLOBAL_VARIABLE_ranking;
	void InitRankingList();//	�����ר�ã����ڳ�ʼ�������˵����а���Ϣ
	bool IsAllUserFinished();//	�����ר��
	void RemoveUserFromUnfinished(std::string ID);//	�����ר��
	bool IsOnline(SOCKET S);//	�����ר��
	bool IsAnybodyOnline();//	�����ר��
	bool GetOut = 0;

	bool RandomStart = 0;
	Json::Value RandomSetting = Json::nullValue;//	�����ʼ����Ϣ��
	std::string RandomMapID = "";

	bool isWatching = 0;//	��ս״̬
	CMap* watchMap = new CMap();//	��ս��ͼ
	void watchBinder(std::string from, std::string to,bool isUnBind = 0);//	�����ר�ã����ڰ������ս��ϵ
	void ClearWatchList();													//	�����ר�ã���չ�ս�б�
	std::map<std::string, std::set<std::string>>& getWatchingList();//	�����ר��

	int pagedown = 0;//	��ҳ�ã�������Ϣʱ���Զ���ҳ
	Json::Value kickInfo = Json::nullValue;	//�ͻ����ã�����Լ��Ƿ���
	std::set<std::string> quit_ID;	//�˳����˵�ID
private:
	WSADATA wsaData;
	sockaddr_in sockAddr;
	SOCKET mySock;

	MapDisplay* MapDisplayer;	//	
	UserManager* userManager;	//	����userManager�ṩһЩ��Ϣ
	Json::Value tmpJson;			//	�ݴ�һЩ��Ϣ�����뽻��
	std::vector<Map*>* Maps;		//	��ͼ��������ã�
	std::map<int, int>* IDtoMaps;	//	ָ��Maps�±꣨������ã�
	Map** GAMEMAP;					//	ָ��ȫ�ֱ���ָ���ָ��
	
	int port = 1234;
	SocketStatu statu = UNACTIVE;

	bool FATALERROR = 0;
	struct in_addr addr;
	fd_set readfds, writefds, exceptfds;
	timeval timeout;
	std::set<SOCKET> S_pool;//	����˹�������ӳأ�������Ч�ͻ��˵�SOCKET���ϣ�
	std::set<SOCKET> S_quit;//	����˹�����˳��أ��ȴ����������Ҫ��Ϣ������ȫɾ��

	bool sending = 0;
	std::map<SOCKET, std::queue<std::string>> SendMessageList;//	ԭʼ��������Ϣ�б�
	std::map<SOCKET, std::queue<std::string>> RecvMessageList;//	ԭʼ������Ϣ�б�

	bool arranging = 0;
	//		�����ĸ��ͻ���			ʱ���				�ְ���Ϣ�б�
	std::map<SOCKET, std::map<std::string, std::vector<std::string>>> RecvSplitMessage;//		���յ��ķְ������Ϣ����һ�ηְ���
	std::map<SOCKET, std::map<std::string, Json::Value>> RecvPackageMessage;//	����ճ�������Ϣ���͵���Json�ɣ�
	
	bool IsFinish = 0;//	���ͨ��״̬
	std::set<std::string> playerNoFinish;//		�����ר��

	std::map<std::string, std::set<std::string>> WatchingList;	//	�����ר�ã����ڹ����սϵͳ
};