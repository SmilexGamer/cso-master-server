#pragma once
#include <string>

using namespace std;

class GameUser {
public:
	GameUser();

	unsigned long userID;
	unsigned short flag;
	unsigned char unk1;
	string nickName;
	string unk4_1;
	unsigned char unk4_2;
	unsigned char unk4_3;
	unsigned char unk4_4;
	unsigned char level;
	unsigned char unk10;
	unsigned long long exp;
	unsigned long long cash;
	unsigned long long points;
	unsigned long battles;
	unsigned long wins;
	unsigned long frags;
	unsigned long deaths;
	string pcBang;
	unsigned short city;
	unsigned short county;
	unsigned short neighborhood;
	string unk200_5;
	unsigned long unk400;
	unsigned char unk800;
	unsigned long unk1000_1;
	unsigned long unk1000_2;
	string unk1000_3;
	unsigned char unk1000_4;
	unsigned char unk1000_5;
	unsigned char unk1000_6;
private:

};