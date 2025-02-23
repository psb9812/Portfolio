#pragma once
#include "PacketDefine.h"
#include "Message.h"

#define PACKET_CODE		0x89

class PacketGenerator
{
private:

public:
	void MakeCreateMyCharacterPacket(Message* message, int id, char hp, short x, short y, unsigned char direction);
	void MakeCreateOtherCharactorPacket(Message* message,int id, unsigned char direction, short x, short y, char hp);
	void MakeMoveStartPacket(Message* message,int id, unsigned char direction, short x, short y);
	void MakeMoveStopPacket(Message* message, int id, unsigned char direction, short x, short y);
	void MakeAttack1Packet(Message* message, int id, unsigned char direction, short x, short y);
	void MakeAttack2Packet(Message* message, int id, unsigned char direction, short x, short y);
	void MakeAttack3Packet(Message* message, int id, unsigned char direction, short x, short y);
	void MakeDamagePacket(Message* message, int attackerID, int damageID, char damage);
	void MakeDeleteCharacterPacket(Message* message, int id);
};