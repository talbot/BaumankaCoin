#include "stdafx.h"
#include "Input.h"



bool Input::setOutput(Output out)
{
	output = out;
	return true;
}
bool Input::setOutput(size_t blockNum, size_t txeNum)
{
	output = Output(blockNum, txeNum);
	return true;
}
bool Input::setTailNum(size_t num)
{
	tailNum = num;
	return true;
}
bool Input::setHash(std::vector<uint8_t> info)
{
	outputHash = SHA_256().process(info);
}
std::vector<uint8_t> Input::convertTo8()
{
	std::vector<uint8_t> to;
	converter32to8(output.blockNumber, to);
	converter32to8(output.txeNumber, to);
	for (auto i : outputHash)
	{
		to.push_back(i);
	}
	converter32to8(tailNum, to);
	return to;
}
bool Input::match(Output out, size_t tail, std::vector<uint8_t> info)
{
	if (out != output || tail != tailNum || SHA_256().process(info) != outputHash)
		return false;
	return true;
}
std::pair<Output, size_t> Input::getInfo() const
{
	return std::pair<Output, size_t>(output, tailNum);
}
Input::~Input()
{
}
