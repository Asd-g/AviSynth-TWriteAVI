#pragma once

#include "Compiler.h"

class OutputFormatWAV
{
public:
	virtual void writeData(void* data, int Samples) {};
	virtual ~OutputFormatWAV() {};
};

