#include "Exception.h"


Exception::Exception(int nr, string des)
{
	number = nr;
	description = des;
}

int Exception::getNumber()
{
	return this->number;
}

string Exception::getDescription()
{
	return this->description;
}
