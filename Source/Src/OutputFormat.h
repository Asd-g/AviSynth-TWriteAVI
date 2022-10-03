#pragma once

#include "Compiler.h"

class OutputFormat
{
public:
	virtual void writeData(void* data, int dataSize, bool keyframe) {};
	virtual void Audio_writeData(void* data, int Samples) {};						// ssS
	virtual ~OutputFormat() {};
};