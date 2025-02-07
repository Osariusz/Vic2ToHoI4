#include "TankModules.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"



HoI4::TankModules::TankModules(std::istream& theStream)
{
	registerRegex(commonItems::catchallRegex, [this](const std::string& moduleSlot, std::istream& theStream) {
		const commonItems::singleString module(theStream);
		modules.insert(std::make_pair(moduleSlot, module.getString()));
	});

	parseStream(theStream);
}