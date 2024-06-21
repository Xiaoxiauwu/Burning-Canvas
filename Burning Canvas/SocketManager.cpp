#include "SocketManager.h"


SocketManager::SocketManager(){
    timeout.tv_sec = 0;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        FATALERROR = 1;
        return;
    }
}

SocketManager::~SocketManager(){
    delete watchMap;
    WSACleanup();
}

//	本地socket创建状态，来判断你是否能进行联网
bool SocketManager::GetErrorStatu()
{
	return FATALERROR;
}

SocketStatu SocketManager::getStatu()
{
    return statu;
}

//  防止using namespace std;占用bind
namespace ATstd {
    int __stdcall FAR bbind(
        _In_ SOCKET s,
        _In_reads_bytes_(namelen) const struct sockaddr FAR* addr,
        _In_ int namelen) {
        return bind(s, addr, namelen);
    }
};

void SocketManager::Server_Start(int port_){
    mySock = socket(PF_INET, SOCK_STREAM, 0);
    unsigned long mode = 1;
    ioctlsocket(mySock, FIONBIO, &mode);

    if (mySock == INVALID_SOCKET) {
        FATALERROR = 1;
        return;
    }

    if (port_ == -1)port_ = port;
    //sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(port_);
    sockAddr.sin_addr.s_addr = inet_addr(GetLocalHost().c_str());

    //应该不会出错的
    if (ATstd::bbind(mySock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        FATALERROR = 1;
        return;
    }

    if (listen(mySock, 5) == SOCKET_ERROR)
    {
        FATALERROR = 1;
        return;
    }
    statu = SERVER;

    //  自身信息保存
    UserInfo.clear();
    Json::Value R;
    R["type"] = "USERINFO";
    R["ID"] = userManager->getRawData()["User"][userManager->getName()]["ID"].asString();
    R["name"] = userManager->getName();
    R["Rating"] = userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat();
    R["avator"] = tmpJson;
    UserInfo[userManager->getID()] = R;
}



void SocketManager::Server_Update(){
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    FD_SET(mySock, &readfds);
    FD_SET(mySock, &writefds);
    FD_SET(mySock, &exceptfds);
    for (auto client : S_pool) {
        FD_SET(client, &readfds);
        FD_SET(client, &writefds);
    }
    int activity = select(0, &readfds, &writefds, 0, &timeout);

    if (FD_ISSET(mySock, &readfds)) {// 新的接入事件处理
        SOCKET clntSock = accept(mySock, nullptr, nullptr);//accept(servSock, (SOCKADDR*)&clntAddr, &nSize);
        S_pool.insert(clntSock);
        auto [ip, prt] = GetClientAddrAndPort(clntSock);
        //std::cout << "[" << ip << ":" << prt << "]" << "接入连接\n";
        SendMessageList[clntSock];
        RecvMessageList[clntSock];
    }

    std::set<SOCKET> del;
    for (auto client : S_pool) {
        if (activity > 0 && FD_ISSET(client, &readfds)) {
            char szBuffer[MAXBYTE] = { 0 };
            //char szBuffer[0xf] = { 0 };
            int k = recv(client, szBuffer, MAXBYTE, NULL);
            //int k = recv(client, szBuffer, 0xf, NULL);
            //cout << "有数据：" << k;
            if (strcmp(szBuffer, "") == 0) {//  处理断开连接时的操作
                del.insert(client);

                if (getStatu() == SERVER && SocketToID.count(client)) {
                    RemoveUserFromUnfinished(SocketToID[client]);// 从当前游戏中踢出该玩家
                    //RecvMessageList[client].push("[QUIT]");
                    auto [ip, prt] = GetClientAddrAndPort(client);
                    //std::cout << "[" << ip << ":" << prt << "]" << "断开连接\n";
                    quit_ID.insert(SocketToID[client]);

                    time_t timer;
                    tm t;
                    time(&timer);
                    localtime_s(&t, &timer);
                    stringstream ss;
                    ss << setw(2) << setfill('0') << right << t.tm_hour << ":" << setw(2) << setfill('0') << right << t.tm_min << ":" << setw(2) << setfill('0') << right << t.tm_sec;
                    Json::Value R;
                    R["type"] = "MESSAGE";
                    R["time"] = ss.str();
                    R["from"] = UserInfo[SocketToID[client]]["name"].asString();
                    R["ID"] = SocketToID[client];
                    R["body"] = encode::UTF8_To_String(u8"[ 离开了房间 ]");
                    R["title"] = "client";

                    SplitSender(JSONToEJSON(R));
                    MessageList.push_back(R);
                    pagedown = 1;
                }
            }
            else {
                RecvMessageList[client].push(szBuffer);
                //auto [ip, prt] = GetClientAddrAndPort(client);
                //std::cout << "[" << ip << ":" << prt << "] " << szBuffer << "\n";
                //std::cout << GetPattern(szBuffer) << "\n";
                //cout << "[" << inet_ntoa(sockAddr_.sin_addr) << ":" << sockAddr_.sin_port << "]"
                //    << szBuffer << "\n";
            }
        }

        if (FD_ISSET(client, &writefds)) {
            while (!SendMessageList[client].empty()) {
                std::string msg = SendMessageList[client].front();
                //auto [ip, prt] = GetClientAddrAndPort(mySock);
                //std::cout << "[" << ip << ":" << prt << "] " << msg << "\n";

                //auto [ip_, prt_] = GetClientAddrAndPort(client);
                //std::cout << "正在向[" << ip_ << ":" << prt_ << "]发包" << "\n";
                std::thread t([=]() {
                    //cout << "正在向" << client <<"发送" << msg << '\n';
                    send(client, msg.c_str(), msg.length(), NULL);
                    //cout << "发送完毕\n";
                });
                t.detach();
                SendMessageList[client].pop();
            }
        }
    }

    for (auto i : del) {
        S_pool.erase(i);
        SendMessageList.erase(i);
        //RecvMessageList.erase(i);
        //UserInfo.erase(SocketToID[i]);//  建议别删，因为聊天室必须要有玩家消息才能展示头像
        //delete AvatorInfo[SocketToID[i]];
        //AvatorInfo.erase(SocketToID[i]);
        
        //SocketToID.erase(i);
    }

    if (!arranging) {// 对接收到的消息进行处理
        arranging = 1;
        std::thread t([&]() {
            Arranger();
            arranging = 0;
            });
        t.detach();
    }

    //for (auto [client, llist] : RecvSplitMessage) {//不能在这里写这个，会冲突
    //    for (auto& [time, msgs] : llist) {
    //        for (auto& msg : msgs) {
    //            cout << "<" << time << ">" << msg << "\n";
    //        }
    //    }
    //}
    //RecvSplitMessage.clear();
}

void SocketManager::Server_End(){
    arranging = 0;//    结束消息组包的线程
    for (auto s : S_pool) {
        closesocket(s);
    }
    closesocket(mySock);
    SendMessageList.clear();
    RecvMessageList.clear();
    RecvSplitMessage.clear();
    RecvPackageMessage.clear();
    S_pool.clear();

    MessageList.clear();
    UserInfo.clear();
    for (auto& [id, avator] : AvatorInfo) {
        delete avator;
    }
    AvatorInfo.clear();
    SocketToID.clear();
    IDToSocket.clear();
    quit_ID.clear();

    statu = UNACTIVE;
}

int& SocketManager::getPort()
{
    return port;
}

std::vector<SOCKET> SocketManager::getAllsocket(){
    std::vector<SOCKET> sockets;
    if (statu == CLIENT) {
        for (auto& sock : S_pool) {
            sockets.push_back(sock);
        }
    }
    else if(statu == CLIENT) {
        sockets.push_back(mySock);
    }
    return sockets;
}

//  unused
void SocketManager::Broadcast(std::string msg){
    for (auto& [sock, list] : SendMessageList) {
        list.push(msg);
    }
}

void SocketManager::sendMessage(std::string msg, std::vector<SOCKET> sock, std::set<SOCKET> ignore){
    if (msg == "")return;//     内容不能为空
    if (sock == std::vector<SOCKET>()) {//   填空值就是默认“广播”这个消息了
        for (auto& [sock_,list] : SendMessageList) {
            if (ignore.empty() || !ignore.count(sock_)) {
                list.push(msg);
                //std::cout << "[消息已经装填，正在发送..]\n";
            }
        }
    }
    else {
        for (SOCKET s : sock) {
            if (ignore.empty() || !ignore.count(s)) {
                SendMessageList[s].push(msg);
            }
        }
    }
}

////////////////////////////////////////////////////分包与粘包处理/////////////////////////////////////////////////////////////
//  JSON消息的分包[xxxxxxxxxxssss][1/2:312][JSON][asdgaasdfccxads]
std::vector<std::string> SPLIT(std::string raw,int packageSize = 200) {
    static auto begin = std::chrono::high_resolution_clock::now();
    auto ttt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin);
    //ImGui::Text(std::to_string(ttt.count()).c_str());
    std::vector<std::string> out;
    long long now = (long long)time(0) * 10000000ll + ttt.count() % 10000000;//clock() % 10000000;

    int n = ceil(1.0 * raw.length() / packageSize);
    int sslen = std::to_string(n).length();
    int i = 1;
    while (!raw.empty()) {
        int len = packageSize;
        if (raw.length() <= packageSize) {
            len = raw.length();
        }
        std::stringstream ss;
        ss << setw(sslen) << right << setfill('0') << i;
        out.push_back(
            "[" + std::to_string(now) + "][" 
            + ss.str() + "/" + std::to_string(n) + ":" + std::to_string(len) + "][JSON]["
            + raw.substr(0, len) + "]"
        );
        raw = raw.substr(len);
        i++;
    }
    return out;
}

