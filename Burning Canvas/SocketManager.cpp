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

//	����socket����״̬�����ж����Ƿ��ܽ�������
bool SocketManager::GetErrorStatu()
{
	return FATALERROR;
}

SocketStatu SocketManager::getStatu()
{
    return statu;
}

//  ��ֹusing namespace std;ռ��bind
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

    //Ӧ�ò�������
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

    //  ������Ϣ����
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

    if (FD_ISSET(mySock, &readfds)) {// �µĽ����¼�����
        SOCKET clntSock = accept(mySock, nullptr, nullptr);//accept(servSock, (SOCKADDR*)&clntAddr, &nSize);
        S_pool.insert(clntSock);
        auto [ip, prt] = GetClientAddrAndPort(clntSock);
        //std::cout << "[" << ip << ":" << prt << "]" << "��������\n";
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
            //cout << "�����ݣ�" << k;
            if (strcmp(szBuffer, "") == 0) {//  ����Ͽ�����ʱ�Ĳ���
                del.insert(client);

                if (getStatu() == SERVER && SocketToID.count(client)) {
                    RemoveUserFromUnfinished(SocketToID[client]);// �ӵ�ǰ��Ϸ���߳������
                    //RecvMessageList[client].push("[QUIT]");
                    auto [ip, prt] = GetClientAddrAndPort(client);
                    //std::cout << "[" << ip << ":" << prt << "]" << "�Ͽ�����\n";
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
                    R["body"] = encode::UTF8_To_String(u8"[ �뿪�˷��� ]");
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
                //std::cout << "������[" << ip_ << ":" << prt_ << "]����" << "\n";
                std::thread t([=]() {
                    //cout << "������" << client <<"����" << msg << '\n';
                    send(client, msg.c_str(), msg.length(), NULL);
                    //cout << "�������\n";
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
        //UserInfo.erase(SocketToID[i]);//  �����ɾ����Ϊ�����ұ���Ҫ�������Ϣ����չʾͷ��
        //delete AvatorInfo[SocketToID[i]];
        //AvatorInfo.erase(SocketToID[i]);
        
        //SocketToID.erase(i);
    }

    if (!arranging) {// �Խ��յ�����Ϣ���д���
        arranging = 1;
        std::thread t([&]() {
            Arranger();
            arranging = 0;
            });
        t.detach();
    }

    //for (auto [client, llist] : RecvSplitMessage) {//����������д��������ͻ
    //    for (auto& [time, msgs] : llist) {
    //        for (auto& msg : msgs) {
    //            cout << "<" << time << ">" << msg << "\n";
    //        }
    //    }
    //}
    //RecvSplitMessage.clear();
}

void SocketManager::Server_End(){
    arranging = 0;//    ������Ϣ������߳�
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
    if (msg == "")return;//     ���ݲ���Ϊ��
    if (sock == std::vector<SOCKET>()) {//   ���ֵ����Ĭ�ϡ��㲥�������Ϣ��
        for (auto& [sock_,list] : SendMessageList) {
            if (ignore.empty() || !ignore.count(sock_)) {
                list.push(msg);
                //std::cout << "[��Ϣ�Ѿ�װ����ڷ���..]\n";
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

////////////////////////////////////////////////////�ְ���ճ������/////////////////////////////////////////////////////////////
//  JSON��Ϣ�ķְ�[xxxxxxxxxxssss][1/2:312][JSON][asdgaasdfccxads]
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

//  ��ȡ��k��[]�ֶ�
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
                    //  ����ճ���ķְ�����
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
        //�������ճ��
        for (auto &[client, llist] : RecvSplitMessage) {
            std::vector<std::string> finishList;
            for (auto& [ttime, msgs] : llist) {
                //  ֻ���ռ����㹻�İ����ŻῪʼճ��
                std::string tmp=GetPattern(msgs[0], 1);
                tmp = tmp.substr(tmp.find('/') + 1);
                tmp = tmp.substr(0, tmp.find(':'));
                if (msgs.size() != stoi(tmp)) {
                    //cout << "��ϧ���յ���" << msgs.size() << "����������һ��Ҫ" << tmp << "����\n";
                    //for (auto i : msgs) {
                    //    cout << i << "\n";
                    //}
                    break;
                }
                //  �����������
                std::sort(msgs.begin(), msgs.end());
                std::string ejson = "";
                for (auto& msg : msgs) {
                    //cout << "<" << time << ">" << msg << "\n";
                    //cout << msg;
                    ejson += GetPattern(msg, 2);
                }

                //  ����ΪJSON������Ϣ����
                Json::Value R;
                R = EJSONToJSON(ejson);//   R["JSONERROR"]=1;// json�����������İ�������
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

                if (R["type"] == "MESSAGE") {//             ����д�յ��û���Ϣ�����
                    MessageList.push_back(R);
                    pagedown = 1;
                    //cout << "�յ�" << client << "����Ϣ\n";
                    //cout << "���յ�һ����Ϣ\n";
                    if (getStatu() == SERVER) {//   �����Ҫ������
                        SplitSender(ejson);//   ����Ϣ�㲥��ȥ
                    }
                }
                else if (R["type"] == "USERINFO") {//       �յ����û���Ϣ����Ҫ��
                    if (getStatu() == SERVER) {
                        if (UserInfo.count(R["ID"].asString()) && !quit_ID.count(R["ID"].asString())) {// ����������������һ�û���ͬһ���浵���Լ����Լ����豸�������ID�ظ������󣬵����ȼ���������Ϣ����д
                            Json::Value reply;
                            reply["type"] = "KICK";
                            reply["reason"] = encode::UTF8_To_String(u8"���Ѿ�������÷����ˣ������ظ�����");
                            reply["comfirm"] = 0;

                            SplitSender(JSONToEJSON(reply), { client });//    �߳������
                        }
                        else if (!IsAllUserFinished()) {
                            Json::Value reply;
                            reply["type"] = "KICK";
                            reply["reason"] = encode::UTF8_To_String(u8"��ǰ����������Ϸ�У����Ժ��ټ���");
                            reply["comfirm"] = 0;

                            SplitSender(JSONToEJSON(reply), { client });//    �߳������
                        }
                        else {
                            if (quit_ID.count(R["ID"].asString())) {
                                quit_ID.erase(R["ID"].asString());
                            }
                            for (auto& [ID, info] : UserInfo) {//   ������յ�������һ���ͻ��˵��û���Ϣ��Ҫ�Ȱ������û���Ϣȫ���ظ����ÿͻ���
                                SplitSender(JSONToEJSON(info), { client });
                            }
                            for (auto& client_ : S_pool) {
                                if (client_ == client)continue;
                                SplitSender(ejson, { client_ });//  �㲥�ÿͻ��˵��û���Ϣ
                                //cout << "��" << SocketToID[client_] << "�㲥���û���Ϣ\n";
                            }
                            SocketToID[client] = R["ID"].asString();
                            IDToSocket[R["ID"].asString()] = client;

                            if (!Maps->empty()) {//     �����û����͡�չʾ��ָ��
                                Json::Value Reply;
                                Reply["type"] = "DISPLAY";
                                Reply["ID"] = selectMapID;
                                SplitSender(JSONToEJSON(Reply), { client });
                                //cout << "�����Ŀͻ��ˣ�չʾһ�����ŵ�ͼ\n"; 
                            }

                            time_t timer;//     �㲥���û�����
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
                            Reply["body"] = encode::UTF8_To_String(u8"[ �����˷��� ]");
                            Reply["title"] = "client";

                            SplitSender(JSONToEJSON(Reply));
                            MessageList.push_back(Reply);
                            pagedown = 1;

                            UserInfo[R["ID"].asString()] = R;//     ������˱��������Ϣ
                            CMap* tmp = new CMap();
                            tmp->LoadMapFromJson(R["avator"]);
                            AvatorInfo[R["ID"].asString()] = tmp;
                        }
                    }
                    else {
                        UserInfo[R["ID"].asString()] = R;//     �ͻ��˾�ֻ���������Ϣ
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
                        SplitSender(JSONToEJSON(Reply), { mySock });//  �ͻ��������������ͼ����
                        //cout << "��û�������ͼ��\n";
                    }
                    selectMapID = R["ID"].asString();
                    //cout << selectMapID << "]";
                }
                else if (R["type"] == "GETMAP") {
                    Json::Value Reply = SaveMapAsJson((CMap*)(*Maps)[(*IDtoMaps)[stoi(R["ID"].asString())]]);
                    Reply["type"] = "MAP";
                    SplitSender(JSONToEJSON(Reply), { client });
                    //cout << "�����ˣ��㿴��...\n";
                }
                else if (R["type"] == "MAP") {
                    CMap* tmpMap = new CMap();
                    tmpMap->LoadMapFromJson(R);
                    tmpMapInfo[R["ID"].asString()] = tmpMap;
                    //cout << "�յ���\n";
                }
                else if (R["type"] == "START") {
                    GetStart = 1;
                    //cout << "�յ�\n";
                }
                else if (R["type"] == "RANKING") {
                    if (getStatu() == SERVER) {
                        for (auto& client_ : S_pool) {
                            if (client_ == client)continue;
                            SplitSender(ejson, { client_ });//  �㲥�ÿͻ��˵��û���Ϣ
                        }
                        if (R["isFinish"].asInt() == 1) {
                            playerNoFinish.erase(SocketToID[client]);
                        }

                        if (WatchingList.count(SocketToID[client])) {// �������Ҫ��ս���Ļ�������Ҫ��ͼ
                            Json::Value reply;
                            reply["type"] = "GETGAMINGMAP";
                            SplitSender(JSONToEJSON(reply), {client});//
                            //cout << "������Ҫ��ͼ\n";
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
                    //cout << "�յ����а���Ϣ\n";
                }
                else if (R["type"] == "WATCH") {//  ����˰󶨹�ս��ϵ
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
                else if (R["type"] == "GETGAMINGMAP") {//   �ͻ����յ���ȡ��ͼ��ָ��
                    //cout << "�յ������ڻظ���ͼ\n";
                    std::thread t([&]() {
                        Json::Value reply;

                        while ((*GAMEMAP) == nullptr ||(*GAMEMAP)->isEditing);//cout<<"��ռ�õĵ�ͼ\n";
                        (*GAMEMAP)->isEditing = 1;
                        reply = SaveMapAsJson((CMap*)*GAMEMAP);
                        (*GAMEMAP)->isEditing = 0;

                        reply["type"] = "MAP_FOR_WATCH_server";
                        //cout << reply["x"] << " " << reply["y"];
                        SplitSender(JSONToEJSON(reply));//  ��Ӧ�����
                        //cout << "����ȥ��\n";
                    });

                    t.detach();

                    //cout << R["x"] << " " << R["y"];
                }
                else if (R["type"] == "MAP_FOR_WATCH_server") {//   �����ת��
                    R["type"] = "MAP_FOR_WATCH";
                    //cout << R["x"] << " " << R["y"];
                    //cout << "���õ�����ĵ�ͼ�ˣ�����ת��������Ҫ����\n";
                    for (std::string from: WatchingList[SocketToID[client]]) {
                        if (from == userManager->getID()) {
                            //  ���Լ�Ҳ�뿴���˵ĵ�ͼ
                            //cout << "���Լ��뿴\n";
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
                            //cout << "���б����뿴\n";
                            SplitSender(JSONToEJSON(R), { IDToSocket[from] });
                        }
                    }
                }
                else if (R["type"] == "MAP_FOR_WATCH") {//  �ͻ����յ���ս�ĵ�ͼ��Ϣ
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
                            SplitSender(ejson, { client_ });//  �㲥�ÿͻ��˵���Ϣ
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
                //  �ǵ�ɾ
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
    if (type == 0) {//  JSON�ı���Ϣ�����ܣ�
        for (std::string seg : SPLIT(msg, 200)) {// 200
            sendMessage(seg, sock, ignore);
        }
    }
    else {//    �ļ���Ϣ
        //  ȱʡ����ʱ���õ���д
    }
}

void SocketManager::Client_Start(std::string IP, int port_){
    //std::cout << "���ã�" << IP << ":" << port_ << "\n";
    mySock = socket(PF_INET, SOCK_STREAM, 0);
    unsigned long mode = 1;

    memset(&sockAddr, 0, sizeof(sockAddr));	 
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(port_);
    sockAddr.sin_addr.s_addr = inet_addr(IP.c_str());
    statu = CLIENT_CONNECTING;

    SendMessageList[mySock];//�ͻ���Ҫ�ڷ�����Ϣ�б���ע��һ���Լ�
    RecvMessageList[mySock];

    //cout << "[" << mySock << "]";// DEBUG
}

//  signΪ0�������ڷ�����������Ϊ1����ɹ����ӣ�Ϊ2�����û�����ȡ������
void SocketManager::Client_ConnetToServer(int* signal){//  ��һ���ź�����������״̬
    while (*signal == 0) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        FD_SET(mySock, &readfds);//	�Ƿ�ɶ�
        FD_SET(mySock, &writefds);//	�Ƿ��д/��������

        timeval timeout;
        timeout.tv_sec = 1;
        //std::cout << "������["<< inet_ntoa(sockAddr.sin_addr) <<":"<<sockAddr.sin_port << "]����������..\n";

        int state = connect(mySock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
        int activity = select(0, &readfds, &writefds, 0, &timeout);
        if (FD_ISSET(mySock, &writefds)) {
            *signal = 1;
            //std::cout << "���ӳɹ�\n";

            //  ������Ϣ����
            UserInfo.clear();
            Json::Value R;
            R["type"] = "USERINFO";
            R["ID"] = userManager->getRawData()["User"][userManager->getName()]["ID"].asString();
            R["name"] = userManager->getName();
            R["Rating"] = userManager->getRawData()["User"][userManager->getName()]["Rating"].asFloat();
            R["avator"] = tmpJson;
            UserInfo[userManager->getID()] = R;
            
            statu = CLIENT;

            //  �����˷����Լ�����Ϣ
            Json::StreamWriterBuilder writebuild;
            writebuild["emitUTF8"] = true;
            string document = Json::writeString(writebuild, R);

            SplitSender(ENCODE(document), { mySock });
            break;
            //cout << "ok";//"[" << inet_ntoa(sockAddr_.sin_addr) << ":" << sockAddr_.sin_port << "]" << "��������\n";
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
        //cout << "�����ݣ�" << k;
        if (strcmp(szBuffer, "") == 0) {
            //del.insert(client);
            auto [ip, prt] = GetClientAddrAndPort(mySock);
            //std::cout << "[" << ip << ":" << prt << "] " << "�Ͽ�����\n";
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
            //std::cout << "������[" << ip_ << ":" << prt_ << "]����" << "\n";
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
    arranging = 0;//    ������Ϣ������߳�
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
    //��һ�û����绷��������أ�
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
    //cout << "[" << inet_ntoa(sockAddr_.sin_addr) << ":" << sockAddr_.sin_port << "]" << "��������\n";
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
    //cout << "������״̬" << IsFinish << "\n";
}

void SocketManager::InitRankingList()
{
    //Json::Value R;
    //R["type"] = "RANKING";
    //R["penelty"] = 0;
    //R["time"] = 0;
    //R["isFinish"] = 0;//    0����δͨ�أ�1����ͨ�أ�-1�����뿪��δ��ɣ�
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
                break;//    һ�����һ��ֻ�ܹ�սһ����
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
