#ifndef UNIT_MAPPING_H
#define UNIT_MAPPING_H



#include "HoI4UnitType.h"
#include "Parser.h"
#include <optional>



namespace HoI4
{

class UnitMapping: commonItems::parser
{
  public:
	explicit UnitMapping(std::istream& theStream);

	[[nodiscard]] std::optional<std::pair<std::string, std::vector<HoI4UnitType>>> getMappings() const;

  private:
	std::optional<std::string> Vic2Type;
	std::vector<HoI4UnitType> HoI4Types;
};

} // namespace HoI4



#endif // UNIT_MAPPING_H