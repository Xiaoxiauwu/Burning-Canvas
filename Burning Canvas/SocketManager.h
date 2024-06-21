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
	SocketManager();//	懒得设置成单例模式了
	~SocketManager();
	bool GetErrorStatu();//	获取本地连接状态
	SocketStatu getStatu();

	void Server_Start(int port_ = -1);//	以指定端口启动服务器
	void Server_Update();//	启动服务端事件监听，发送，接收
	void Server_End();//	关闭服务端

	void Client_Start(std::string IP, int port_);//	以指定端口启动服务器
	void Client_ConnetToServer(int* signal);//	建立连接
	void Client_Update();//	启动客户端事件监听，发送，接收
	void Client_End();//	关闭客户端

	void Update();
	void End();

	int& getPort();
	std::vector<SOCKET> getAllsocket();
	void Broadcast(std::string msg);//	广播消息(服务端专属（？），当然可以试试让服务端转发来自客户端的消息)
	void sendMessage(std::string msg, std::vector<SOCKET> sock = std::vector<SOCKET>(), std::set<SOCKET> ignore = std::set<SOCKET>());
	void Arranger();
	//	单发消息
	void SplitSender(std::string msg, std::vector<SOCKET> sock = std::vector<SOCKET>(), std::set<SOCKET> ignore = std::set<SOCKET>(), bool type = 0);//	字符串/文件分包
	std::string GetLocalHost();
	std::pair<std::string,int> GetClientAddrAndPort(SOCKET sock);

	std::vector<Json::Value>& getMessageList();

	std::vector<Json::Value> MessageList;		//	用户消息列表(记得限定消息条数保存大小)
	std::map<std::string, Json::Value> UserInfo;//	用户信息，按ID作为索引
	std::map<std::string, CMap*> AvatorInfo;		//	用户头像信息，按ID索引，记得delete
	std::map<SOCKET, std::string> SocketToID;	//	服务端专用，服务端管理所有socket，不需要客户端管
	std::map<std::string, SOCKET> IDToSocket;	//	服务端专用，服务端管理所有socket，不需要客户端管
	
	std::map<std::string, CMap*> tmpMapInfo;			//	接收缓存用地图
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
	std::map<std::string, Json::Value> UserRanking;	//	加载排行榜
	//vector<tuple<string, int, int>>& GLOBAL_VARIABLE_ranking;
	void InitRankingList();//	服务端专用，用于初始化所有人的排行榜信息
	bool IsAllUserFinished();//	服务端专用
	void RemoveUserFromUnfinished(std::string ID);//	服务端专用
	bool IsOnline(SOCKET S);//	服务端专用
	bool IsAnybodyOnline();//	服务端专用
	bool GetOut = 0;

	bool RandomStart = 0;
	Json::Value RandomSetting = Json::nullValue;//	随机开始的信息集
	std::string RandomMapID = "";

	bool isWatching = 0;//	观战状态
	CMap* watchMap = new CMap();//	观战地图
	void watchBinder(std::string from, std::string to,bool isUnBind = 0);//	服务端专用，用于绑定与解绑观战关系
	void ClearWatchList();													//	服务端专用，清空观战列表
	std::map<std::string, std::set<std::string>>& getWatchingList();//	服务端专用

	int pagedown = 0;//	翻页用，有新消息时，自动翻页
	Json::Value kickInfo = Json::nullValue;	//客户端用，检查自己是否被踢
	std::set<std::string> quit_ID;	//退出的人的ID
private:
	WSADATA wsaData;
	sockaddr_in sockAddr;
	SOCKET mySock;

	MapDisplay* MapDisplayer;	//	
	UserManager* userManager;	//	急需userManager提供一些信息
	Json::Value tmpJson;			//	暂存一些信息，用与交换
	std::vector<Map*>* Maps;		//	地图（服务端用）
	std::map<int, int>* IDtoMaps;	//	指向Maps下标（服务端用）
	Map** GAMEMAP;					//	指向全局变量指针的指针
	
	int port = 1234;
	SocketStatu statu = UNACTIVE;

	bool FATALERROR = 0;
	struct in_addr addr;
	fd_set readfds, writefds, exceptfds;
	timeval timeout;
	std::set<SOCKET> S_pool;//	服务端管理的连接池（真正有效客户端的SOCKET集合）
	std::set<SOCKET> S_quit;//	服务端管理的退出池，等待程序处理完必要信息后再完全删除

	bool sending = 0;
	std::map<SOCKET, std::queue<std::string>> SendMessageList;//	原始待发送消息列表
	std::map<SOCKET, std::queue<std::string>> RecvMessageList;//	原始接收消息列表

	bool arranging = 0;
	//		来自哪个客户端			时间戳				分包信息列表
	std::map<SOCKET, std::map<std::string, std::vector<std::string>>> RecvSplitMessage;//		接收到的分包后的消息（第一次分包）
	std::map<SOCKET, std::map<std::string, Json::Value>> RecvPackageMessage;//	处理粘包后的消息（就当是Json吧）
	
	bool IsFinish = 0;//	玩家通关状态
	std::set<std::string> playerNoFinish;//		服务端专用

	std::map<std::string, std::set<std::string>> WatchingList;	//	服务端专用，用于管理观战系统
};