//  获取第k个[]字段
std::string GetPattern(std::string raw, int k) {
    while (k--) {
        if (raw.find('[') == -1)return "";
        raw = raw.substr(raw.find('[') + 1);
    }
    if (raw.find(']') == -1)return "";
    return raw.substr(0, raw.find(']'));
}

void SocketManager::Arranger() {
    Json::StreamWriterBuilder writebuild;
    writebuild["emitUTF8"] = true;
    while (arranging) {
        for (auto& [client, msgQueue] : RecvMessageList) {
            while (!msgQueue.empty()) {
                if (msgQueue.size() == 1 && GetPattern(msgQueue.front(), 4) == "") {
                    break;
                }
                std::string tmp = msgQueue.front();
                msgQueue.pop();
                if (!msgQueue.empty()) {
                     tmp += msgQueue.front();
                     msgQueue.pop();
                }

                //while()
                //RecvSplitMessage_stack[client].push()

                while (GetPattern(tmp, 4) != "") {
                    std::string msg = "";
                    for (int i = 0; i < 4; i++) {
                        msg += "[" + GetPattern(tmp, 1) + "]";
                        tmp = tmp.substr(tmp.find('[') + 1);
                    }
                    //cout << "{" << msg << "}";
                    //  进行粘包的分包处理
                    RecvSplitMessage[client][GetPattern(msg, 1)].push_back("[" + GetPattern(msg, 2) + "][" + GetPattern(msg, 4) + "]");
                }

                if (tmp != "") {
                    if (msgQueue.empty()) {
                        msgQueue.push(tmp);
                    }
                    else {
                        msgQueue.front() = tmp + msgQueue.front();
                    }
                }
            }
        }
        //这里进行粘包
        for (auto &[client, llist] : RecvSplitMessage) {
            std::vector<std::string> finishList;
            for (auto& [ttime, msgs] : llist) {
                //  只有收集够足够的包，才会开始粘包
                std::string tmp=GetPattern(msgs[0], 1);
                tmp = tmp.substr(tmp.find('/') + 1);
                tmp = tmp.substr(0, tmp.find(':'));
                if (msgs.size() != stoi(tmp)) {
                    //cout << "可惜，收到了" << msgs.size() << "个包，但是一共要" << tmp << "个包\n";
                    //for (auto i : msgs) {
                    //    cout << i << "\n";
                    //}
                    break;
                }
                //  按包标号排序
                std::sort(msgs.begin(), msgs.end());
                std::string ejson = "";
                for (auto& msg : msgs) {
                    //cout << "<" << time << ">" << msg << "\n";
                    //cout << msg;
                    ejson += GetPattern(msg, 2);
                }

                //  解密为JSON，对信息分类
                Json::Value R;
                R = EJSONToJSON(ejson);//   R["JSONERROR"]=1;// json解析出错，发的包有问题
                if (R == Json::nullValue) {
                    R["type"] = "ERROR";
                }
                //if (R["JSONERROR"] != Json::nullValue) {
                //    R["type"] = "ERROR";
                //}

                //cout << R << "\n";

                //if (getStatu() == CLIENT) {
                //    cout << R;
                //}

                if (R["type"] == "MESSAGE") {//             这里写收到用户信息后干嘛
                    MessageList.push_back(R);
                    pagedown = 1;
                    //cout << "收到" << client << "的消息\n";
                    //cout << "接收到一条消息\n";
                    if (getStatu() == SERVER) {//   服务端要做的事
                        SplitSender(ejson);//   把信息广播出去
                    }
                }
                else if (R["type"] == "USERINFO") {//       收到了用户信息更新要求
                    if (getStatu() == SERVER) {
                        if (UserInfo.count(R["ID"].asString()) && !quit_ID.count(R["ID"].asString())) {// 极其特殊的情况，万一用户用同一个存档，自己连自己的设备，会出现ID重复的现象，导致先加入的玩家信息被覆写
                            Json::Value reply;
                            reply["type"] = "KICK";
                            reply["reason"] = encode::UTF8_To_String(u8"你已经进入过该房间了，不能重复进入");
                            reply["comfirm"] = 0;

                            SplitSender(JSONToEJSON(reply), { client });//    踢出该玩家
                        }
                        else if (!IsAllUserFinished()) {
                            Json::Value reply;
                            reply["type"] = "KICK";
                            reply["reason"] = encode::UTF8_To_String(u8"当前房间正在游戏中，请稍后再加入");
                            reply["comfirm"] = 0;

                            SplitSender(JSONToEJSON(reply), { client });//    踢出该玩家
                        }
                        else {
                            if (quit_ID.count(R["ID"].asString())) {
                                quit_ID.erase(R["ID"].asString());
                            }
                            for (auto& [ID, info] : UserInfo) {//   服务端收到了来自一个客户端的用户信息，要先把已有用户信息全发回给给该客户端
                                SplitSender(JSONToEJSON(info), { client });
                            }
                            for (auto& client_ : S_pool) {
                                if (client_ == client)continue;
                                SplitSender(ejson, { client_ });//  广播该客户端的用户信息
                                //cout << "向" << SocketToID[client_] << "广播新用户信息\n";
                            }
                            SocketToID[client] = R["ID"].asString();
                            IDToSocket[R["ID"].asString()] = client;

                            if (!Maps->empty()) {//     向新用户发送“展示”指令
                                Json::Value Reply;
                                Reply["type"] = "DISPLAY";
                                Reply["ID"] = selectMapID;
                                SplitSender(JSONToEJSON(Reply), { client });
                                //cout << "新来的客户端，展示一下这张地图\n"; 
                            }

                            time_t timer;//     广播新用户加入
                            tm t;
                            time(&timer);
                            localtime_s(&t, &timer);
                            stringstream ss;
                            ss << setw(2) << setfill('0') << right << t.tm_hour << ":" << setw(2) << setfill('0') << right << t.tm_min << ":" << setw(2) << setfill('0') << right << t.tm_sec;
                            Json::Value Reply;
                            Reply["type"] = "MESSAGE";
                            Reply["time"] = ss.str();
                            Reply["from"] = R["name"].asString();
                            Reply["ID"] = SocketToID[client];
                            Reply["body"] = encode::UTF8_To_String(u8"[ 进入了房间 ]");
                            Reply["title"] = "client";

                            SplitSender(JSONToEJSON(Reply));
                            MessageList.push_back(Reply);
                            pagedown = 1;

                            UserInfo[R["ID"].asString()] = R;//     最后服务端保存玩家信息
                            CMap* tmp = new CMap();
                            tmp->LoadMapFromJson(R["avator"]);
                            AvatorInfo[R["ID"].asString()] = tmp;
                        }
                    }
                    else {
                        UserInfo[R["ID"].asString()] = R;//     客户端就只保存玩家信息
                        CMap* tmp = new CMap();
                        tmp->LoadMapFromJson(R["avator"]);
                        AvatorInfo[R["ID"].asString()] = tmp;
                    }
                }
                else if (R["type"] == "DISPLAY") {
                    if (!tmpMapInfo.count(R["ID"].asString())) {
                        Json::Value Reply;
                        Reply["type"] = "GETMAP";
                        Reply["ID"] = R["ID"].asString();
                        SplitSender(JSONToEJSON(Reply), { mySock });//  客户端向服务端请求地图数据
                        //cout << "我没有这个地图啊\n";
                    }
                    selectMapID = R["ID"].asString();
                    //cout << selectMapID << "]";
                }
                else if (R["type"] == "GETMAP") {
                    Json::Value Reply = SaveMapAsJson((CMap*)(*Maps)[(*IDtoMaps)[stoi(R["ID"].asString())]]);
                    Reply["type"] = "MAP";
                    SplitSender(JSONToEJSON(Reply), { client });
                    //cout << "发你了，你看看...\n";
                }
                else if (R["type"] == "MAP") {
                    CMap* tmpMap = new CMap();
                    tmpMap->LoadMapFromJson(R);
                    tmpMapInfo[R["ID"].asString()] = tmpMap;
                    //cout << "收到了\n";
                }
                else if (R["type"] == "START") {
                    GetStart = 1;
                    //cout << "收到\n";
                }
                else if (R["type"] == "RANKING") {
                    if (getStatu() == SERVER) {
                        for (auto& client_ : S_pool) {
                            if (client_ == client)continue;
                            SplitSender(ejson, { client_ });//  广播该客户端的用户信息
                        }
                        if (R["isFinish"].asInt() == 1) {
                            playerNoFinish.erase(SocketToID[client]);
                        }

                        if (WatchingList.count(SocketToID[client])) {// 如果有人要观战他的话，问他要地图
                            Json::Value reply;
                            reply["type"] = "GETGAMINGMAP";
                            SplitSender(JSONToEJSON(reply), {client});//
                            //cout << "我向你要地图\n";
                        }
                    }
                    UserRanking[R["ID"].asString()] = R;
                }
                else if (R["type"] == "FINALSTANDING") {
                    if (R["discardThis"] == Json::nullValue) {
                        MessageList.push_back(R);
                        pagedown = 1;
                    }
                    GetOut = 1;
                    //cout << "收到排行榜信息\n";
                }
                else if (R["type"] == "WATCH") {//  服务端绑定观战关系
                    if (R["from"].asString() == R["to"].asString()) {
                        watchBinder(R["from"].asString(), R["to"].asString(), 1);
                    }
                    else {
                        watchBinder(R["from"].asString(), R["to"].asString());

                        Json::Value reply;
                        reply["type"] = "GETGAMINGMAP";
                        SplitSender(JSONToEJSON(reply), { IDToSocket[R["to"].asString()] });//
                    }
                }
                else if (R["type"] == "GETGAMINGMAP") {//   客户端收到获取地图的指令
                    //cout << "收到，正在回复地图\n";
                    std::thread t([&]() {
                        Json::Value reply;

                        while ((*GAMEMAP) == nullptr ||(*GAMEMAP)->isEditing);//cout<<"被占用的地图\n";
                        (*GAMEMAP)->isEditing = 1;
                        reply = SaveMapAsJson((CMap*)*GAMEMAP);
                        (*GAMEMAP)->isEditing = 0;

                        reply["type"] = "MAP_FOR_WATCH_server";
                        //cout << reply["x"] << " " << reply["y"];
                        SplitSender(JSONToEJSON(reply));//  回应服务端
                        //cout << "发过去了\n";
                    });

                    t.detach();

                    //cout << R["x"] << " " << R["y"];
                }
                else if (R["type"] == "MAP_FOR_WATCH_server") {//   服务端转发
                    R["type"] = "MAP_FOR_WATCH";
                    //cout << R["x"] << " " << R["y"];
                    //cout << "我拿到了你的地图了，正在转发给有需要的人\n";
                    for (std::string from: WatchingList[SocketToID[client]]) {
                        if (from == userManager->getID()) {
                            //  我自己也想看别人的地图
                            //cout << "我自己想看\n";
                            std::thread t([&](Json::Value R) {
                                while (watchMap->isEditing);
                                watchMap->isEditing = 1;
                                //cout << R["x"] << " " << R["y"];
                                watchMap->LoadMapFromJson(R);
                                watchMap->isEditing = 0;
                            }, R);
                            t.detach();
                        }
                        else {
                            //cout << "还有别人想看\n";
                            SplitSender(JSONToEJSON(R), { IDToSocket[from] });
                        }
                    }
                }
                else if (R["type"] == "MAP_FOR_WATCH") {//  客户端收到观战的地图信息
                    std::thread t([&](Json::Value R) {
                        while (watchMap->isEditing);
                        watchMap->isEditing = 1;
                        watchMap->LoadMapFromJson(R);
                        watchMap->isEditing = 0;
                    }, R);

                    t.detach();
                }
                else if (R["type"] == "SHARE") {
                    if (getStatu() == SERVER) {
                        for (auto& client_ : S_pool) {
                            if (client_ == client)continue;
                            SplitSender(ejson, { client_ });//  广播该客户端的信息
                        }
                    }
                    MessageList.push_back(R);
                    }
                else if (R["type"] == "RANDOMSTART") {
                    if (RandomMapID != "") {
                        delete tmpMapInfo[RandomMapID];
                        tmpMapInfo.erase(RandomMapID);
                    }
                    RandomMapID = to_string(R["ID"].asInt());
                    tmpMapInfo[RandomMapID] = new CMap();
                    tmpMapInfo[RandomMapID]->LoadMapFromJson(R);

                    RandomStart = 1;
                }
                else if (R["type"] == "KICK" && getStatu() == CLIENT) {
                    kickInfo = R;
                }
                //  记得删
                finishList.push_back(ttime);
            }

            for (std::string ttime : finishList) {
                llist.erase(ttime);
            }
        }
        //RecvSplitMessage.clear();
    }
}

