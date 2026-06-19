#include "RakNetClient.h"


void RakNetClient::CreateClient(const char* ip,unsigned short port) {
	Peer = SLNet::RakPeerInterface::GetInstance();
	SLNet::SocketDescriptor sd = SLNet::SocketDescriptor();
	Peer->Startup(1, &sd, 1);
	Peer->Connect(ip, port, 0, 0);
}

void RakNetClient::Disconnection() {
	//const char* message = "Hello RakNet";
	//Peer->Send((char*)message, strlen(message), HIGH_PRIORITY, RELIABLE, 0, SLNet::UNASSIGNED_RAKNET_GUID, true);
	Peer->Shutdown(300);
	delete Peer;
}