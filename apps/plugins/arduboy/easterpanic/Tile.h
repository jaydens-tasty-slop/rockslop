#pragma once

class Tile
{
private:
	uint8_t value;
	
public:
	Tile(void) = default;
	Tile(const uint8_t & value)
		: value(value)
	{
	}
	Tile(const bool & isSolid)
		: value(isSolid ? 0x0 : 0x1)
	{
	}
	
	bool isSolid(void) const
	{
		return ((this->value & 0x1) == 0);
	}
	
	void setIsSolid(const bool & isSolid)
	{
		this->value &= ~0x1;
		if(!isSolid)
			this->value |= 0x1;
	}
};