// unused 
std::vector<std::string> R_split(std::string raw) {
    std::vector<std::string> list;
    while (!raw.empty()) {
        std::string tmp = "";
        //for (int i = 0; i < 4);
    }
    return std::vector<std::string>();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SocketManager::SplitSender(std::string msg, std::vector<SOCKET> sock, std::set<SOCKET> ignore, bool type){
    if (type == 0) {//  JSON文本消息（加密）
        for (std::string seg : SPLIT(msg, 200)) {// 200
            sendMessage(seg, sock, ignore);
        }
    }
    else {//    文件消息
        //  缺省，到时候用到再写
    }
}

void SocketManager::Client_Start(std::string IP, int port_){
    //std::cout << "设置：" << IP << ":" << port_ << "\n";
    mySock = socket(PF_INET, SOCK_STREAM, 0);
    unsigned long mode = 1;

    memset(&sockAddr, 0, sizeof(sockAddr));	 
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(port_);
    sockAddr.sin_addr.s_addr = inet_addr(IP.c_str());
    statu = CLIENT_CONNECTING;

    SendMessageList[mySock];//客户端要在发送信息列表里注册一下自己
    RecvMessageList[mySock];

    //cout << "[" << mySock << "]";// DEBUG
}

//  sign为0代表正在发送连接请求，为1代表成功连接，为2代表用户自行取消连接
void SocketManager::Client_ConnetToServer(int* signal){//  用一个信号来传递连接状态
    while (*signal == 0) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        FD_SET(mySock, &readfds);//	是否可读
        FD_SET(mySock, &writefds);//	是否可写/建立连接

        timeval timeout;
        timeout.tv_sec = 1;
        //std::cout << "正在向["<< inet_ntoa(sockAddr.sin_addr) <<":"<<sockAddr.sin_port << "]请求建立连接..\n";

        int state = connect(mySock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
        int activity = select(0, &readfds, &writefds, 0, &timeout);
        if (FD_ISSET(mySock, &writefds)) {
            *signal = 1;
            //std::cout << "连接成功\n";

            //  自身信息保存
            UserInfo.clear();
            Json::Value R;
            R["type"] = "USERINFO";
            R["ID"] = userManager->getRawData()["User"][userManager->getName()]["ID"].asString();
            R["name"] = userManager->getName();
            R["Rating"] = userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat();
            R["avator"] = tmpJson;
            UserInfo[userManager->getID()] = R;
            
            statu = CLIENT;

            //  向服务端发送自己的信息
            Json::StreamWriterBuilder writebuild;
            writebuild["emitUTF8"] = true;
            string document = Json::writeString(writebuild, R);

            SplitSender(ENCODE(document), { mySock });
            break;
            //cout << "ok";//"[" << inet_ntoa(sockAddr_.sin_addr) << ":" << sockAddr_.sin_port << "]" << "接入连接\n";
        }
        Sleep(800);
    }
    if (statu != CLIENT) {
        statu = UNACTIVE;
    }
}

void SocketManager::Client_Update(){
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    FD_SET(mySock, &readfds);
    FD_SET(mySock, &writefds);

    int activity = select(0, &readfds, &writefds, 0, &timeout);

    if (activity > 0 && FD_ISSET(mySock, &readfds)) {
        sockaddr_in sockAddr_;
        int len = sizeof(sockAddr_);
        memset(&sockAddr_, 0, sizeof(sockAddr_));
        getpeername(mySock, (SOCKADDR*)&sockAddr_, &len);

        char szBuffer[MAXBYTE] = { 0 };
        int k = recv(mySock, szBuffer, MAXBYTE, NULL);
        //cout << "有数据：" << k;
        if (strcmp(szBuffer, "") == 0) {
            //del.insert(client);
            auto [ip, prt] = GetClientAddrAndPort(mySock);
            //std::cout << "[" << ip << ":" << prt << "] " << "断开连接\n";
            //break;
            //RecvMessageList[mySock].push("[QUIT]");
            Client_End();
        }
        else {
            //auto [ip, prt] = GetClientAddrAndPort(mySock);
            //std::cout << "[" << ip << ":" << prt << "] " << szBuffer << "\n";
            RecvMessageList[mySock].push(szBuffer);
        }
    }

    if (FD_ISSET(mySock, &writefds)) {
        while (!SendMessageList[mySock].empty()) {

            std::string msg = SendMessageList[mySock].front();
            //auto [ip, prt] = GetClientAddrAndPort(mySock);
            //std::cout << "[" << GetLocalHost() << ":" << port << "] " << msg << "\n";

            //auto [ip_, prt_] = GetClientAddrAndPort(mySock);
            //std::cout << "正在向[" << ip_ << ":" << prt_ << "]发包" << "\n";
            std::thread t([=](){
                send(mySock, msg.c_str(), msg.length(), NULL);
            });
            t.detach();
            SendMessageList[mySock].pop();
        }
        //while (!SendMessageList[mySock].empty()) {
        //    std::string msg = SendMessageList[mySock].front();
        //    send(mySock, msg.c_str(), msg.length(), NULL);
        //    SendMessageList[mySock].pop();
        //}
    }

    if (!arranging) {
        arranging = 1;
        std::thread t([&]() {
            Arranger();
            arranging = 0;
            });
        t.detach();
    }

    //for (auto& [client, llist] : RecvSplitMessage) {
    //    for (auto& [time, msgs] : llist) {
    //        for (auto& msg : msgs) {
    //            cout << "<" << time << ">" << msg << "\n";
    //        }
    //    }
    //}
    //RecvSplitMessage.clear();
}

void SocketManager::Client_End(){
    arranging = 0;//    结束消息组包的线程
    closesocket(mySock);
    SendMessageList.clear();
    RecvMessageList.clear();
    RecvSplitMessage.clear();
    RecvPackageMessage.clear();

    MessageList.clear();
    UserInfo.clear();
    for (auto& [id, avator] : AvatorInfo) {
        if (avator != nullptr) {
            delete avator;
        }
    }
    SocketToID.clear();
    for (auto& [id, map] : tmpMapInfo) {
        delete map;
    }
    tmpMapInfo.clear();
    statu = UNACTIVE;
}

void SocketManager::Update(){
    if (statu != SERVER && statu != CLIENT)return;
    if (statu == SERVER) {
        Server_Update();
    }
    else {
        Client_Update();
    }
}

void SocketManager::End(){
    if (statu != SERVER && statu != CLIENT)return;
    if (statu == SERVER) {
        Server_End();
    }
    else {
        Client_End();
    }
}

std::string SocketManager::GetLocalHost()
{
    //万一用户网络环境变更了呢？
    //if (localAddr != "")return localAddr;
    char host_name[255];
    gethostname(host_name, sizeof(host_name));
    //cout << host_name << "\n";
    struct hostent* phe = gethostbyname(host_name);
    std::string out;
    for (int i = 0; phe->h_addr_list[i]; i++) {
        memcpy(&addr, phe->h_addr_list[i], sizeof(phe->h_addr_list));
        //std::cout << inet_ntoa(addr) << "\n";
        out = std::string(inet_ntoa(addr));
    }
    //localAddr = inet_ntoa(addr);
    return out;
}

std::pair<std::string, int> SocketManager::GetClientAddrAndPort(SOCKET sock)
{
    sockaddr_in sockAddr_;
    int len = sizeof(sockAddr_);
    memset(&sockAddr_, 0, sizeof(sockAddr_));
    getpeername(sock, (SOCKADDR*)&sockAddr_, &len);
    //cout << "[" << inet_ntoa(sockAddr_.sin_addr) << ":" << sockAddr_.sin_port << "]" << "接入连接\n";
    return std::make_pair(inet_ntoa(sockAddr_.sin_addr), sockAddr_.sin_port);
}

std::vector<Json::Value>& SocketManager::getMessageList()
{
    return MessageList;
}

void SocketManager::setUserManager(UserManager* userManager_)
{
    userManager = userManager_;
}

void SocketManager::setMapDisplayer(MapDisplay* MapDisplayer_)
{
    MapDisplayer = MapDisplayer_;
}

void SocketManager::setMaps(std::vector<Map*>* Maps_)
{
    Maps = Maps_;
}

void SocketManager::setIDtoMaps(std::map<int, int>* IDtoMaps_)
{
    IDtoMaps = IDtoMaps_;
}

void SocketManager::setTmp(Json::Value tmp_)
{
    tmpJson = tmp_;
    //cout << tmpJson;
}

void SocketManager::setGAMEMAP(Map** GAMEMAP_)
{
    GAMEMAP = GAMEMAP_;
}

bool SocketManager::getFinishStatu()
{
    return IsFinish;
}

void SocketManager::setFinishStatu(bool fs)
{
    IsFinish = fs;
    //cout << "设置了状态" << IsFinish << "\n";
}

void SocketManager::InitRankingList()
{
    //Json::Value R;
    //R["type"] = "RANKING";
    //R["penelty"] = 0;
    //R["time"] = 0;
    //R["isFinish"] = 0;//    0代表未通关，1代表通关，-1代表离开（未完成）
    //for (auto& client : S_pool) {
    //    R["ID"] = SocketToID[client];
    //    SplitSender(JSONToEJSON(R));
    //}
    //R["ID"] = userManager->getID();
    //SplitSender(JSONToEJSON(R));
    //UserRanking[userManager->getID()] = R;
    playerNoFinish.clear();
    for (auto& client : S_pool) {
        playerNoFinish.insert(SocketToID[client]);
    }
    playerNoFinish.insert(userManager->getID());
}

bool SocketManager::IsAllUserFinished()
{
    return playerNoFinish.empty();
}

void SocketManager::RemoveUserFromUnfinished(std::string ID)
{
    if (playerNoFinish.count(ID)) {
        playerNoFinish.erase(ID);
    }
}

bool SocketManager::IsOnline(SOCKET S)
{
    return S_pool.count(S);
}

bool SocketManager::IsAnybodyOnline()
{
    return !S_pool.empty();
}

void SocketManager::watchBinder(std::string from, std::string to, bool isUnBind)
{
    if (!isUnBind) {
        WatchingList[to].insert(from);
    }
    else {
        std::string del = "";
        for (auto& [TO, FROM_SET] : WatchingList) {
            if (FROM_SET.count(from)) {
                del = TO;
                break;//    一个玩家一次只能观战一个人
            }
        }
        if (del != "") {
            WatchingList[del].erase(from);
            if (WatchingList[del].empty()) {
                WatchingList.erase(del);
            }
        }
    }
}

void SocketManager::ClearWatchList()
{
    WatchingList.clear();
}

std::map<std::string, std::set<std::string>>& SocketManager::getWatchingList()
{
    return WatchingList;
}
