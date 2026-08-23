#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"

class RakNetClient
{
public:
	void CreateClient(const char* ip, unsigned short port);
	void Disconnection();

private:
	SLNet::RakPeerInterface* Peer;
};