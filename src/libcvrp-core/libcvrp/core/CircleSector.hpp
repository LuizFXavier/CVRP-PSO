#pragma once

namespace cvrp
{
struct CircleSector
{
  int start{};
  int end{};

	static int 
  positive_mod(int i)
	{
		// Using the formula positive_mod(n,x) = (n % x + x) % x
		return (i % 65536 + 65536) % 65536;
	}

  void
  initialize(int angle)
  {
    start = angle;
    end = angle;
  }

  inline bool 
  isEnclosed(int angle)
	{
		return (positive_mod(angle - start) <= positive_mod(end - start));
	}

  inline bool
  overlap(const CircleSector sector2)
  {
    return ((positive_mod(sector2.start - this->start) <= positive_mod(this->end - this->start))
			|| (positive_mod(this->start - sector2.start) <= positive_mod(sector2.end - sector2.start)));
  }

  inline void 
  extend(int angle)
	{
		if (!isEnclosed(angle))
		{
			if (positive_mod(angle - end) <= positive_mod(start - angle))
				end = angle;
			else
				start = angle;
		}
	}
}; 
} // namespace cvrp