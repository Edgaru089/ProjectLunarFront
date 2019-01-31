#pragma once

#include <cstdlib>
#include <string>
#include <random>
#include <istream>
#include <ostream>

using namespace std;

extern mt19937 uuid_random_engine;
extern uniform_int_distribution<unsigned int> uuid_distribution;

class Uuid {
public:

	static void seed(unsigned int val = 5489u) { uuid_random_engine.seed(val); }

	// Version 4.1 (Random)
	static Uuid get() {
		Uuid id;

		unsigned int x = uuid_distribution(uuid_random_engine);
		id.sc1 = x;

		x = uuid_distribution(uuid_random_engine);
		id.sc2 = x & 0xFFFFu;                 //  Lower 16 bits (2 bytes)
		id.sc3 = x & (x & 0xFFFF0000u) >> 16; // Higher 16 bits (2 bytes)

		x = uuid_distribution(uuid_random_engine);
		id.sc4 = x & 0xFFFFu;                   //  Lower 16 bits (2 bytes)
		id.sc5_high2 = (x & 0xFFFF0000u) >> 16; // Higher 16 bits (2 bytes)

		x = uuid_distribution(uuid_random_engine);
		id.sc5_low4 = x;

		// Set the version bit (xxxxxxxx-xxxx-4xxx-axxx-xxxxxxxxxxxx)

		// sc3: binary(0100 xxxx xxxx xxxx)
		id.sc3 &= 0x0FFF;  // Set upper 4 bits to 0
		id.sc3 |= 0x4000;  // Set the 4 bit

		// sc4: binary(0100 xxxx xxxx xxxx)
		id.sc4 &= 0x0FFF;  // Set upper 4 bits to 0
		id.sc4 |= 0xA000;  // Set the A bit

		return id;
	}

	static Uuid nil() { return Uuid(); }

public:
	// Empty Uuid ( Nil 00000000-0000-0000-0000-000000000000 )
	Uuid() :sc1(0), sc2(0), sc3(0), sc4(0), sc5_high2(0), sc5_low4(0) {}
	Uuid(const Uuid& copy) :sc1(copy.sc1), sc2(copy.sc2), sc3(copy.sc3), sc4(copy.sc4), sc5_high2(copy.sc5_high2), sc5_low4(copy.sc5_low4) {}
	Uuid(unsigned long long high, unsigned long long low) {
		sc1 = high >> 32;
		sc2 = (high >> 16) & 0xFFFF;
		sc3 = high & 0xFFFF;
		sc4 = (low >> 48) & 0xFFFF;
		sc5_high2 = (low >> 32) & 0xFFFF;
		sc5_low4 = low & 0xFFFFFFFF;
	}
	Uuid(const string& str) {
		if (sscanf(str.c_str(), "%08x-%04hx-%04hx-%04hx-%04hx%08x", &sc1, &sc2, &sc3, &sc4, &sc5_high2, &sc5_low4) != 6)
			sc1 = sc2 = sc3 = sc4 = sc5_high2 = sc5_low4 = 0;
	}

	const Uuid& operator = (const Uuid& copy) {
		sc1 = copy.sc1;
		sc2 = copy.sc2;
		sc3 = copy.sc3;
		sc4 = copy.sc4;
		sc5_high2 = copy.sc5_high2;
		sc5_low4 = copy.sc5_low4;
		return *this;
	}

	string toString() const {
		char buf[60];
		sprintf(buf, "%08x-%04hx-%04hx-%04hx-%04hx%08x", sc1, sc2, sc3, sc4, sc5_high2, sc5_low4);
		return string(buf);
	}

	pair<unsigned long long, unsigned long long> toULLPair() const {
		typedef unsigned long long ull;
		return make_pair((((ull)sc1) << 32) + (((ull)sc2) << 16) + ((ull)sc3),
			(((ull)sc4) << 48) + (((ull)sc5_high2) << 32) + ((ull)sc5_low4));
	}

public:

	unsigned int    sc1;
	unsigned short  sc2, sc3, sc4;
	unsigned short  sc5_high2;
	unsigned int    sc5_low4;
};

inline const bool operator == (const Uuid& left, const Uuid& right) {
	return left.sc1 == right.sc1 && left.sc2 == right.sc2 &&
		left.sc3 == right.sc3 && left.sc4 == right.sc4
		&& left.sc5_high2 == right.sc5_high2 &&
		left.sc5_low4 == right.sc5_low4;
}

inline const bool operator < (const Uuid& left, const Uuid& right) {
	return left.sc1 < right.sc1 ||
		(left.sc1 == right.sc1 &&
		 left.sc2 < right.sc2) ||
		 (left.sc1 == right.sc1 && left.sc2 == right.sc2 &&
		  left.sc3 < right.sc3) ||
		  (left.sc1 == right.sc1 && left.sc2 == right.sc2 &&
		   left.sc3 == right.sc3 && left.sc4 < right.sc4) ||
		   (left.sc1 == right.sc1 && left.sc2 == right.sc2 && left.sc3 == right.sc3 &&
			left.sc4 == right.sc4 && left.sc5_high2 < right.sc5_high2) ||
			(left.sc1 == right.sc1 && left.sc2 == right.sc2 &&
			 left.sc3 == right.sc3 && left.sc4 == right.sc4 &&
			 left.sc5_high2 == right.sc5_high2 && left.sc5_low4 < right.sc5_low4);
}

inline const bool operator != (const Uuid& left, const Uuid& right) { return !(left == right); }
inline const bool operator <= (const Uuid& left, const Uuid& right) { return (left == right) || (left < right); }
inline const bool operator > (const Uuid& left, const Uuid& right) { return !(left <= right); }
inline const bool operator >= (const Uuid& left, const Uuid& right) { return !(left < right); }


#ifdef SFML_PACKET_HPP

inline sf::Packet& operator <<(sf::Packet& packet, const Uuid& id) {
	return packet << id.sc1 << id.sc2 << id.sc3 << id.sc4 << id.sc5_high2 << id.sc5_low4;
}

inline sf::Packet& operator >>(sf::Packet& packet, Uuid& id) {
	return packet >> id.sc1 >> id.sc2 >> id.sc3 >> id.sc4 >> id.sc5_high2 >> id.sc5_low4;
}

#endif
