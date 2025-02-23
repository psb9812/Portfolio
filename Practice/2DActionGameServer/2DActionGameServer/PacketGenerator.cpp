#include "PacketGenerator.h"

void PacketGenerator::MakeCreateMyCharacterPacket(Message* message,
	int id, char hp, short x, short y, unsigned char direction)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_CREATE_MY_CHARACTER_PACKET);
	header.type = PACKET_SC_CREATE_MY_CHARACTER;
	message->PutData((char*) & header, sizeof(header));
	*message << id << hp << x << y << direction;
}

void PacketGenerator::MakeCreateOtherCharactorPacket(Message* message, int id, unsigned char direction, short x, short y, char hp)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_CREATE_OTHER_CHARACTER_PACKET);
	header.type = PACKET_SC_CREATE_OTHER_CHARACTER;
	message->PutData((char*)&header, sizeof(header));
	*message << id << direction << x << y << hp;
}

void PacketGenerator::MakeMoveStartPacket(Message* message, int id, unsigned char direction, short x, short y)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_MOVE_START_PACKET);
	header.type = PACKET_SC_MOVE_START;
	message->PutData((char*)&header, sizeof(header));
	*message << id << direction << x << y;
}

void PacketGenerator::MakeMoveStopPacket(Message* message, int id, unsigned char direction, short x, short y)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_MOVE_STOP_PACKET);
	header.type = PACKET_SC_MOVE_STOP;
	message->PutData((char*)&header, sizeof(header));
	*message << id << direction << x << y;
}

void PacketGenerator::MakeAttack1Packet(Message* message, int id, unsigned char direction, short x, short y)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_ATTACK1_PACKET);
	header.type = PACKET_SC_ATTACK1;
	message->PutData((char*)&header, sizeof(header));
	*message << id << direction << x << y;
}
void PacketGenerator::MakeAttack2Packet(Message* message, int id, unsigned char direction, short x, short y)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_ATTACK2_PACKET);
	header.type = PACKET_SC_ATTACK2;
	message->PutData((char*)&header, sizeof(header));
	*message << id << direction << x << y;
}
void PacketGenerator::MakeAttack3Packet(Message* message, int id, unsigned char direction, short x, short y)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_ATTACK3_PACKET);
	header.type = PACKET_SC_ATTACK3;
	message->PutData((char*)&header, sizeof(header));
	*message << id << direction << x << y;
}

void PacketGenerator::MakeDamagePacket(Message* message, int attackerID, int damageID, char damage)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_DAMAGE_PACKET);
	header.type = PACKET_SC_DAMAGE;
	message->PutData((char*)&header, sizeof(header));
	*message << attackerID << damageID << damage;
}

void PacketGenerator::MakeDeleteCharacterPacket(Message* message, int id)
{
	PACKET_HEADER header;
	header.code = PACKET_CODE;
	header.size = sizeof(SC_DELETE_CHARACTER_PACKET);
	header.type = PACKET_SC_DELETE_CHARACTER;
	message->PutData((char*)&header, sizeof(header));
	*message << id;
}